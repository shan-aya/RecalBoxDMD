-- license:BSD-3-Clause
-- hiscore_probe/init.lua v1 -- 2026-08-25
-- Plugin MAME reel (PAS un autoboot_script) -- structure calquee sur le
-- plugin officiel "autofire" (meme fichier lu comme reference cette
-- session, /usr/share/libretro-mame/mame0278/plugins/autofire/init.lua) :
-- emu.add_machine_frame_notifier() enregistre UNE FOIS dans
-- startplugin(), l'initialisation dependante d'une machine chargee
-- (zones RAM, champs ioport, emu.romname()) est deportee dans
-- emu.register_prestart() -- EXACTEMENT le motif qu'utilise autofire
-- lui-meme pour son propre callback process_frame(), et qui survit a
-- une session complete (autofire fonctionne, verifie en usage reel par
-- toute la communaute MAME).
--
-- Raison d'etre : sur cette meme investigation via -autoboot_script
-- (voir tools/dump_full_memmap.lua v3 + DECISIONS.md "Lua autoboot_script
-- (RB2, MAME0278) -- le notifier par frame plafonne a ~150 frames"),
-- le notifier s'arretait silencieusement apres ~150 frames (~2.5s) SANS
-- ERREUR, 5 hypotheses testees et ecartees avec preuve directe (E/S,
-- action press, reference GC, prestart hook seul, DRC). Piste retenue :
-- autoboot_script n'est probablement pas concu pour un usage
-- long-vivant -- un vrai plugin utilise un mecanisme de chargement
-- different, potentiellement plus robuste.
-- **CONFIRME EN PRATIQUE (2026-08-25)** : ce plugin a tourne la sequence
-- COMPLETE (11 phases, heartbeat jusqu'a 840+ frames) sur 16/16 roms en
-- lot automatise, la ou -autoboot_script plafonnait a ~150 frames sans
-- exception -- voir DECISIONS.md pour le detail et les resultats.
--
-- Sequence pilotee en frames : BOOT_SETTLE -> credit -> POST_CREDIT ->
-- start -> POST_START -> dwell PLAY_1..8 (score/vies suivis dans le
-- temps). Lecture RAM etalee sur plusieurs frames (BYTES_PER_FRAME)
-- pour rester leger par callback, gardee de la version autoboot_script
-- meme si elle ne resolvait pas le plafond -- bonne pratique par
-- ailleurs.
--
-- DEPLOIEMENT (different d'un autoboot_script, PAS un simple chemin de
-- fichier dans mame.ini) :
--   1. Copier ce dossier entier vers un chemin ECRIVABLE (PAS
--      /usr/share/... -- lecture seule confirme, overlay root) que MAME
--      peut scanner, ex. /recalbox/share/bios/mame/plugins/hiscore_probe/
--      (init.lua + plugin.json).
--   2. mame.ini : pluginspath doit lister LES DEUX chemins, separes par
--      ';' -- l'original (lecture seule, hiscore/cheat/autofire/etc.)
--      ET le nouveau chemin ecrivable, sinon les plugins standards
--      disparaissent :
--        pluginspath   /usr/share/libretro-mame/mame0278/plugins;/recalbox/share/bios/mame/plugins
--   3. plugin.ini (meme dossier ini que mame.ini) active/desactive
--      explicitement CE plugin (independant du "start":"true" du
--      plugin.json, qui seul ne suffit pas de maniere fiable) :
--        hiscore_probe     1    -- active
--        hiscore_probe     0    -- desactive (RESTAURER APRES USAGE --
--      ce plugin presse credit+start sur CHAQUE lancement MAME tant
--      qu'il est actif, y compris pendant d'autres campagnes de
--      recolte -- ne jamais le laisser a 1 par defaut).
--   4. Sortie : SNAPSHOT_OUT (env, defaut /tmp/mame_lua_snapshot.txt),
--      format TSV documente dans dump_full_memmap.lua (ZONE/SNAP/
--      #heartbeat/#done).

local exports = {
	name = 'hiscore_probe',
	version = '0.0.2',
	description = 'RAM snapshot diff probe (safe-modify, hi-score generique)',
	license = 'BSD-3-Clause',
	author = { name = 'safe-modify' } }

local hiscore_probe = exports

local frame_subscription

function hiscore_probe.startplugin()
	local function safe(fn)
		local ok, val = pcall(fn)
		if ok then return val else return nil end
	end

	-- etat courant (reinitialise a chaque prestart, i.e. a chaque
	-- nouvelle machine/rom chargee)
	local snap_file = nil
	local zones = {}
	local pending = nil
	local frame_count = 0
	local next_idx = 1
	local PHASES = {}
	local coin_field = nil
	local start_field = nil

	local EXCLUDE_KEYWORDS = {"gfx", "palette", "scroll", "object", "sprite",
		"vram", "tile", "qsound", "bg_", "fg_", "video", "char"}
	local function is_excluded_name(name)
		if not name or name == "" then return false end
		local low = string.lower(name)
		for _, kw in ipairs(EXCLUDE_KEYWORDS) do
			if string.find(low, kw, 1, true) then return true end
		end
		return false
	end

	-- v2 - 2026-08-25 - safe-modify - BUG REEL trouve en analysant les 5
	-- jeux (nemo/msgogo/mtwins/gogomile/willow) qui finissaient avec
	-- #zones=0 malgre coin/start bien trouves : ces jeux ont une zone
	-- de RAM de travail bien reelle et souvent NOMMEE explicitement
	-- "mainram" (CPS1 : 00ff0000-00ffffff, 64 Ko) mais l'ancien plafond
	-- MAX_ZONE_SIZE=32768 l'excluait PUREMENT par taille (aucun mot-cle
	-- ne matchait) -- confirme en comparant au dump memmap complet
	-- (dump_full_memmap.lua, sans plafond). Plafond releve a 128 Ko
	-- (couvre ce cas courant tout en excluant toujours les gros buffers
	-- graphiques, ex. CPS1 gfxram 00900000-0092ffff/192 Ko, deja exclu
	-- par mot-cle "gfx" independamment de la taille). BYTES_PER_FRAME
	-- releve en consequence (128 -> 1024, une zone 64 Ko passe de ~512
	-- frames/8.5s a ~64 frames/1s) pour ne pas allonger excessivement
	-- la duree de session -- le plafond ~150 frames qui motivait un
	-- decoupage tres prudent etait specifique a -autoboot_script (voir
	-- DECISIONS.md), un vrai plugin n'a pas montre cette limite meme a
	-- 840+ frames.
	local MAX_ZONE_SIZE = 131072
	local BYTES_PER_FRAME = 1024

	local function start_snapshot(phase)
		pending = {phase = phase, zone_idx = 1, addr = zones[1] and zones[1].astart or nil, buffers = {}}
		for i = 1, #zones do pending.buffers[i] = {} end
	end

	local function step_snapshot()
		if not pending then return false end
		local budget = BYTES_PER_FRAME
		while budget > 0 and pending.zone_idx <= #zones do
			local z = zones[pending.zone_idx]
			if pending.addr > z.aend then
				pending.zone_idx = pending.zone_idx + 1
				if pending.zone_idx <= #zones then
					pending.addr = zones[pending.zone_idx].astart
				end
			else
				local ok, val = pcall(function() return z.space:read_u8(pending.addr) end)
				local buf = pending.buffers[pending.zone_idx]
				buf[#buf + 1] = ok and string.format("%02x", val) or "??"
				pending.addr = pending.addr + 1
				budget = budget - 1
			end
		end
		if pending.zone_idx > #zones then
			for i, z in ipairs(zones) do
				snap_file:write(string.format("SNAP\t%s\t%d\t%s\n", pending.phase, i, table.concat(pending.buffers[i])))
			end
			snap_file:flush()
			pending = nil
			return true
		end
		return false
	end

	local function process_frame()
		if not snap_file then return end  -- pas encore initialise pour cette machine (avant le 1er prestart)
		local ok, err = pcall(function()
			frame_count = frame_count + 1
			if frame_count % 30 == 0 then
				snap_file:write(string.format("#heartbeat\t%d\n", frame_count))
				snap_file:flush()
			end
			if pending then
				step_snapshot()
			end
			while (not pending) and next_idx <= #PHASES and PHASES[next_idx].at <= frame_count do
				local p = PHASES[next_idx]
				if p.action == "snap" then
					start_snapshot(p.name)
					step_snapshot()
				elseif p.action == "press" then
					if p.field then safe(function() p.field:set_value(1) end) end
				elseif p.action == "release" then
					if p.field then safe(function() p.field:clear_value() end) end
				end
				next_idx = next_idx + 1
			end
			if next_idx > #PHASES and (not pending) and snap_file then
				snap_file:write("#done\n")
				snap_file:close()
				snap_file = nil
			end
		end)
		if not ok and snap_file then
			snap_file:write(string.format("#FRAME_ERROR\t%d\t%s\n", frame_count, tostring(err)))
			snap_file:flush()
		end
	end

	local function setup_for_new_machine()
		local romname = safe(function() return emu.romname() end) or "?"
		local out_path = os.getenv("SNAPSHOT_OUT") or "/tmp/mame_lua_snapshot.txt"
		snap_file = io.open(out_path, "w")
		if not snap_file then return end
		snap_file:write("#game\t" .. romname .. "\n")
		snap_file:write("#plugin\thiscore_probe\n")

		zones = {}
		frame_count = 0
		next_idx = 1
		pending = nil

		local devices = safe(function() return manager.machine.devices end)
		if devices then
			for tag, dev in pairs(devices) do
				local low_tag = string.lower(tag)
				if not (string.find(low_tag, "audio", 1, true) or string.find(low_tag, "sound", 1, true)) then
					local spaces = safe(function() return dev.spaces end)
					if spaces then
						for spname, space in pairs(spaces) do
							local map = safe(function() return space.map end)
							local entries = map and safe(function() return map.entries end)
							if entries then
								for _, entry in ipairs(entries) do
									local rd = safe(function() return entry.read end)
									local wr = safe(function() return entry.write end)
									local rtype = rd and safe(function() return rd.handlertype end)
									local wtype = wr and safe(function() return wr.handlertype end)
									if rtype == "ram" or wtype == "ram" then
										local astart = safe(function() return entry.address_start end)
										local aend = safe(function() return entry.address_end end)
										local share = safe(function() return entry.share end)
										local region = safe(function() return entry.region end)
										local label = share or region or ""
										if astart and aend and not is_excluded_name(label) then
											local size = aend - astart + 1
											if size and size > 0 and size <= MAX_ZONE_SIZE then
												table.insert(zones, {tag = tag, space = space, spname = spname,
													astart = astart, aend = aend, label = label})
											end
										end
									end
								end
							end
						end
					end
				end
			end
		end
		snap_file:write(string.format("#zones\t%d\n", #zones))
		for i, z in ipairs(zones) do
			snap_file:write(string.format("ZONE\t%d\t%s\t%s\t%08x\t%08x\t%s\n",
				i, z.tag, z.spname, z.astart, z.aend, z.label))
		end

		local COIN_NAMES = {"Coin 1", "Coin1", "Coin A", "Service 1"}
		local START_NAMES = {"1 Player Start", "P1 Start", "Start 1", "1P Start", "Start"}
		local ports2 = safe(function() return manager.machine.ioport.ports end)
		local function find_field(names)
			if not ports2 then return nil end
			for ptag, port in pairs(ports2) do
				local fields = safe(function() return port.fields end)
				if fields then
					for fname, field in pairs(fields) do
						for _, n in ipairs(names) do
							if fname == n then return field end
						end
					end
				end
			end
			return nil
		end
		coin_field = find_field(COIN_NAMES)
		start_field = find_field(START_NAMES)
		snap_file:write(string.format("#coin_field_found\t%s\n", tostring(coin_field ~= nil)))
		snap_file:write(string.format("#start_field_found\t%s\n", tostring(start_field ~= nil)))

		PHASES = {
			{at = 180, action = "snap", name = "BOOT_SETTLE"},
			{at = 190, action = "press", field = coin_field},
			{at = 196, action = "release", field = coin_field},
			{at = 240, action = "snap", name = "POST_CREDIT"},
			{at = 250, action = "press", field = start_field},
			{at = 256, action = "release", field = start_field},
			{at = 360, action = "snap", name = "POST_START"},
			{at = 420, action = "snap", name = "PLAY_1"},
			{at = 480, action = "snap", name = "PLAY_2"},
			{at = 540, action = "snap", name = "PLAY_3"},
			{at = 600, action = "snap", name = "PLAY_4"},
			{at = 660, action = "snap", name = "PLAY_5"},
			{at = 720, action = "snap", name = "PLAY_6"},
			{at = 780, action = "snap", name = "PLAY_7"},
			{at = 840, action = "snap", name = "PLAY_8"},
		}
		snap_file:flush()
	end

	frame_subscription = emu.add_machine_frame_notifier(process_frame)
	emu.register_prestart(setup_for_new_machine)
end

return exports
