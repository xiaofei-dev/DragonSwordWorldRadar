-- Stable UE4SS module name for the generated treasure dataset.
-- The installer writes data/generated/treasures.lua from the local game PAK.
local source = debug.getinfo(1, "S").source
if type(source) ~= "string" or string.sub(source, 1, 1) ~= "@" then
    error("DragonSwordWorldRadar could not resolve the treasure dataset path")
end
local scripts_directory = string.match(string.sub(source, 2), "^(.*)[/\\][^/\\]+$")
local mod_directory = scripts_directory
    and string.match(scripts_directory, "^(.*)[/\\][^/\\]+$")
if mod_directory == nil then
    error("DragonSwordWorldRadar could not resolve the mod directory")
end
return dofile(mod_directory .. "\\data\\generated\\treasures.lua")
