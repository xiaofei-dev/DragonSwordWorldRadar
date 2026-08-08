local source = debug.getinfo(1, "S").source
local file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local module_dir = file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir = scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."

local targets = {
    {
        role = "conditioned",
        place_id = 104,
        cid = 143,
        uid = "5945914773662957327",
        uid_name = "DSkeletonLeader_Nam_1041101",
        class_name = "DsMon_Skeleton_leader_Named02_C",
        x = 77563.0, y = 104532.0, z = 2688.0,
    },
    {
        role = "control",
        place_id = 126,
        cid = 110,
        uid = "15241393205769957563",
        uid_name = "DSkeletonCommander_Nam_Treasure_1022501",
        class_name = "DsMon_Skeleton_commander_Named01_C",
        x = 31753.0, y = 177642.0, z = 6276.0,
    },
}

local current_path = mod_dir .. "\\runtime\\reports\\assault-target-time-weather-v3-current.tsv"
local history_path = mod_dir .. "\\runtime\\reports\\assault-target-time-weather-v3-history.tsv"
local sequence = 0
local previous = {}

local headers = {
    "UTC", "Sequence", "Event", "Role", "PlaceID", "CID", "UID", "UIDName",
    "ClassName", "ClassActorCount", "MatchCount", "LocalActorPresent", "MatchMethod",
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

local function safe_scalar(value)
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

local function scan_target(target, utc, environment)
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
                }
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
        ClassName = target.class_name, ClassActorCount = #actors, MatchCount = #matches,
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
    local rows, changes = {}, {}
    for _, target in ipairs(targets) do
        ctx.phase("scan_exact_class_place_" .. tostring(target.place_id))
        local row, changed = scan_target(target, utc, environment)
        rows[#rows + 1] = row
        if changed then changes[#changes + 1] = row end
    end
    write_rows(current_path, rows, false)
    if #changes > 0 then write_rows(history_path, changes, true) end

    ctx.log("ASSAULT_TARGET_PRESENCE_PAIR", {
        sequence = sequence,
        conditioned_present = rows[1].LocalActorPresent,
        control_present = rows[2].LocalActorPresent,
        exact_classes = 2,
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
        value_type = "presence_pair",
        value = "conditioned=" .. tostring(rows[1].LocalActorPresent)
            .. ";control=" .. tostring(rows[2].LocalActorPresent),
        fingerprint = "assault_pair:" .. tostring(rows[1].LocalActorPresent)
            .. ":" .. tostring(rows[2].LocalActorPresent),
    }
end

return {
    id = "assault_target_presence_pair",
    enabled = true,
    description = "Bounded 10-second comparison of one conditioned Assault target and one control target using two exact generated classes only.",
    steps = {{
        kind = "custom",
        name = "assault_target_presence_pair",
        class = "<two exact generated monster classes>",
        role = "assault_target_availability_boundary",
        purpose = "loaded_object_presence_pair",
        cadence = 1,
        run = run,
    }},
}
