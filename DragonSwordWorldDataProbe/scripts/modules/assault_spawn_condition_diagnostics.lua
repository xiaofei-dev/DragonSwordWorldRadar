local source = debug.getinfo(1, "S").source
local file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local module_dir = file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir = scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."

local targets = {
    { role = "night_candidate", place_id = 104, cid = 143, uid = "5945914773662957327", x = 77563.0, y = 104532.0, z = 2688.0 },
    { role = "group_zero_peer", place_id = 109, cid = 148, uid = "3060543806338943140", x = 15325.0, y = 14193.0, z = 21767.0 },
    { role = "ordinary_control", place_id = 120, cid = 106, uid = "9492034567087927094", x = 117793.0, y = 111674.0, z = 9570.0 },
}

local current_path = mod_dir .. "\\runtime\\reports\\assault-spawn-condition-current.tsv"
local history_path = mod_dir .. "\\runtime\\reports\\assault-spawn-condition-history.tsv"
local discovery_path = mod_dir .. "\\runtime\\reports\\assault-spawn-condition-discovery.tsv"
local cached = {}
local previous_fingerprint = {}
local sequence = 0
local last_discovery_sequence = -1000

local headers = {
    "UTC", "Sequence", "Event", "Role", "PlaceID", "CID", "UID",
    "SpawnerFound", "SpawnerClass", "SpawnerObject", "SpawnerX", "SpawnerY", "SpawnerZ", "DistanceXY", "DistanceZ",
    "MonsterKey", "LevelSettingsKey", "SpawnConditionKey", "RespawnCycleKey", "RevealCycleKey",
    "GroupID", "HPPercentOnSpawn", "ServerCheckDeath", "RegisterToETManager", "RespawnCycleSet",
    "IgnoreCosmeticsHandle", "DeathCheckTag", "DisposableActor", "RoamingFarDistance",
    "InitialSpawnInGround", "RespawnInGround", "InitialSpawnLocationType", "RespawnLocationType",
    "TimeOfDay", "CurrentWeatherState", "CurrentWeatherBTState", "Interpretation",
}

local discovery_headers = {
    "UTC", "Sequence", "SpawnerIndex", "Valid", "SpawnerClass", "SpawnerObject",
    "SpawnerX", "SpawnerY", "SpawnerZ", "NearestRole", "NearestPlaceID", "NearestDistanceXY",
    "MonsterKey", "SpawnConditionKey", "RespawnCycleKey", "RevealCycleKey", "GroupID",
}

local function clean(value)
    return tostring(value == nil and "" or value):gsub("\t", " "):gsub("[\r\n]", " ")
end

local function write_rows(path, row_headers, rows, append)
    local exists = false
    if append then
        local check = io.open(path, "r")
        if check then exists = true; check:close() end
    end
    local handle = io.open(path, append and "a" or "w")
    if not handle then return false end
    if not exists then handle:write(table.concat(row_headers, "\t"), "\n") end
    for _, row in ipairs(rows) do
        local values = {}
        for _, header in ipairs(row_headers) do values[#values + 1] = clean(row[header]) end
        handle:write(table.concat(values, "\t"), "\n")
    end
    handle:close()
    return true
end

local function read_field(object, name)
    local ok, value = pcall(function() return object[name] end)
    if ok then return value, true end
    return nil, false
end

local function scalar(value)
    local kind = type(value)
    if kind == "number" or kind == "string" or kind == "boolean" then return value end
    return ""
end

local function read_key(object, property_name)
    local wrapper, property_ok = read_field(object, property_name)
    if not property_ok or wrapper == nil then return "", false end
    local key, key_ok = read_field(wrapper, "Key")
    if not key_ok then return "", false end
    local numeric = tonumber(key)
    if numeric == nil then return "", false end
    return numeric, true
end

local function valid(object)
    if object == nil then return false end
    local ok, value = pcall(function() return object:IsValid() end)
    return ok and value == true
end

local function location(object)
    local value = nil
    pcall(function() value = object:K2_GetActorLocation() end)
    if value == nil then return nil, nil, nil end
    return tonumber(value.X), tonumber(value.Y), tonumber(value.Z)
end

local function full_name(object)
    local value = ""
    if object ~= nil then pcall(function() value = object:GetFullName() end) end
    return type(value) == "string" and value or ""
end

local function class_name(object)
    local value = ""
    if object ~= nil then
        pcall(function()
            local class = object:GetClass()
            if class ~= nil then value = class:GetFullName() end
        end)
    end
    return type(value) == "string" and value or ""
end

local function distance_xy(x1, y1, x2, y2)
    if not x1 or not y1 then return nil end
    local dx, dy = x1 - x2, y1 - y2
    return math.sqrt(dx * dx + dy * dy)
end

local function nearest_target(x, y)
    local best, best_distance = nil, nil
    for _, target in ipairs(targets) do
        local distance = distance_xy(x, y, target.x, target.y)
        if distance and (best_distance == nil or distance < best_distance) then
            best, best_distance = target, distance
        end
    end
    return best, best_distance
end

local function environment()
    local time_of_day, weather_state, weather_bt_state = "", "", ""
    local game = FindFirstOf("DGameSingleton")
    if valid(game) then time_of_day = scalar((read_field(game, "TimeOfDay"))) end
    local manager = FindFirstOf("DsEnvironmentManager")
    if valid(manager) then
        weather_state = scalar((read_field(manager, "CurrentWeatherState")))
        weather_bt_state = scalar((read_field(manager, "CurrentWeatherBTState")))
    end
    return time_of_day, weather_state, weather_bt_state
end

local function discover(utc)
    local objects = {}
    local ok, found = pcall(function() return FindAllOf("MonsterSpawnBase") end)
    if ok and type(found) == "table" then objects = found end
    local rows = {}
    local best = {}
    for index, object in ipairs(objects) do
        local is_valid = valid(object)
        local x, y, z = nil, nil, nil
        if is_valid then x, y, z = location(object) end
        local nearest, nearest_distance = nearest_target(x, y)
        local row = {
            UTC = utc, Sequence = sequence, SpawnerIndex = index, Valid = is_valid,
            SpawnerClass = is_valid and class_name(object) or "",
            SpawnerObject = is_valid and full_name(object) or "",
            SpawnerX = x or "", SpawnerY = y or "", SpawnerZ = z or "",
            NearestRole = nearest and nearest.role or "", NearestPlaceID = nearest and nearest.place_id or "",
            NearestDistanceXY = nearest_distance or "",
            MonsterKey = is_valid and select(1, read_key(object, "TableKey_Monster")) or "",
            SpawnConditionKey = is_valid and select(1, read_key(object, "TableKey_SpawnCondition")) or "",
            RespawnCycleKey = is_valid and select(1, read_key(object, "TableKey_RespawnCycle")) or "",
            RevealCycleKey = is_valid and select(1, read_key(object, "TableKey_RevealCycle")) or "",
            GroupID = is_valid and scalar((read_field(object, "GroupID"))) or "",
        }
        rows[#rows + 1] = row
        if is_valid and nearest and nearest_distance and nearest_distance <= 1500 then
            local old = best[nearest.role]
            if old == nil or nearest_distance < old.distance then
                best[nearest.role] = { object = object, distance = nearest_distance }
            end
        end
    end
    cached = best
    write_rows(discovery_path, discovery_headers, rows, false)
    return #objects, #rows
end

local function cached_valid()
    local count = 0
    for _, entry in pairs(cached) do
        if not valid(entry.object) then return false end
        count = count + 1
    end
    return count == #targets
end

local function make_row(target, utc, time_of_day, weather_state, weather_bt_state)
    local entry = cached[target.role]
    local object = entry and entry.object or nil
    local found = valid(object)
    local x, y, z = nil, nil, nil
    if found then x, y, z = location(object) end
    local respawn_data = found and select(1, read_field(object, "RespawnCycleData")) or nil
    local row = {
        UTC = utc, Sequence = sequence, Role = target.role, PlaceID = target.place_id, CID = target.cid, UID = target.uid,
        SpawnerFound = found, SpawnerClass = found and class_name(object) or "", SpawnerObject = found and full_name(object) or "",
        SpawnerX = x or "", SpawnerY = y or "", SpawnerZ = z or "",
        DistanceXY = found and distance_xy(x, y, target.x, target.y) or "",
        DistanceZ = found and z and math.abs(z - target.z) or "",
        MonsterKey = found and select(1, read_key(object, "TableKey_Monster")) or "",
        LevelSettingsKey = found and select(1, read_key(object, "TableKey_LevelSettings")) or "",
        SpawnConditionKey = found and select(1, read_key(object, "TableKey_SpawnCondition")) or "",
        RespawnCycleKey = found and select(1, read_key(object, "TableKey_RespawnCycle")) or "",
        RevealCycleKey = found and select(1, read_key(object, "TableKey_RevealCycle")) or "",
        GroupID = found and scalar((read_field(object, "GroupID"))) or "",
        HPPercentOnSpawn = found and scalar((read_field(object, "HPPercentOnSpawn"))) or "",
        ServerCheckDeath = found and scalar((read_field(object, "bServerCheckDeath"))) or "",
        RegisterToETManager = found and scalar((read_field(object, "bRegisterToETManager"))) or "",
        RespawnCycleSet = found and scalar((read_field(object, "RespawnCycleSet"))) or "",
        IgnoreCosmeticsHandle = found and scalar((read_field(object, "bIgnoreCosmeticsHandle"))) or "",
        DeathCheckTag = found and scalar((read_field(object, "bIsDeathCheckTag"))) or "",
        DisposableActor = found and scalar((read_field(object, "bIsDisposableActor"))) or "",
        RoamingFarDistance = found and scalar((read_field(object, "bRoamingFarDistance"))) or "",
        InitialSpawnInGround = respawn_data and scalar((read_field(respawn_data, "InitialSpawnInGround"))) or "",
        RespawnInGround = respawn_data and scalar((read_field(respawn_data, "RespawnInGround"))) or "",
        InitialSpawnLocationType = respawn_data and scalar((read_field(respawn_data, "InitialSpawnLocationType"))) or "",
        RespawnLocationType = respawn_data and scalar((read_field(respawn_data, "RespawnLocationType"))) or "",
        TimeOfDay = time_of_day, CurrentWeatherState = weather_state, CurrentWeatherBTState = weather_bt_state,
        Interpretation = found and "spawner_bound_read_only" or "target_spawner_not_loaded_or_not_within_1500uu",
    }
    local fingerprint = table.concat({ tostring(found), tostring(row.MonsterKey), tostring(row.SpawnConditionKey),
        tostring(row.RespawnCycleKey), tostring(row.RevealCycleKey), tostring(row.RespawnCycleSet) }, ":")
    local old = previous_fingerprint[target.role]
    row.Event = old == nil and "baseline" or (old ~= fingerprint and "changed" or "current")
    previous_fingerprint[target.role] = fingerprint
    return row
end

local function run(ctx)
    sequence = sequence + 1
    local utc = os.date("!%Y-%m-%dT%H:%M:%SZ")
    local enumerated = 0
    if not cached_valid() and sequence - last_discovery_sequence >= 6 then
        ctx.phase("discover_monster_spawn_base_once")
        enumerated = select(1, discover(utc))
        last_discovery_sequence = sequence
    end
    local time_of_day, weather_state, weather_bt_state = environment()
    local rows, changes = {}, {}
    for _, target in ipairs(targets) do
        local row = make_row(target, utc, time_of_day, weather_state, weather_bt_state)
        rows[#rows + 1] = row
        if row.Event ~= "current" then changes[#changes + 1] = row end
    end
    write_rows(current_path, headers, rows, false)
    if #changes > 0 then write_rows(history_path, headers, changes, true) end
    ctx.log("ASSAULT_SPAWN_CONDITION_DIAGNOSTICS", {
        sequence = sequence, enumerated_spawners = enumerated,
        night_candidate_found = rows[1].SpawnerFound, night_reveal_cycle_key = rows[1].RevealCycleKey,
        night_spawn_condition_key = rows[1].SpawnConditionKey,
        peer_found = rows[2].SpawnerFound, peer_reveal_cycle_key = rows[2].RevealCycleKey,
        ordinary_found = rows[3].SpawnerFound, ordinary_reveal_cycle_key = rows[3].RevealCycleKey,
        time_of_day = time_of_day, global_actor_scan = false, global_character_scan = false,
        unknown_function_calls = 0,
    })
    return {
        status = "ok", value_type = "spawn_condition_diagnostics",
        value = "night=" .. tostring(rows[1].RevealCycleKey) .. ";ordinary=" .. tostring(rows[3].RevealCycleKey),
        fingerprint = "spawn_condition:" .. tostring(rows[1].RevealCycleKey) .. ":" .. tostring(rows[3].RevealCycleKey),
    }
end

return {
    id = "assault_spawn_condition_diagnostics",
    enabled = true,
    description = "Bounded read-only binding of known Assault targets to fixed MonsterSpawnBase actors and their exact condition table keys.",
    steps = {{
        kind = "custom", name = "assault_spawn_condition_diagnostics", class = "MonsterSpawnBase",
        role = "assault_spawn_condition_binding", purpose = "read_exact_condition_keys", cadence = 1, run = run,
    }},
}
