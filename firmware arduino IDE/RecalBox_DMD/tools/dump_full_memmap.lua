-- dump_full_memmap.lua v3 -- 2026-08-24
-- v3 : ajoute un 3e mode, "snapshot_diff" (gate LUA_HARVEST_MODE, v1/v2
-- ci-dessous INCHANGES et TOUJOURS executes, ce mode est un bloc
-- supplementaire en fin de fichier). Objectif : score/credits/continue/
-- vies/stage/position-joueur SONT en RAM (contrairement a difficulte/
-- nb-joueurs-configurable, deja couverts par le dump ioport v2) --
-- seule methode fiable = diff de snapshots RAM autour d'evenements
-- CONTROLES (credit, start), + suivi temporel pendant un dwell pour
-- distinguer score (monotone) de position (borne) d'un timer.
-- Sequence pilotee en FRAMES, entierement deterministe, PAS de
-- dependance a un declenchement reseau externe (retroarch) ni a une
-- correlation d'horloge murale -- API confirmee (pas de tatonnement,
-- contrairement au reste de ce fichier) en lisant le CODE SOURCE REEL
-- du plugin officiel MAME "autofire" (/usr/share/libretro-mame/
-- mame0278/plugins/autofire/init.lua) : emu.add_machine_frame_notifier
-- (callback par frame emulee) + field:set_value(1)/:clear_value()
-- (piloter un bouton directement, exactement ce que fait l'autofire
-- pour appuyer/relacher).
-- Demande utilisateur explicite : tester en priorite sur la liste des
-- 16 jeux "RB Challenge" (romsets resolus via gamelist.xml).
-- v2 : ajoute un 2e dump, les ioport (DIP switches / boutons) --
-- demande utilisateur ("difficulte du jeu, nombre de joueurs" sont
-- generalement des DIP, pas de la RAM -- lecture directe via l'API Lua
-- ioport, PAS de recherche en RAM pour ca). Noms de proprietes Lua
-- INCERTAINS (jamais testes) -- ecrit large (tous les candidats
-- plausibles en safe()) pour decouvrir empiriquement lesquels
-- resolvent, meme methode que pour la carte memoire ci-dessous.
-- Fichier separe (IOPORT_OUT) pour ne rien risquer sur le dump memmap
-- deja valide : toute la section est enveloppee en safe(), une
-- erreur ici ne peut pas casser le dump principal.
-- Enumere la carte memoire COMPLETE (tous les CPU/address spaces) d'un
-- jeu MAME via l'API Lua native (manager.machine.memory), independamment
-- de READ_CORE_RAM/RetroArch (qui ne couvre que ~30-50% des drivers --
-- voir DECISIONS.md "piste autoboot_script"). Pour chaque entree de la
-- carte d'adressage : plage d'adresses, type de handler lecture/ecriture
-- (rom/ram/delegate/none/...), nom de memory share si present, nom de
-- region si present, et -- tres informatif -- nom de la fonction C++
-- pour les handlers "delegate" (ex. "cps2_state::qsound_sharedram1_r"),
-- qui donne souvent un indice semantique direct sur le role de la zone
-- (objram, cps_a_regs, etc.) meme sans connaitre le jeu.
--
-- Ne filtre PAS par defaut sur handlertype=="ram" -- ecrit TOUTES les
-- entrees (le filtrage "RAM uniquement" se fait plus simplement cote
-- Python en aval, sur le fichier deja produit, sans avoir a relancer le
-- jeu si le filtre doit changer).
--
-- Format de sortie : une ligne par entree, champs separes par des
-- tabulations (simple a parser, pas besoin d'une lib JSON Lua) :
--   cpu_tag<TAB>space_name<TAB>addr_start<TAB>addr_end<TAB>read_type<TAB>read_name<TAB>write_type<TAB>write_name<TAB>share<TAB>region

local OUT_PATH = os.getenv("MEMMAP_OUT") or "/tmp/mame_lua_memmap.txt"

local function safe(fn)
    local ok, val = pcall(fn)
    if ok then return val else return nil end
end

local out = io.open(OUT_PATH, "w")
if not out then return end

local romname = safe(function() return emu.romname() end) or "?"
out:write("#game\t" .. romname .. "\n")

local devices = safe(function() return manager.machine.devices end)
if devices then
    for tag, dev in pairs(devices) do
        local spaces = safe(function() return dev.spaces end)
        if spaces then
            for spname, space in pairs(spaces) do
                local map = safe(function() return space.map end)
                local entries = map and safe(function() return map.entries end)
                if entries then
                    for _, entry in ipairs(entries) do
                        local astart = safe(function() return entry.address_start end) or -1
                        local aend = safe(function() return entry.address_end end) or -1
                        local share = safe(function() return entry.share end)
                        local region = safe(function() return entry.region end)
                        local rd = safe(function() return entry.read end)
                        local wr = safe(function() return entry.write end)
                        local rtype = rd and safe(function() return rd.handlertype end) or "?"
                        local rname = rd and safe(function() return rd.name end) or ""
                        local wtype = wr and safe(function() return wr.handlertype end) or "?"
                        local wname = wr and safe(function() return wr.name end) or ""
                        out:write(string.format(
                            "%s\t%s\t%08x\t%08x\t%s\t%s\t%s\t%s\t%s\t%s\n",
                            tag, spname, astart, aend,
                            tostring(rtype), tostring(rname or ""),
                            tostring(wtype), tostring(wname or ""),
                            tostring(share or ""), tostring(region or "")))
                    end
                end
            end
        end
    end
end

out:write("#done\n")
out:close()

-- === section ioport (DIP switches / boutons) -- v2 === --
local IOPORT_OUT = os.getenv("IOPORT_OUT") or "/tmp/mame_lua_ioports.txt"
local iout = io.open(IOPORT_OUT, "w")
if iout then
    iout:write("#game\t" .. romname .. "\n")
    local ports = safe(function() return manager.machine.ioport.ports end)
    if ports then
        for ptag, port in pairs(ports) do
            local fields = safe(function() return port.fields end)
            if fields then
                for fname, field in pairs(fields) do
                    local ftype = safe(function() return field.type end)
                    local mask = safe(function() return field.mask end)
                    local player = safe(function() return field.player end)
                    local defvalue = safe(function() return field.defvalue end)
                    local default_value = safe(function() return field.default_value end)
                    local sensitivity = safe(function() return field.sensitivity end)
                    local way = safe(function() return field.way end)
                    local live = safe(function() return field.live end)
                    local live_value = live and safe(function() return live.value end)
                    iout:write(string.format(
                        "FIELD\t%s\t%s\ttype=%s\tmask=%s\tplayer=%s\tdefvalue=%s\tdefault_value=%s\tsensitivity=%s\tway=%s\tlive_value=%s\n",
                        tostring(ptag), tostring(fname),
                        tostring(ftype), tostring(mask), tostring(player),
                        tostring(defvalue), tostring(default_value),
                        tostring(sensitivity), tostring(way), tostring(live_value)))

                    local settings = safe(function() return field.settings end)
                    if settings then
                        local ok_pairs = safe(function()
                            for sval, sname in pairs(settings) do
                                iout:write(string.format(
                                    "SETTING_PAIRS\t%s\t%s\t%s\t%s\n",
                                    tostring(ptag), tostring(fname), tostring(sval), tostring(sname)))
                            end
                            return true
                        end)
                        if not ok_pairs then
                            safe(function()
                                for _, setting in ipairs(settings) do
                                    local sname = safe(function() return setting.name end)
                                    local svalue = safe(function() return setting.value end)
                                    iout:write(string.format(
                                        "SETTING_IPAIRS\t%s\t%s\tvalue=%s\tname=%s\n",
                                        tostring(ptag), tostring(fname), tostring(svalue), tostring(sname)))
                                end
                            end)
                        end
                    end
                end
            end
        end
    end
    iout:write("#done\n")
    iout:close()
end

-- === MODE snapshot_diff -- v3 (bloc additionnel, n'affecte pas v1/v2 ci-dessus) === --
local MODE = os.getenv("LUA_HARVEST_MODE") or "memmap"
if MODE == "snapshot_diff" then
    local SNAP_OUT = os.getenv("SNAPSHOT_OUT") or "/tmp/mame_lua_snapshot.txt"
    local snap_file = io.open(SNAP_OUT, "w")
    if snap_file then
        snap_file:write("#game\t" .. romname .. "\n")

        -- 1) decouverte des zones RAM candidates : cote CPU principal
        -- uniquement (exclut audio/sound par tag), exclut par nom les
        -- shares clairement graphiques/son (heuristique de pre-tri,
        -- affinable), plafonne la taille par zone pour borner le cout
        -- d'un snapshot (lecture octet par octet en Lua).
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

        local MAX_ZONE_SIZE = 32768

        local zones = {}
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
                                                table.insert(zones, {tag=tag, space=space, spname=spname,
                                                    astart=astart, aend=aend, label=label})
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

        -- v3f -- cause racine probable trouvee : take_snapshot() lisait
        -- TOUTES les zones (jusqu'a ~4100 octets sur joemacr) en UNE
        -- SEULE frame, synchrone -- constat par elimination sur 5
        -- essais : le blocage survient systematiquement juste apres le
        -- 1er appel a take_snapshot(), QUELLE QUE SOIT la frame ou il
        -- est programme, MEME sans aucun press/release (dernier essai
        -- de controle). Suspicion : depasse le budget d'une frame
        -- (~16ms a 60fps), declenche une protection anti-blocage cote
        -- RetroArch/MAME qui coupe le notifier. Fix : etaler la lecture
        -- sur plusieurs frames (BYTES_PER_FRAME octets par frame, etat
        -- "pending" repris a chaque frame suivante) au lieu d'un bloc
        -- synchrone.
        local BYTES_PER_FRAME = 128
        local pending = nil  -- {phase=, zone_idx=, addr=, buffers={}}

        local function start_snapshot(phase)
            pending = {phase = phase, zone_idx = 1, addr = zones[1] and zones[1].astart or nil, buffers = {}}
            for i = 1, #zones do pending.buffers[i] = {} end
            if #zones == 0 then pending.zone_idx = 1 end
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

        -- 2) recherche des champs ioport credit/start par nom usuel --
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
        local coin_field = find_field(COIN_NAMES)
        local start_field = find_field(START_NAMES)
        snap_file:write(string.format("#coin_field_found\t%s\n", tostring(coin_field ~= nil)))
        snap_file:write(string.format("#start_field_found\t%s\n", tostring(start_field ~= nil)))
        if (not coin_field) or (not start_field) then
            -- diagnostic : liste tous les noms de champs trouves pour
            -- affiner COIN_NAMES/START_NAMES au prochain tour si besoin
            if ports2 then
                for ptag, port in pairs(ports2) do
                    local fields = safe(function() return port.fields end)
                    if fields then
                        for fname, _ in pairs(fields) do
                            snap_file:write(string.format("#fieldname\t%s\t%s\n", tostring(ptag), tostring(fname)))
                        end
                    end
                end
            end
        end

        -- 3) machine a etats pilotee par emu.add_machine_frame_notifier --
        local frame_count = 0
        -- v3d : sequence resserree (tout tient avant ~150 frames, le
        -- plafond observe) -- BOOT_SETTLE plus court (1s au lieu de 3s,
        -- risque assume : peut-etre pas assez pour certains jeux a
        -- intro longue, a re-elargir seulement si le plafond est
        -- resolu). Le dwell PLAY_1..8 est retire pour l'instant (aurait
        -- ajoute des ecritures/frames au-dela du plafond) -- objectif
        -- immediat = confirmer POST_CREDIT/POST_START, pas encore le
        -- suivi temporel du score.
        -- v3f -- calendrier complet retabli (lecture etalee maintenant
        -- -- chaque "at" est un plancher, la phase suivante attend que
        -- le pending precedent soit termine avant de se declencher,
        -- donc les frames reelles peuvent glisser un peu plus tard que
        -- prevu si une zone est grosse -- sans gravite pour notre usage).
        -- v3g -- DIAGNOSTIC : test de l'hypothese DRC (drc=0 force dans
        -- mame.ini pour cet essai) -- pas de press, juste des heartbeats
        -- et quelques snaps espaces pour voir si le plafond ~150 saute.
        local PHASES = {
            {at = 180, action = "snap", name = "T180"},
            {at = 300, action = "snap", name = "T300"},
            {at = 420, action = "snap", name = "T420"},
        }
        local next_idx = 1

        local function process_frame_inner()
            frame_count = frame_count + 1
            if frame_count % 30 == 0 and snap_file then
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
        end

        local function process_frame()
            -- diagnostic : capture le message d'erreur EXACT si le
            -- notifier plante (au lieu de laisser MAME l'avaler /
            -- desenregistrer silencieusement le notifier -- suspicion
            -- v3 initiale : le notifier pourrait etre efface par un
            -- reset interne du board apres le POST/self-test).
            local ok, err = pcall(process_frame_inner)
            if not ok and snap_file then
                snap_file:write(string.format("#FRAME_ERROR\t%d\t%s\n", frame_count, tostring(err)))
                snap_file:flush()
            end
        end

        -- v3c : cause racine trouvee -- heartbeat s'arretait net vers
        -- la frame 150-180 SANS erreur ni reset (2 hypotheses testees
        -- et ecartees par instrumentation directe : ni #FRAME_ERROR ni
        -- #machine_reset_seen ni #prestart_reregistered ne sont jamais
        -- apparus), alors que le jeu tournait normalement en verite
        -- (SCREENSHOT prise au moment fige : ecran BEST PLAYERS anime,
        -- pas de gel reel). Relecture du code source reel du plugin
        -- "autofire" : il stocke le retour de emu.add_machine_frame_
        -- notifier() dans une variable de PORTEE PLUGIN (jamais locale
        -- a une fonction) -- `frame_subscription = emu.add_machine_
        -- frame_notifier(process_frame)`. Notre appel precedent
        -- ignorait cette valeur de retour : le garbage collector Lua
        -- pouvait recuperer l'objet d'abonnement (plus aucune
        -- reference) des le premier cycle de GC, coupant le notifier
        -- silencieusement -- explique exactement le symptome observe.
        -- Fix : conserver la reference dans une variable qui survit
        -- (locale au bloc snapshot_diff, jamais reassignee a nil).
        local frame_subscription = emu.add_machine_frame_notifier(process_frame)
        if snap_file then
            snap_file:write(string.format("#notifier_registered\t%s\n", tostring(frame_subscription ~= nil)))
            snap_file:flush()
        end
    end
end
