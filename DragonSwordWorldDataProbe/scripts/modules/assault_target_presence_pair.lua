local source = debug.getinfo(1, "S").source
local file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local module_dir = file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir = scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."

local targets = {
    {
        role = "special_night_candidate",
        place_id = 104,
        cid = 143,
        uid = "5945914773662957327",
        uid_name = "DSkeletonLeader_Nam_1041101",
        class_name = "DsMon_Skeleton_leader_Named02_C",
        x = 77563.0, y = 104532.0, z = 2688.0,
    },
    {
        role = "special_group_zero_peer",
        place_id = 109,
        cid = 148,
        uid = "3060543806338943140",
        uid_name = "DHound_Nam_1061201_Pat",
        class_name = "DsMon_Hound_Named02_C",
        x = 15325.0, y = 14193.0, z = 21767.0,
    },
    {
        role = "ordinary_reveal_control",
        place_id = 120,
        cid = 106,
        uid = "9492034567087927094",
        uid_name = "DGoblinBerserker_Nam_1040901_Treasure",
        class_name = "DsMon_Goblin_berserker_Named_C",
        x = 117793.0, y = 111674.0, z = 9570.0,
    },
}

local current_path = mod_dir .. "\\runtime\\reports\\assault-special-spawn-v4-current.tsv"
local history_path = mod_dir .. "\\runtime\\reports\\assault-special-spawn-v4-history.tsv"
local sequence = 0
local previous = {}
local safe_scalar

local headers = {
    "UTC", "Sequence", "Event", "Role", "PlaceID", "CID", "UID", "UIDName",
    "ClassName", "ActualClassName", "ClassActorCount", "MatchCount", "LocalActorPresent", "MatchMethod",
    "ActorUIDName", "ActorCID", "ActorX", "ActorY", "ActorZ", "DistanceXY",
    "GameSingletonPresent", "TimeOfDay", "LoginStartTimeOfDay",
    "SkyActorPresent", "SkyTimeOfDay", "SkyRealTimeOfDay",
    "WeatherManagerPresent", "CurrentWeatherState", "CurrentWeatherBTState",
    "UseCustomTime", "CustomEnvTime", "CustomEnvFxTime", "MultipleTime", "IsTeleport",
    "Interpretation",
}

local function clean(value)
    return tostring(value == nil and "" or value):gsub("\t", " "):gsub("[\r\n]", " ")
end

local function read_field(actor, names)
    for _, name in ipairs(names) do
        local ok, value = pcall(function() return actor[name] end)
        if ok and value ~= nil then return value end
    end
    return nil
end

local function read_location(actor)
    local location = nil
    pcall(function() location = actor:K2_GetActorLocation() end)
    if location == nil then pcall(function() location = actor:GetActorLocation() end) end
    if location == nil then return nil, nil, nil end
    return tonumber(location.X), tonumber(location.Y), tonumber(location.Z)
end

local function distance_xy(ax, ay, bx, by)
    if not ax or not ay then return nil end
    local dx, dy = ax - bx, ay - by
    return math.sqrt(dx * dx + dy * dy)
end

local function class_full_name(object)
    local value = ""
    pcall(function()
        local class = object:GetClass()
        if class ~= nil then value = class:GetFullName() end
    end)
    return tostring(value or "")
end

local function object_full_name(object)
    local value = ""
    if object ~= nil then pcall(function() value = object:GetFullName() end) end
    return tostring(value or "")
end

local function identity_actor(object)
    if object == nil then return nil end
    local owner = nil
    pcall(function() owner = object:GetOwner() end)
    if owner ~= nil then
        local valid_ok, valid = pcall(function() return owner:IsValid() end)
        if valid_ok and valid == true then return owner end
    end
    return object
end

local function read_lock_on_targets()
    local components = {}
    local ok, found = pcall(function() return FindAllOf("DsPCTargetingComponent") end)
    if ok and type(found) == "table" then components = found end
    local targets = {}
    for _, component in ipairs(components) do
        local valid_ok, valid = pcall(function() return component:IsValid() end)
        if valid_ok and valid == true then
            local target = read_field(component, { "LockOnTarget" })
            if target ~= nil then
                local target_valid_ok, target_valid = pcall(function() return target:IsValid() end)
                if target_valid_ok and target_valid == true then
                    targets[#targets + 1] = target
                end
            end
        end
    end
    return targets, #components
end

local function summarize_lock_on_target(target)
    if target == nil then
        return { class_name = "", uid_name = "", cid = "", x = "", y = "", z = "" }
    end
    local actor = identity_actor(target)
    local x, y, z = read_location(actor)
    return {
        source_class_name = class_full_name(target),
        class_name = class_full_name(actor),
        uid_name = safe_scalar(read_field(actor, { "UIDName", "UidName", "ActorUIDName" })),
        cid = safe_scalar(read_field(actor, { "CID", "ActorCID", "CharacterID" })),
        x = safe_scalar(x), y = safe_scalar(y), z = safe_scalar(z),
    }
end

local function read_weather_identity()
    local manager = FindFirstOf("DsEnvironmentManager")
    if manager == nil then return {} end
    local valid_ok, valid = pcall(function() return manager:IsValid() end)
    if not valid_ok or valid ~= true then return {} end
    local weather_actor = read_field(manager, { "DsWeatherActorBP" })
    local payload = read_field(manager, { "CurrentPayloadGroupData" })
    local default_fx = read_field(manager, { "DefaultFxData" })
    local force_weather = read_field(manager, { "ForceCustomWeather" })
    return {
        weather_actor_full_name = object_full_name(weather_actor),
        weather_actor_class = class_full_name(weather_actor),
        payload_full_name = object_full_name(payload),
        payload_class = class_full_name(payload),
        default_fx_full_name = object_full_name(default_fx),
        default_fx_class = class_full_name(default_fx),
        force_weather_full_name = object_full_name(force_weather),
        force_weather_class = class_full_name(force_weather),
    }
end

local function write_rows(path, rows, append)
    local exists = false
    if append then
        local check = io.open(path, "r")
        if check then exists = true; check:close() end
    end
    local handle = io.open(path, append and "a" or "w")
    if not handle then return false end
    if not exists then handle:write(table.concat(headers, "\t"), "\n") end
    for _, row in ipairs(rows) do
        local values = {}
        for _, header in ipairs(headers) do values[#values + 1] = clean(row[header]) end
        handle:write(table.concat(values, "\t"), "\n")
    end
    handle:close()
    return true
end

safe_scalar = function(value)
    local kind = type(value)
    if kind == "number" or kind == "string" or kind == "boolean" then
        return value
    end
    return ""
end

local function read_world_environment()
    local game = FindFirstOf("DGameSingleton")
    local game_present = false
    local time_of_day, login_start_time = "", ""
    if game ~= nil then
        local valid_ok, valid = pcall(function() return game:IsValid() end)
        if valid_ok and valid == true then
            game_present = true
            time_of_day = safe_scalar(read_field(game, { "TimeOfDay" }))
            login_start_time = safe_scalar(read_field(game, { "LoginStartTimeOfDay" }))
        end
    end

    local sky = FindFirstOf("DsSkyActor")
    if sky == nil then sky = FindFirstOf("DsNewSkyActor") end
    local sky_present = false
    local sky_time, sky_real_time = "", ""
    if sky ~= nil then
        local valid_ok, valid = pcall(function() return sky:IsValid() end)
        if valid_ok and valid == true then
            sky_present = true
            sky_time = safe_scalar(read_field(sky, { "TimeOfDay" }))
            sky_real_time = safe_scalar(read_field(sky, { "RealTimeOfDay" }))
        end
    end

    local manager = FindFirstOf("DsEnvironmentManager")
    if manager == nil then
        return game_present, time_of_day, login_start_time,
            sky_present, sky_time, sky_real_time,
            false, "", "", "", "", "", "", ""
    end
    local valid_ok, valid = pcall(function() return manager:IsValid() end)
    if not valid_ok or valid ~= true then
        return game_present, time_of_day, login_start_time,
            sky_present, sky_time, sky_real_time,
            false, "", "", "", "", "", "", ""
    end
    return game_present, time_of_day, login_start_time,
        sky_present, sky_time, sky_real_time,
        true,
        safe_scalar(read_field(manager, { "CurrentWeatherState" })),
        safe_scalar(read_field(manager, { "CurrentWeatherBTState" })),
        safe_scalar(read_field(manager, { "UseCustomTime" })),
        safe_scalar(read_field(manager, { "CustomEnvTime" })),
        safe_scalar(read_field(manager, { "CustomEnvFxTime" })),
        safe_scalar(read_field(manager, { "MultipleTime" })),
        safe_scalar(read_field(manager, { "IsTeleport" }))
end

local function scan_target(target, utc, environment, lock_on_targets)
    local actors = {}
    local ok, found = pcall(function() return FindAllOf(target.class_name) end)
    if ok and type(found) == "table" then actors = found end

    local matches = {}
    for _, actor in ipairs(actors) do
        local valid_ok, valid = pcall(function() return actor:IsValid() end)
        if valid_ok and valid == true then
            local actor_uid_name = read_field(actor, { "UIDName", "UidName", "ActorUIDName" })
            local actor_cid = tonumber(read_field(actor, { "CID", "ActorCID", "CharacterID" }))
            local x, y, z = read_location(actor)
            local dxy = distance_xy(x, y, target.x, target.y)
            local method = nil
            if tostring(actor_uid_name or "") == target.uid_name then
                method = "exact_uid_name"
            elseif actor_cid == target.cid and dxy and dxy <= 500 then
                method = "cid_and_static_xy"
            elseif dxy and dxy <= 200 then
                method = "exact_class_and_static_xy"
            end
            if method then
                matches[#matches + 1] = {
                    method = method, uid_name = actor_uid_name, cid = actor_cid,
                    x = x, y = y, z = z, distance_xy = dxy,
                    actual_class_name = class_full_name(actor),
                }
            end
        end
    end

    if #matches == 0 and target.role == "ordinary_reveal_control" then
        for _, lock_on_target in ipairs(lock_on_targets or {}) do
            local actor = identity_actor(lock_on_target)
            local actor_uid_name = read_field(actor, { "UIDName", "UidName", "ActorUIDName" })
            local actor_cid = tonumber(read_field(actor, { "CID", "ActorCID", "CharacterID" }))
            local x, y, z = read_location(actor)
            local dxy = distance_xy(x, y, target.x, target.y)
            if tostring(actor_uid_name or "") == target.uid_name
                or (actor_cid == target.cid and dxy and dxy <= 500)
            then
                matches[#matches + 1] = {
                    method = "lock_on_target_identity",
                    uid_name = actor_uid_name,
                    cid = actor_cid,
                    x = x, y = y, z = z, distance_xy = dxy,
                    actual_class_name = class_full_name(actor),
                }
                break
            end
        end
    end

    local match = #matches == 1 and matches[1] or nil
    local present = match ~= nil
    local key = target.role .. "|" .. target.uid
    local old = previous[key]
    local event = old == nil and "baseline" or (old ~= present and "changed" or "current")
    previous[key] = present

    return {
        UTC = utc, Sequence = sequence, Event = event, Role = target.role,
        PlaceID = target.place_id, CID = target.cid, UID = target.uid, UIDName = target.uid_name,
        ClassName = target.class_name,
        ActualClassName = match and match.actual_class_name or "",
        ClassActorCount = #actors, MatchCount = #matches,
        LocalActorPresent = present, MatchMethod = match and match.method or "",
        ActorUIDName = match and match.uid_name or "", ActorCID = match and match.cid or "",
        ActorX = match and match.x or "", ActorY = match and match.y or "",
        ActorZ = match and match.z or "", DistanceXY = match and match.distance_xy or "",
        GameSingletonPresent = environment.game_present,
        TimeOfDay = environment.time_of_day,
        LoginStartTimeOfDay = environment.login_start_time,
        SkyActorPresent = environment.sky_present,
        SkyTimeOfDay = environment.sky_time,
        SkyRealTimeOfDay = environment.sky_real_time,
        WeatherManagerPresent = environment.weather_present,
        CurrentWeatherState = environment.weather_state,
        CurrentWeatherBTState = environment.weather_bt_state,
        UseCustomTime = environment.use_custom_time,
        CustomEnvTime = environment.custom_env_time,
        CustomEnvFxTime = environment.custom_env_fx_time,
        MultipleTime = environment.multiple_time,
        IsTeleport = environment.is_teleport,
        Interpretation = present and "local_target_observed" or "not_observed_in_loaded_object_set",
    }, event ~= "current"
end

local function run(ctx)
    sequence = sequence + 1
    local utc = os.date("!%Y-%m-%dT%H:%M:%SZ")
    local game_present, time_of_day, login_start_time,
        sky_present, sky_time, sky_real_time,
        weather_present, weather_state, weather_bt_state,
        use_custom_time, custom_env_time, custom_env_fx_time,
        multiple_time, is_teleport = read_world_environment()
    local environment = {
        game_present = game_present,
        time_of_day = time_of_day,
        login_start_time = login_start_time,
        sky_present = sky_present,
        sky_time = sky_time,
        sky_real_time = sky_real_time,
        weather_present = weather_present,
        weather_state = weather_state,
        weather_bt_state = weather_bt_state,
        use_custom_time = use_custom_time,
        custom_env_time = custom_env_time,
        custom_env_fx_time = custom_env_fx_time,
        multiple_time = multiple_time,
        is_teleport = is_teleport,
    }
    local lock_on_targets, targeting_component_count = read_lock_on_targets()
    local lock_target_1 = summarize_lock_on_target(lock_on_targets[1])
    local lock_target_2 = summarize_lock_on_target(lock_on_targets[2])
    local lock_target_3 = summarize_lock_on_target(lock_on_targets[3])
    local rows, changes = {}, {}
    for _, target in ipairs(targets) do
        ctx.phase("scan_exact_class_place_" .. tostring(target.place_id))
        local row, changed = scan_target(target, utc, environment, lock_on_targets)
        rows[#rows + 1] = row
        if changed then changes[#changes + 1] = row end
    end
    write_rows(current_path, rows, false)
    if #changes > 0 then write_rows(history_path, changes, true) end

    ctx.log("ASSAULT_TARGET_PRESENCE_PAIR", {
        sequence = sequence,
        conditioned_present = rows[1].LocalActorPresent,
        control_present = rows[2].LocalActorPresent,
        ordinary_control_present = rows[3].LocalActorPresent,
        ordinary_control_match_method = rows[3].MatchMethod,
        ordinary_control_actual_class = rows[3].ActualClassName,
        targeting_component_count = targeting_component_count,
        nonempty_lock_on_target_count = #lock_on_targets,
        lock_target_1_class = lock_target_1.class_name,
        lock_target_1_source_class = lock_target_1.source_class_name,
        lock_target_1_uid_name = lock_target_1.uid_name,
        lock_target_1_cid = lock_target_1.cid,
        lock_target_1_x = lock_target_1.x,
        lock_target_1_y = lock_target_1.y,
        lock_target_1_z = lock_target_1.z,
        lock_target_2_class = lock_target_2.class_name,
        lock_target_2_source_class = lock_target_2.source_class_name,
        lock_target_2_uid_name = lock_target_2.uid_name,
        lock_target_2_cid = lock_target_2.cid,
        lock_target_3_class = lock_target_3.class_name,
        lock_target_3_source_class = lock_target_3.source_class_name,
        lock_target_3_uid_name = lock_target_3.uid_name,
        lock_target_3_cid = lock_target_3.cid,
        exact_classes = 3,
        global_character_scan = false,
        absence_semantics = "loaded_object_set_only",
        weather_manager_present = weather_present,
        current_weather_state = weather_state,
        current_weather_bt_state = weather_bt_state,
        time_of_day = time_of_day,
        sky_time_of_day = sky_time,
        is_teleport = is_teleport,
    })

    return {
        status = "ok",
        value_type = "presence_comparison",
        value = "conditioned=" .. tostring(rows[1].LocalActorPresent)
            .. ";group_zero_control=" .. tostring(rows[2].LocalActorPresent)
            .. ";ordinary_control=" .. tostring(rows[3].LocalActorPresent),
        fingerprint = "assault_pair:" .. tostring(rows[1].LocalActorPresent)
            .. ":" .. tostring(rows[2].LocalActorPresent)
            .. ":" .. tostring(rows[3].LocalActorPresent),
    }
end

return {
    id = "assault_target_presence_pair",
    enabled = true,
    description = "Bounded 10-second comparison of the two GroupID-zero Assault targets and ordinary PlaceID 120 control, using three exact generated classes only.",
    steps = {{
        kind = "custom",
        name = "assault_target_presence_pair",
        class = "<three exact generated monster classes>",
        role = "assault_target_availability_boundary",
        purpose = "loaded_object_presence_pair",
        cadence = 1,
        run = run,
    }},
}
