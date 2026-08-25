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
	version = '0.0.22',
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
	local fire_field = nil
	local down_field = nil
	local up_field = nil
	local forward_field = nil

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
		-- v14/v15/v16 - 2026-08-25 - safe-modify - TENTATIVE ABANDONNEE :
		-- manager.machine.video:snapshot() (avec ou sans argument nom de
		-- fichier) etait envisagee pour eliminer la latence du sondage
		-- externe (~1s, cause probable de l'echec a correler un score
		-- >600 sur inthunt). Resultat confirme sur matiere reelle :
		-- l'appel Lua reussit TOUJOURS (pcall ok, aucune erreur, 30/30
		-- phases) mais NE PRODUIT JAMAIS AUCUN FICHIER (verifie par
		-- `ls`/`find` sur tout le systeme apres coup) -- cette API MAME
		-- native semble interceptee/ignoree silencieusement dans ce
		-- contexte libretro (le core mame0278 gere probablement son
		-- propre pipeline de sortie video, sans relayer les appels
		-- video:snapshot() de MAME vers un vrai fichier disque). NE PAS
		-- RETENTER cette piste sans preuve nouvelle -- rester sur la
		-- methode externe (commande reseau SCREENSHOT, deja fiable tout
		-- au long de ce projet) et compenser sa latence par un
		-- espacement de phases plus large plutot que chercher a
		-- l'eliminer cote MAME.
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

	-- v5 - 2026-08-25 - safe-modify - BUG REEL trouve par verite d'abord :
	-- sur `inthunt`, le credit prenait bien effet ("CREDIT 01" visible
	-- a l'ecran, confirme par capture) mais START restait totalement
	-- sans effet meme jusqu'a PLAY_8 (toujours l'ecran titre, capture
	-- reelle) -- alors que coin_field/start_field sont TOUS LES DEUX
	-- trouves. Relecture du code source du plugin "autofire" (deja lu
	-- comme reference cette session) : il REAFFIRME set_value()/
	-- clear_value() a CHAQUE FRAME tant que la touche physique est
	-- tenue (boucle process_frame -> process_button appelee en boucle,
	-- pas un seul appel ponctuel). Notre "press" ne faisait qu'UN SEUL
	-- appel set_value(1) -- suffisant pour un detecteur a FRONT MONTANT
	-- (le credit, edge-triggered sur la plupart des drivers -- d'ou son
	-- succes), mais probablement insuffisant pour un bouton lu comme
	-- MAINTENU sur plusieurs frames (start, sur ce driver en tout cas).
	-- Fix : liste de holds actifs, reaffirme set_value(1) CHAQUE frame
	-- tant que le hold est actif, clear_value() une seule fois a la fin.
	local active_holds = {}  -- { {field=, until_frame=}, ... }

	local function process_frame()
		if not snap_file then return end  -- pas encore initialise pour cette machine (avant le 1er prestart)
		local ok, err = pcall(function()
			frame_count = frame_count + 1
			if frame_count % 30 == 0 then
				snap_file:write(string.format("#heartbeat\t%d\n", frame_count))
				snap_file:flush()
			end

			-- reaffirme chaque hold actif CETTE frame (avant tout le
			-- reste, pour ne jamais sauter une frame de maintien)
			for i = #active_holds, 1, -1 do
				local h = active_holds[i]
				if frame_count < h.until_frame then
					safe(function() h.field:set_value(1) end)
					-- v12 - 2026-08-25 - safe-modify - DIAGNOSTIC : verifie
					-- si set_value() est reellement vu "presse" en relisant
					-- le champ juste apres (field.live.value si accessible)
					-- -- sur inthunt, tir maintenu ET tapote donnent un
					-- resultat final IDENTIQUE (meme frame exacte), suspect
					-- d'un tir sans aucun effet reel.
					if frame_count % 60 == 0 and snap_file then
						local live = safe(function() return h.field.live end)
						local lv = live and safe(function() return live.value end)
						snap_file:write(string.format("#hold_readback\t%d\t%s\n", frame_count, tostring(lv)))
					end
				else
					safe(function() h.field:clear_value() end)
					table.remove(active_holds, i)
				end
			end

			if pending then
				step_snapshot()
			end
			while (not pending) and next_idx <= #PHASES and PHASES[next_idx].at <= frame_count do
				local p = PHASES[next_idx]
				if p.action == "snap" then
					start_snapshot(p.name)
					step_snapshot()
				elseif p.action == "hold" then
					if p.field then
						table.insert(active_holds, {field = p.field, until_frame = frame_count + p.duration})
						safe(function() p.field:set_value(1) end)
					end
				end
				next_idx = next_idx + 1
			end
			if next_idx > #PHASES and (not pending) and #active_holds == 0 and snap_file then
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
		-- v6 - 2026-08-25 - safe-modify - demande utilisateur explicite :
		-- ajouter un tir automatique pendant le dwell pour generer un
		-- vrai score (sans ca, un jeu qui demarre vraiment reste a
		-- score=0 toute la session, comme confirme sur `inthunt` --
		-- aucun candidat "score" ne peut alors etre valide par verite
		-- d'abord). Noms usuels du bouton de tir principal.
		local FIRE_NAMES = {"Button 1", "P1 Button 1", "1P Button 1"}
		-- v8 - 2026-08-25 - safe-modify - le tir seul ne suffit pas
		-- (confirme sur inthunt : score reste a 0, le sous-marin reste
		-- immobile faute de mouvement -- capture reelle) -- ajoute un
		-- mouvement tenu pendant le dwell, en plus du tir, pour
		-- explorer et croiser des ennemis.
		-- v9 - 2026-08-25 - safe-modify - demande utilisateur : direction
		-- generalisee selon le type de scrolling du jeu (info venant du
		-- genre gamelist.xml, cote externe -- ce script Lua n'a pas
		-- acces au XML) -- lu via MOVE_DIRECTION (env, defaut "down").
		-- Scroll horizontal -> mouvement vertical (down/up) pour
		-- balayer l'ecran ; scroll vertical -> mouvement horizontal
		-- (right/left).
		-- v10 - 2026-08-25 - safe-modify - retour utilisateur : le
		-- mouvement doit etre CYCLIQUE (alterner les 2 sens), pas
		-- maintenu dans une seule direction en continu -- confirme par
		-- capture reelle que "down" seul coince le sous-marin contre le
		-- fond. OPPOSITE_DIR donne le sens oppose a alterner.
		local MOVE_DIRECTION = os.getenv("MOVE_DIRECTION") or "down"
		local MOVE_NAMES_BY_DIR = {
			down = {"P1 Down", "Down", "1P Down"},
			up = {"P1 Up", "Up", "1P Up"},
			right = {"P1 Right", "Right", "1P Right"},
			left = {"P1 Left", "Left", "1P Left"},
		}
		local OPPOSITE_DIR = {down = "up", up = "down", right = "left", left = "right"}
		local DOWN_NAMES = MOVE_NAMES_BY_DIR[MOVE_DIRECTION] or MOVE_NAMES_BY_DIR.down
		local UP_NAMES = MOVE_NAMES_BY_DIR[OPPOSITE_DIR[MOVE_DIRECTION] or "up"] or MOVE_NAMES_BY_DIR.up
		-- v18 - 2026-08-25 - safe-modify - retour utilisateur : "un
		-- scrolling horizontal n'impose pas forcement un deplacement
		-- horizontal, c'est souvent le joueur qui se deplace pour
		-- avancer dans le decor -- ici on reste statique en tirant
		-- (mouvement haut-bas uniquement), il faut aussi aller vers
		-- l'avant". Confirme par capture reelle : tir visible (vrai
		-- projectile a l'ecran) mais score toujours 0 apres ~90s de
		-- pilotage sans jamais avancer.
		-- v21 - 2026-08-25 - safe-modify - BUG REEL trouve par verite
		-- d'abord sur `gbusters` (retour utilisateur en direct : "ici
		-- c'est un scrolling vertical donc on inverse... coince dans le
		-- decor a droite et tu tires a droite donc 0 kill") : la
		-- deduction automatique "avant = perpendiculaire au balayage"
		-- devinait TOUJOURS "right" quel que soit le sens de balayage --
		-- fausse pour un scroll VERTICAL (balayage gauche/droite, avance
		-- = HAUT, pas droite). Remplace par une variable d'environnement
		-- dediee et explicite (FORWARD_DIRECTION), independante de
		-- MOVE_DIRECTION -- plus fiable qu'un mapping devine.
		local FORWARD_DIRECTION = os.getenv("FORWARD_DIRECTION") or "right"
		local FORWARD_NAMES = MOVE_NAMES_BY_DIR[FORWARD_DIRECTION] or MOVE_NAMES_BY_DIR.right
		local ports2 = safe(function() return manager.machine.ioport.ports end)
		-- v20 - 2026-08-25 - safe-modify - BUG REEL trouve par verite
		-- d'abord sur `gbusters` (retour utilisateur en direct : "tu n'as
		-- fait aucun insert coin" / "le texte est PLEASE INSERT COIN") :
		-- le "coin_field" trouve etait en realite le champ DIP SWITCH
		-- "Coin A" sur le port :DSW1 (reglage de configuration de la
		-- monnayeur, PAS l'entree live de credit) -- bug deja identifie
		-- (ordre non deterministe de pairs()) mais jamais corrige avant
		-- de tomber dessus en vrai. Fix double : (1) parcourt les NOMS
		-- dans l'ordre de PRIORITE (pas les champs dans un ordre
		-- aleatoire), (2) exclut explicitement tout champ dont
		-- type_class est "dipswitch" ou "config" -- meme logique
		-- d'exclusion que le plugin officiel "autofire"
		-- (is_supported_input(), deja lu comme reference cette session).
		local function find_field(names)
			if not ports2 then return nil end
			for _, n in ipairs(names) do
				for ptag, port in pairs(ports2) do
					local fields = safe(function() return port.fields end)
					if fields then
						local field = fields[n]
						if field then
							local tclass = safe(function() return field.type_class end)
							if tclass ~= "dipswitch" and tclass ~= "config" then
								return field, ptag, n
							end
						end
					end
				end
			end
			return nil
		end
		local coin_ptag, coin_fname, start_ptag, start_fname, fire_ptag, fire_fname
		coin_field, coin_ptag, coin_fname = find_field(COIN_NAMES)
		start_field, start_ptag, start_fname = find_field(START_NAMES)
		fire_field, fire_ptag, fire_fname = find_field(FIRE_NAMES)
		local down_ptag, down_fname, up_ptag, up_fname, forward_ptag, forward_fname
		down_field, down_ptag, down_fname = find_field(DOWN_NAMES)
		up_field, up_ptag, up_fname = find_field(UP_NAMES)
		forward_field, forward_ptag, forward_fname = find_field(FORWARD_NAMES)
		snap_file:write(string.format("#forward_field_found\t%s\t%s\t%s\n",
			tostring(forward_field ~= nil), tostring(forward_ptag), tostring(forward_fname)))
		snap_file:write(string.format("#coin_field_found\t%s\t%s\t%s\n",
			tostring(coin_field ~= nil), tostring(coin_ptag), tostring(coin_fname)))
		snap_file:write(string.format("#start_field_found\t%s\t%s\t%s\n",
			tostring(start_field ~= nil), tostring(start_ptag), tostring(start_fname)))
		snap_file:write(string.format("#fire_field_found\t%s\t%s\t%s\n",
			tostring(fire_field ~= nil), tostring(fire_ptag), tostring(fire_fname)))
		snap_file:write(string.format("#down_field_found\t%s\t%s\t%s\n",
			tostring(down_field ~= nil), tostring(down_ptag), tostring(down_fname)))
		snap_file:write(string.format("#up_field_found\t%s\t%s\t%s\n",
			tostring(up_field ~= nil), tostring(up_ptag), tostring(up_fname)))
		if coin_field then
			snap_file:write(string.format("#coin_field_mask\t%s\n", tostring(safe(function() return coin_field.mask end))))
			snap_file:write(string.format("#coin_field_type\t%s\n", tostring(safe(function() return coin_field.type end))))
		end

		-- v3 - 2026-08-25 - safe-modify - BUG REEL trouve par "verite
		-- d'abord" (captures SCREENSHOT reelles demandees explicitement
		-- par l'utilisateur) : sur `progear`, BOOT_SETTLE=180 (3s)
		-- tombe encore en PLEIN dans l'intro non-interactive (warning
		-- legal USA/Canada/Mexico -> logo QSound -> logo CAVE) --
		-- confirme visuellement a 3 instants differents (BOOT_SETTLE/
		-- POST_CREDIT/POST_START), TOUJOURS le meme ecran de warning.
		-- Mesure directe (captures toutes les 5s) : l'ecran titre reel
		-- ("INSERT 2 COINS") n'apparait que vers t=20s (~1100-1200
		-- frames), pas 3s -- meme famille de piege que `darkseal` deja
		-- documente ailleurs dans ce depot (intro non-interactive
		-- longue, deviner un instant fixe court ne peut pas marcher).
		-- Explique tres probablement `credit_diff=0` observe sur
		-- dynagear/inthunt/kamenrid/progear/pzloop2/tbyahhoo (aucun
		-- octet ne bouge -- normal, la partie n'a jamais commence).
		-- `progear` demande en plus explicitement "INSERT 2 COINS" --
		-- 2e appui credit ajoute (harmless pour les jeux a 1 credit,
		-- laisse juste un credit en banque).
		-- BOOT_SETTLE releve a 1400 (~23s, marge sur les 20s mesures) --
		-- accepte une session sensiblement plus longue par jeu en
		-- echange de la fiabilite, plus de plafond d'infrastructure
		-- avec un vrai plugin (voir DECISIONS.md) donc ce compromis est
		-- maintenant praticable sans risque technique.
		-- v4 - 2026-08-25 - safe-modify - DIAGNOSTIC : sur `progear`,
		-- meme arrive au bon ecran titre interactif (fix v3 confirme
		-- par capture reelle), le credit reste sans AUCUN effet
		-- (credit_diff=0 en RAM, "CREDITS: 0(0/2)" inchange sur 3
		-- captures). Hypothese testee ici : appui trop court (6 frames/
		-- 100ms) -- allonge a 30 frames (0.5s) par appui, sur credit ET
		-- start. Diagnostic complementaire ajoute plus haut (log du
		-- port/masque/type du champ trouve) pour confirmer ou ecarter
		-- une hypothese "mauvais champ" en parallele.
		-- v6 - 2026-08-25 - safe-modify - BUG REEL trouve par verite
		-- d'abord sur `progear` : meme avec le "hold" reaffirme chaque
		-- frame (v5), seul 1 credit sur 2 prenait effet ("INSERT 1 MORE
		-- COIN"/"CREDITS: 0(1/2)" fige jusqu'a PLAY_8, capture reelle) --
		-- le candidat "+10 par etape" precedemment trouve etait donc
		-- FAUX (jeu jamais demarre). Hypothese : l'ecart de 10 frames
		-- entre les 2 holds credit est trop court pour ce driver (pas
		-- assez de temps a "0" entre les deux appuis pour que le
		-- compteur de pieces les distingue). Ecart porte a 60 frames.
		-- v13 - 2026-08-25 - safe-modify - DIAGNOSTIC TEMPORAIRE :
		-- decouverte majeure par capture manuelle (commandes reseau
		-- PLAYER1_SELECT/START envoyees HORS de ce plugin, jeu lance
		-- normalement) -- l'attract mode d'inthunt joue une VRAIE demo
		-- de gameplay avec un score REEL non nul ("1P 00000400"/
		-- "2P 00000100" observes sur capture d'ecran), overlay
		-- "INSERT COIN" toujours present (la demo tourne independamment
		-- de l'etat credit). Nos propres holds credit/start/tir
		-- interferaient probablement avec cette demo scriptee -- tout
		-- retire ici. Uniquement des snapshots reguliers pendant que la
		-- demo tourne naturellement, pour capturer la RAM au moment ou
		-- un score reel est affiche (a correler avec une capture
		-- SCREENSHOT externe prise au meme instant via le meme
		-- mecanisme que test_visual_play.sh).
		-- v18 - 2026-08-25 - safe-modify - retour utilisateur explicite :
		-- "il vaut mieux inserer des credits et jouer MAIS pour traiter
		-- les 3000 jeux de mame je vais pas pouvoir jouer moi meme c'est
		-- toi qui va devoir le faire" -- l'approche "demo attract-mode a
		-- score reel" (v13-v17) ne generalise PAS (2/2 jeux testes apres
		-- inthunt -- willow, gbusters -- n'ont pas de demo a score
		-- exploitable, confirme par observation directe utilisateur sur
		-- l'ecran physique). Retour a la sequence PILOTEE (credit+start+
		-- tir+mouvement, deja construite v0.0.6-v0.0.12) mais avec une
		-- fenetre de jeu BEAUCOUP plus longue (~85s au lieu de ~5s) et
		-- des snapshots reguliers tout du long, pour verifier une bonne
		-- fois si le pilotage automatique peut generer un vrai kill (le
		-- point jamais tranche lors des essais precedents sur inthunt --
		-- score reste reste a 0 mais fenetre de test bien plus courte
		-- alors).
		-- v22 - 2026-08-25 - safe-modify - 1er essai "continue" avec 2
		-- credits seulement : le 2e credit a ete consomme AVANT l'ecran
		-- CONTINUE (vie bonus auto a une mort precedente, mecanisme
		-- distinct), donc l'ecran CONTINUE est bien apparu ("CONTINUE 7"
		-- countdown confirme par capture ecran) mais CREDIT 00 au meme
		-- moment -- countdown expire faute de credit, GAME OVER (capture
		-- ecran "GAME OVER" qui suit confirme). Aucun vrai continue n'a
		-- pu etre accepte. On monte a 5 credits pour garantir qu'il en
		-- reste au moins un disponible quand l'ecran CONTINUE apparait.
		PHASES = {
			{at = 1400, action = "snap", name = "BOOT_SETTLE"},
			{at = 1410, action = "hold", field = coin_field, duration = 30},
			{at = 1500, action = "hold", field = coin_field, duration = 30},
			{at = 1590, action = "hold", field = coin_field, duration = 30},
			{at = 1680, action = "hold", field = coin_field, duration = 30},
			{at = 1770, action = "hold", field = coin_field, duration = 30},
			{at = 1860, action = "snap", name = "POST_CREDIT"},
			{at = 1870, action = "hold", field = start_field, duration = 60},
			{at = 2050, action = "snap", name = "POST_START"},
		}

		local PLAY_START = 2060
		local PLAY_END = 2060 + 6000  -- ~70s de jeu pilote a ~85fps

		-- avance continue (perpendiculaire au balayage) sur toute la
		-- fenetre -- sans ca le joueur reste statique pres du point de
		-- depart et ne rencontre jamais les ennemis plus loin dans le
		-- niveau (confirme par capture reelle : tir visible mais aucun
		-- ennemi croise en ~90s sans avancer)
		if forward_field then
			table.insert(PHASES, {at = PLAY_START, action = "hold", field = forward_field,
				duration = PLAY_END - PLAY_START})
		end

		-- tir en tapotement (4 frames ON / 4 OFF) sur toute la fenetre
		local fire_t = PLAY_START
		while fire_t < PLAY_END do
			table.insert(PHASES, {at = fire_t, action = "hold", field = fire_field, duration = 4})
			fire_t = fire_t + 8
		end

		-- mouvement cyclique (alterne down_field/up_field toutes les
		-- 120 frames) sur toute la fenetre
		local CYCLE_FRAMES = 120
		local move_t = PLAY_START
		local toggle = true
		while move_t < PLAY_END do
			local dur = math.min(CYCLE_FRAMES, PLAY_END - move_t)
			table.insert(PHASES, {at = move_t, action = "hold",
				field = toggle and down_field or up_field, duration = dur})
			move_t = move_t + CYCLE_FRAMES
			toggle = not toggle
		end

		-- v22 - 2026-08-25 - safe-modify - retour utilisateur : le "RB
		-- Challenge" est en 1CC (1 seul credit) -- si le joueur utilise
		-- un continue, son score NE DOIT PAS etre retenu. Objectif :
		-- trouver l'adresse/flag "continue utilise" pour pouvoir
		-- invalider un score le cas echeant. Necessite un exemple
		-- POSITIF (continue reellement accepte) pour comparer au cas
		-- deja capture (credit_current reste inchange toute la partie
		-- -- confirme qu'aucun continue n'avait ete accepte jusqu'ici).
		-- Appui periodique sur START pendant toute la fenetre de jeu
		-- (accepte un continue s'il apparait -- generalement sans effet
		-- indesirable pendant le jeu normal sur la plupart des drivers).
		local start_t = PLAY_START
		while start_t < PLAY_END do
			table.insert(PHASES, {at = start_t, action = "hold", field = start_field, duration = 30})
			start_t = start_t + 170
		end

		-- snapshots reguliers tout du long (toutes les 500 frames, ~6s)
		-- pour pouvoir reperer a quel moment (s'il arrive) le score
		-- bouge reellement
		local snap_t = PLAY_START + 60
		local snap_i = 1
		while snap_t < PLAY_END do
			table.insert(PHASES, {at = snap_t, action = "snap", name = "PLAY_" .. snap_i})
			snap_t = snap_t + 500
			snap_i = snap_i + 1
		end

		table.sort(PHASES, function(a, b) return a.at < b.at end)
		snap_file:flush()
	end

	frame_subscription = emu.add_machine_frame_notifier(process_frame)
	emu.register_prestart(setup_for_new_machine)
end

return exports
