-- Stable UE4SS loader for the install-generated Mole NPC/trigger catalog.
-- Install.cmd extracts the current game's Mole data from the local PAK and
-- writes data/generated/moles.lua. Runtime code never hardcodes coordinates.
local source = debug.getinfo(1, "S").source
if type(source) ~= "string" or string.sub(source, 1, 1) ~= "@" then
    error("DragonSwordWorldRadar could not resolve the Mole dataset path")
end

local scripts_directory = string.match(
    string.sub(source, 2),
    "^(.*)[/\\][^/\\]+$"
)
local mod_directory = scripts_directory
    and string.match(scripts_directory, "^(.*)[/\\][^/\\]+$")
if mod_directory == nil then
    error("DragonSwordWorldRadar could not resolve the Mod directory")
end

return dofile(mod_directory .. "\\data\\generated\\moles.lua")
