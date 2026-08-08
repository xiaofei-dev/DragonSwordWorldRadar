local config = dofile((debug.getinfo(1, "S").source:sub(2):match("^(.*)[/\\]modules[/\\][^/\\]+$") or ".") .. "\\config.lua")
local settings = config.module_settings and config.module_settings.class_presence or {}
local steps = {}
if config.safety.reflection == true then
    for index, target in ipairs(settings.targets or {}) do
        if type(target) == "string" and target ~= "" then
            steps[#steps + 1] = {
                name = "class_presence_" .. tostring(index), kind = "class", class = target,
                role = "configured_class", purpose = "explicit_whitelist_class_presence",
            }
        end
    end
end
return { id = "class_presence", enabled = #steps > 0, steps = steps }
