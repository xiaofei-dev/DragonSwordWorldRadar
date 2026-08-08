local source = debug.getinfo(1, "S").source
local scripts_dir = source:sub(2):match("^(.*)[/\\]modules[/\\][^/\\]+$") or "."
local config = dofile(scripts_dir .. "\\config.lua")
local settings = config.module_settings and config.module_settings.targeted_hook or {}
local registry = rawget(_G, "__DSWDP_TARGETED_HOOKS_V1")
if type(registry) ~= "table" then registry = {}; rawset(_G, "__DSWDP_TARGETED_HOOKS_V1", registry) end
local function denied(path)
    for _, token in ipairs(settings.denied_path_tokens or {}) do
        if tostring(path):find(tostring(token), 1, true) then return token end
    end
    return nil
end
local function object_name(value)
    if value == nil then return "nil" end
    local object = value
    pcall(function() object = value:get() end)
    local text = ""
    pcall(function() text = object:GetFullName() end)
    if text == "" then text = "<" .. type(object) .. ">" end
    return tostring(text)
end
local steps = {}
if config.safety.native_hooks == true then
    for index, hook in ipairs(settings.hooks or {}) do
        if type(hook) == "table" and type(hook.path) == "string" and not denied(hook.path) then
            steps[#steps+1] = {
                name=hook.name or ("targeted_hook_" .. tostring(index)), kind="custom",
                role="targeted_hook", purpose=hook.purpose or "explicit_whitelist_targeted_hook",
                once_per_session=true,
                run=function(ctx)
                    if registry[hook.path] then
                        return {status="readable", value_type="hook", value="already_registered", fingerprint="hook:"..hook.path}
                    end
                    if type(RegisterHook) ~= "function" then
                        return {status="unsupported", value_type="hook", value="RegisterHook_unavailable", fingerprint="hook:unavailable"}
                    end
                    ctx.phase("register_hook")
                    local callback = function(context, ...)
                        local args = {...}; local names = {}
                        for i=1, math.min(#args, tonumber(hook.max_arguments) or 8) do names[#names+1] = object_name(args[i]) end
                        ctx.log("TARGETED_HOOK_EVENT", { path=hook.path, context=object_name(context), argument_count=#args, arguments=table.concat(names, " || ") })
                    end
                    local ok, pre_id, post_id = pcall(function() return RegisterHook(hook.path, callback, function() end) end)
                    if not ok then return {status="call_failed", value_type="hook", value=tostring(pre_id), fingerprint="hook:failed:"..hook.path} end
                    registry[hook.path] = {pre=pre_id, post=post_id}
                    return {status="readable", value_type="hook", value="registered", fingerprint="hook:"..hook.path}
                end,
            }
        end
    end
end
return { id="targeted_hook", enabled=#steps > 0, steps=steps }
