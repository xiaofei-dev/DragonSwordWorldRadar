local source = debug.getinfo(1, "S").source
local file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local module_dir = file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local common = dofile(scripts_dir .. "\\core\\treasure_state_common.lua")

local hooks = {
    "/Script/DS.DsAnimationProp:ReturnContentsPropState",
    "/Script/DS.DsAnimationProp:SetDeathProcess",
    "/Script/DS.DsAnimationProp:BP_SetDeathProcess",
    "/Script/DS.DsAnimationProp:NetMultiExecuteInteractProp",
    "/Script/DS.DInteractableComponent:ServerReturnInteract_EnableState",
    "/Script/DS.DInteractableComponent:Server_RunInteractV2",
    "/Script/DS.DInteractableComponent:Server_InputInteractKeyAction",
    "/Script/DS.DInteractableComponent:Server_InputInteractKeyAfterAction",
}

local function capture(path, phase, context)
    local ok, err = pcall(function()
        local row = common.snapshot(context)
        if not row then return end

        -- DsAnimationProp hooks are already narrow. Interactable hooks are kept
        -- only when the owner maps to a treasure or uses TreasureBox interact type.
        local is_prop_hook = path:find("DsAnimationProp", 1, true) ~= nil
        if row.TreasureCandidate == true or is_prop_hook then
            common.append_event(row, "hook", phase, path)
        end
    end)

    if not ok then
        -- Hook callbacks must never throw into UE4SS/native code.
        return tostring(err)
    end

    return ""
end

local function register_all(ctx)
    if type(RegisterHook) ~= "function" then
        return {
            status = "unsupported",
            value_type = "hook",
            value = "RegisterHook_unavailable",
            fingerprint = "treasure_hooks:unavailable",
        }
    end

    local registered = 0
    local failed = {}

    for _, path in ipairs(hooks) do
        if not common.shared.hook_registry[path] then
            ctx.phase("register_" .. path:gsub("[^A-Za-z0-9]", "_"))

            local pre = function(context, ...)
                capture(path, "pre", context)
            end

            local post = function(context, ...)
                capture(path, "post", context)
            end

            local ok, pre_id, post_id = pcall(function()
                return RegisterHook(path, pre, post)
            end)

            if ok then
                common.shared.hook_registry[path] = {
                    pre = pre_id,
                    post = post_id,
                }
                registered = registered + 1
            else
                failed[#failed + 1] = path .. "=" .. tostring(pre_id)
            end
        else
            registered = registered + 1
        end
    end

    ctx.log("TREASURE_STATE_HOOKS_REGISTERED", {
        requested = #hooks,
        registered = registered,
        failed = #failed,
        paths = table.concat(hooks, ";"),
        failures = table.concat(failed, " || "),
        mutation = false,
    })

    return {
        status = registered > 0 and "ok" or "unsupported",
        value_type = "hook_count",
        value = tostring(registered) .. "/" .. tostring(#hooks),
        fingerprint = "treasure_hooks:" .. tostring(registered) .. ":" .. table.concat(failed, "|"),
    }
end

return {
    id = "treasure_state_hooks",
    enabled = true,
    description = "Exact Treasure/Prop interaction observation hooks. Hooks observe game calls and never invoke the functions.",
    steps = {
        {
            kind = "custom",
            name = "register_treasure_state_hooks",
            class = "<exact UFunctions>",
            role = "treasure_state_events",
            purpose = "observation_only_hooks",
            cadence = 1,
            once_per_session = true,
            run = register_all,
        },
    },
}
