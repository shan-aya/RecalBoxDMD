-- dump_full_memmap.lua v2 -- 2026-08-24
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
