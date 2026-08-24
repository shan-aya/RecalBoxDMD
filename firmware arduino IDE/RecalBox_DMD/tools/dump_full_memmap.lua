-- dump_full_memmap.lua v1 -- 2026-08-24
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
