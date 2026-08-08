local M = {}

local GLOBAL_REGISTRY_KEY = "__DSWDP_WORLD_BOSS_HOOK_REGISTRY_V1"

local function get_global_registry()
    local registry = rawget(_G, GLOBAL_REGISTRY_KEY)
    if type(registry) ~= "table" then
        registry = { entries = {} }
        rawset(_G, GLOBAL_REGISTRY_KEY, registry)
    elseif type(registry.entries) ~= "table" then
        registry.entries = {}
    end
    return registry
end

local function clean(value, limit)
    local text = tostring(value == nil and "" or value):gsub("[\r\n\t]+", " ")
    limit = tonumber(limit) or 1200
    if #text > limit then return text:sub(1, limit) .. "..." end
    return text
end

local function unwrap(context)
    if context == nil then return nil end
    local object = context
    pcall(function() object = context:get() end)
    return object
end

local function argument_summary(arguments, count)
    local parts = {}
    for index = 1, math.min(count, 8) do
        local value = arguments[index]
        local kind = type(value)
        if kind == "string" or kind == "number" or kind == "boolean" then
            parts[#parts + 1] = tostring(value)
        else
            local unwrapped = value
            pcall(function() unwrapped = value:get() end)
            local unwrapped_kind = type(unwrapped)
            if unwrapped_kind == "string"
                or unwrapped_kind == "number"
                or unwrapped_kind == "boolean"
            then
                parts[#parts + 1] = tostring(unwrapped)
            else
                parts[#parts + 1] = "<" .. unwrapped_kind .. ">"
            end
        end
    end
    return table.concat(parts, ",")
end

function M.new(catalog, matcher)
    if matcher == nil or type(matcher.identity) ~= "function" or type(matcher.match) ~= "function" then
        error("world_boss_lifecycle_requires_matcher")
    end

    local self = {
        catalog = catalog,
        matcher = matcher,
        logger = nil,
        callback_sequence = 0,
        hook_registry = get_global_registry(),
    }

    local function dispatch_event(service, event_name, context, ...)
        local argument_count = select("#", ...)
        local captured_arguments = { ... }
        local ok, err = xpcall(function()
            local object = unwrap(context)
            if object == nil then return end

            local identity = service.matcher:identity(object, { read_location = false })
            local candidate_kind = service.matcher:candidate_kind(identity)
            if candidate_kind == nil then return end
            service.matcher:populate_location(identity)
            local boss_id, match = service.matcher:match(identity)
            if boss_id == nil then
                if not service.matcher:is_generic_fieldboss(identity) then return end
                boss_id = 0
                match = type(match) == "table" and match or {}
                match.mode = match.mode == "unmatched"
                    and "generic_fieldboss_unmapped"
                    or (match.mode or "generic_fieldboss_unmapped")
            end
            match = type(match) == "table" and match or {}

            service.callback_sequence = service.callback_sequence + 1
            local fields = {
                event = event_name,
                boss_id = boss_id,
                candidate_kind = candidate_kind,
                identity_match = match.mode or "unknown",
                match_mode = match.mode or "unknown",
                match_token = match.token or "",
                distance_xy = match.distance_xy,
                distance_z = match.distance_z,
                actor_name = identity.name,
                actor_full_name = identity.full_name,
                actor_class = identity.class_name,
                x = identity.x,
                y = identity.y,
                z = identity.z,
                argument_count = argument_count,
                arguments = argument_summary(captured_arguments, argument_count),
            }
            if service.logger ~= nil then
                service.logger:lifecycle(fields)
                service.logger:write("WORLD_BOSS_LIFECYCLE_EVENT", {
                    sequence = service.callback_sequence,
                    event = event_name,
                    boss_id = boss_id,
                    candidate_kind = candidate_kind,
                    match_mode = fields.match_mode,
                    match_token = fields.match_token,
                    actor_name = identity.name,
                    actor_class = identity.class_name,
                    x = identity.x or "nil",
                    y = identity.y or "nil",
                    z = identity.z or "nil",
                    distance_xy = fields.distance_xy or "nil",
                    distance_z = fields.distance_z or "nil",
                    argument_count = argument_count,
                })
            end
        end, debug.traceback)
        if not ok and service.logger ~= nil then
            service.logger:error("WORLD_BOSS_LIFECYCLE_CALLBACK_ERROR", {
                event = event_name,
                error = service.logger:trace(err),
            })
        end
    end

    local function callback_for(event_name, hook_path)
        return function(context, ...)
            local entry = self.hook_registry.entries[hook_path]
            local target = type(entry) == "table" and entry.target or nil
            if target == nil then return end
            dispatch_event(target, event_name, context, ...)
        end
    end

    function self:install_one(ctx, hook)
        self.logger = ctx.logger
        local safety = ctx.config and ctx.config.safety or {}
        if safety.passive_actor_lifecycle_hooks ~= true
            or safety.native_hooks ~= true
        then
            return {
                status = "unsupported",
                value_type = "hook_installation",
                value = hook.name .. "=disabled_by_safety_policy",
                fingerprint = "hook:" .. hook.name .. ":disabled_by_safety_policy",
            }
        end

        local entry = self.hook_registry.entries[hook.path]
        if type(entry) ~= "table" then
            entry = { installed = false, registering = false }
            self.hook_registry.entries[hook.path] = entry
        end
        entry.target = self
        entry.event_name = hook.name

        if entry.installed == true then
            ctx.log("WORLD_BOSS_HOOK_REUSED", {
                hook = hook.name,
                path = hook.path,
                pre_id = entry.pre_id or "nil",
                post_id = entry.post_id or "nil",
            })
            return {
                status = "readable",
                value_type = "hook_installation",
                value = hook.name .. "=already_installed_process_session",
                fingerprint = "hook:" .. hook.name .. ":installed",
            }
        end
        if entry.registering == true then
            return {
                status = "readable",
                value_type = "hook_installation",
                value = hook.name .. "=registration_in_progress",
                fingerprint = "hook:" .. hook.name .. ":registering",
            }
        end

        if type(RegisterHook) ~= "function" then
            return {
                status = "unsupported",
                value_type = "hook_installation",
                value = hook.name .. "=RegisterHook_unavailable",
                fingerprint = "hook:" .. hook.name .. ":RegisterHook_unavailable",
            }
        end

        ctx.phase("register_hook_" .. hook.name)
        entry.registering = true
        local ok, pre_id, post_id = pcall(function()
            local registered_pre, registered_post = RegisterHook(
                hook.path,
                callback_for(hook.name, hook.path),
                function() end
            )
            return registered_pre, registered_post
        end)
        entry.registering = false

        if ok then
            entry.installed = true
            entry.pre_id = pre_id
            entry.post_id = post_id
            ctx.log("WORLD_BOSS_HOOK_REGISTERED", {
                hook = hook.name,
                path = hook.path,
                pre_id = pre_id or "nil",
                post_id = post_id or "nil",
            })
            return {
                status = "readable",
                value_type = "hook_installation",
                value = hook.name .. "=registered",
                fingerprint = "hook:" .. hook.name .. ":registered",
            }
        end

        entry.installed = false
        ctx.log("WORLD_BOSS_HOOK_REGISTRATION_FAILED", {
            hook = hook.name,
            path = hook.path,
            error = clean(pre_id, 1000),
        })
        return {
            status = "call_failed",
            value_type = "hook_installation",
            value = hook.name .. "=failed:" .. clean(pre_id, 400),
            fingerprint = "hook:" .. hook.name .. ":failed:" .. clean(pre_id, 400),
        }
    end

    function self:steps()
        local hooks = {
            { name = "ReceiveDestroyed", path = "/Script/Engine.Actor:ReceiveDestroyed" },
            { name = "ReceiveEndPlay", path = "/Script/Engine.Actor:ReceiveEndPlay" },
        }
        local steps = {}
        for _, definition in ipairs(hooks) do
            local captured = definition
            steps[#steps + 1] = {
                name = "install_world_boss_hook_" .. captured.name,
                kind = "custom",
                role = "boss_kill_lifecycle_event",
                purpose = "passively_observe_static_uid_or_semantic_spatial_boss_actor_" .. captured.name,
                cadence = 1,
                run = function(ctx) return self:install_one(ctx, captured) end,
            }
        end
        return steps
    end

    return self
end

return M
