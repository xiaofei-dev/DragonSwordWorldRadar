local source = debug.getinfo(1, "S").source
local scripts_dir = source:sub(2):match("^(.*)[/\\]modules[/\\][^/\\]+$") or "."
local config = dofile(scripts_dir .. "\\config.lua")
local settings = config.module_settings and config.module_settings.property_snapshot or {}
local steps = {}
if config.safety.property_reads == true then
    for index, target in ipairs(settings.targets or {}) do
        if type(target) == "table" and type(target.class) == "string" and type(target.path) == "table" then
            steps[#steps + 1] = {
                name = target.name or ("property_snapshot_" .. tostring(index)),
                kind = "property", class = target.class, path = target.path,
                role = target.role or "configured_property",
                purpose = target.purpose or "explicit_whitelist_property_snapshot",
                cadence = target.cadence or 1,
            }
        end
    end
end
return { id = "property_snapshot", enabled = #steps > 0, steps = steps }
