local M = {}

local function clean_text(value, limit)
    local text = tostring(value == nil and "" or value):gsub("[\r\n\t]+", " ")
    limit = tonumber(limit) or 1200
    if #text > limit then return text:sub(1, limit) .. "..." end
    return text
end

local function safe_call(callback)
    local ok, value = pcall(callback)
    if ok then return true, value end
    return false, clean_text(value, 1000)
end

local function valid(object)
    if object == nil then return false end
    local ok, result = safe_call(function() return object:IsValid() end)
    return ok and result == true
end

local function encoded_summary(status, parts)
    local value = table.concat(parts or {}, ",")
    if value == "" then value = status end
    return {
        status = status,
        value_type = "world_boss_runtime",
        value = value,
        fingerprint = status .. ":" .. value,
    }
end

local function scalar_text(ok, value)
    if not ok then return tostring(value), "error" end
    if value == nil then return "nil", "nil" end
    local kind = type(value)
    if kind == "boolean" or kind == "number" or kind == "string" then
        return tostring(value), kind
    end
    return "<" .. kind .. ">", kind
end

local function bool_candidate(value)
    if type(value) == "boolean" then return value end
    local number = tonumber(value)
    return number ~= nil and number ~= 0
end

function M.new(catalog, matcher, options)
    options = options or {}
    if matcher == nil or type(matcher.identity) ~= "function" or type(matcher.match) ~= "function" then
        error("world_boss_runtime_requires_matcher")
    end

    local self = {
        catalog = catalog,
        matcher = matcher,
        cached = {},
        discovery_count = 0,
        max_objects = math.max(100, tonumber(options.max_objects) or 6000),
        max_unmapped_samples = math.max(0, tonumber(options.max_unmapped_samples) or 8),
    }

    local function cached_ids()
        local ids = {}
        local invalid_ids = {}
        for boss_id, entry in pairs(self.cached) do
            if valid(entry.actor) then
                ids[#ids + 1] = boss_id
            else
                invalid_ids[#invalid_ids + 1] = boss_id
            end
        end
        for _, boss_id in ipairs(invalid_ids) do self.cached[boss_id] = nil end
        table.sort(ids)
        return ids
    end

    local function match_score(entry)
        return entry and entry.match and tonumber(entry.match.score) or 0
    end

    local function match_distance(entry)
        local value = entry and entry.match and tonumber(entry.match.distance_xy) or nil
        return value or math.huge
    end

    local function stable_identity_key(entry)
        local identity = entry and entry.identity or {}
        return tostring(identity.full_name or "") .. "|" .. tostring(identity.name or "")
    end

    local function candidate_is_better(candidate, previous)
        if previous == nil or not valid(previous.actor) then return true end
        if previous.actor == candidate.actor then return true end
        local candidate_score = match_score(candidate)
        local previous_score = match_score(previous)
        if candidate_score ~= previous_score then return candidate_score > previous_score end
        local candidate_distance = match_distance(candidate)
        local previous_distance = match_distance(previous)
        if candidate_distance ~= previous_distance then return candidate_distance < previous_distance end
        local candidate_key = stable_identity_key(candidate)
        local previous_key = stable_identity_key(previous)
        if candidate_key ~= previous_key then return candidate_key < previous_key end
        return (candidate.discovered_index or math.huge)
            < (previous.discovered_index or math.huge)
    end

    local function distance_values(boss_id, identity, match)
        local dxy, dz = self.matcher:distance_for_boss(boss_id, identity)
        if dxy == nil then
            dxy = match and match.distance_xy or nil
            dz = match and match.distance_z or nil
        end
        return dxy, dz
    end

    local function actor_fields(entry, identity)
        identity = identity or (entry and entry.identity) or {}
        local match = entry and entry.match or {}
        local dxy, dz = distance_values(entry and entry.boss_id, identity, match)
        return {
            actor_name = identity.name,
            actor_full_name = identity.full_name,
            actor_class = identity.class_name,
            x = identity.x,
            y = identity.y,
            z = identity.z,
            location_status = identity.location_status,
            location_error = identity.location_error,
            match_mode = match.mode,
            match_token = match.token,
            distance_xy = dxy,
            distance_z = dz,
        }
    end

    local function merge_fields(target, source)
        for key, value in pairs(source or {}) do target[key] = value end
        return target
    end

    function self:discover(ctx)
        local safety = ctx.config and ctx.config.safety or {}
        if safety.targeted_find_all_of_character ~= true
            or safety.find_all_of ~= true
        then
            return encoded_summary("unsupported", { "FindAllOf=disabled_by_safety_policy" })
        end
        ctx.phase("find_all_of_character")
        ctx.log("NATIVE_CALL_BEGIN", {
            operation = "FindAllOf",
            class = "Character",
            purpose = "world_boss_actor_identity_and_spatial_discovery",
            max_objects = self.max_objects,
        })

        if type(FindAllOf) ~= "function" then
            return encoded_summary("unsupported", { "FindAllOf=unavailable" })
        end

        local ok_find, actors = safe_call(function() return FindAllOf("Character") end)
        if not ok_find then
            ctx.log("NATIVE_CALL_FAILED", {
                operation = "FindAllOf",
                class = "Character",
                error = actors,
            })
            return encoded_summary("call_failed", { "FindAllOf=" .. clean_text(actors, 300) })
        end

        local actor_count = type(actors) == "table" and #actors or 0
        ctx.log("NATIVE_CALL_RETURN", {
            operation = "FindAllOf",
            class = "Character",
            value_type = type(actors),
            actor_count = actor_count,
        })
        if type(actors) ~= "table" then
            return encoded_summary("unexpected_result", { "type=" .. type(actors) })
        end

        ctx.phase("match_character_identity_then_candidate_location")
        local next_cache = {}
        for boss_id, entry in pairs(self.cached) do
            if valid(entry.actor) then next_cache[boss_id] = entry end
        end

        local scanned = 0
        local valid_objects = 0
        local text_candidates = 0
        local noncandidate_skips = 0
        local location_reads = 0
        local location_errors = 0
        local matched_objects = 0
        local replaced_cache_entries = 0
        local identity_errors = 0
        local unmapped_fieldboss = 0
        local unmapped_logged = 0

        for index, actor in ipairs(actors) do
            if scanned >= self.max_objects then break end
            scanned = scanned + 1
            if valid(actor) then
                valid_objects = valid_objects + 1
                local identity_ok, identity = safe_call(function()
                    return self.matcher:identity(actor, { read_location = false })
                end)
                if not identity_ok or type(identity) ~= "table" then
                    identity_errors = identity_errors + 1
                else
                    local candidate_kind = self.matcher:candidate_kind(identity)
                    if candidate_kind == nil then
                        noncandidate_skips = noncandidate_skips + 1
                    else
                        text_candidates = text_candidates + 1
                        location_reads = location_reads + 1
                        local location_ok = self.matcher:populate_location(identity)
                        if not location_ok then location_errors = location_errors + 1 end

                        local match_ok, boss_id, match = pcall(function()
                            local id, info = self.matcher:match(identity)
                            return id, info
                        end)
                        if match_ok and boss_id ~= nil then
                            match = type(match) == "table" and match or {}
                            local candidate = {
                                boss_id = boss_id,
                                actor = actor,
                                identity = identity,
                                match = match,
                                discovered_index = index,
                                discovered_utc = os.date("!%Y-%m-%dT%H:%M:%SZ"),
                            }
                            local previous = next_cache[boss_id]
                            if candidate_is_better(candidate, previous) then
                                if previous ~= nil and previous.actor ~= actor then
                                    replaced_cache_entries = replaced_cache_entries + 1
                                end
                                next_cache[boss_id] = candidate
                            end
                            matched_objects = matched_objects + 1
                            ctx.actor_sample(merge_fields({
                                source = "FindAllOf(Character)",
                                sample = "identity_match",
                                candidate_kind = candidate_kind,
                                boss_id = boss_id,
                                status = "matched",
                                value_type = "loaded_actor",
                                value = "score=" .. tostring(match.score or 0),
                            }, actor_fields(candidate, identity)))
                        elseif match_ok and self.matcher:is_generic_fieldboss(identity) then
                            unmapped_fieldboss = unmapped_fieldboss + 1
                            if unmapped_logged < self.max_unmapped_samples then
                                unmapped_logged = unmapped_logged + 1
                                ctx.actor_sample({
                                    source = "FindAllOf(Character)",
                                    sample = "identity_candidate",
                                    candidate_kind = candidate_kind,
                                    boss_id = 0,
                                    actor_name = identity.name,
                                    actor_full_name = identity.full_name,
                                    actor_class = identity.class_name,
                                    x = identity.x, y = identity.y, z = identity.z,
                                    location_status = identity.location_status,
                                    location_error = identity.location_error,
                                    match_mode = type(match) == "table" and match.mode or "unmatched",
                                    match_token = type(match) == "table" and match.token or "",
                                    distance_xy = type(match) == "table" and match.distance_xy or nil,
                                    distance_z = type(match) == "table" and match.distance_z or nil,
                                    status = "generic_fieldboss_unmapped",
                                    value_type = "candidate",
                                    value = "not_cached",
                                })
                            end
                        elseif not match_ok then
                            identity_errors = identity_errors + 1
                        end
                    end
                end
            end
        end

        self.cached = next_cache
        self.discovery_count = self.discovery_count + 1
        local ids = cached_ids()
        ctx.log("WORLD_BOSS_ACTOR_DISCOVERY", {
            actor_count = actor_count,
            scanned = scanned,
            valid_objects = valid_objects,
            text_candidates = text_candidates,
            noncandidate_skips = noncandidate_skips,
            location_reads = location_reads,
            location_errors = location_errors,
            matched_objects = matched_objects,
            cached_boss_ids = table.concat(ids, ","),
            cached_count = #ids,
            replaced_cache_entries = replaced_cache_entries,
            identity_errors = identity_errors,
            unmapped_fieldboss = unmapped_fieldboss,
            discovery_count = self.discovery_count,
            staged_location_reads = true,
            exact_uid_or_semantic_spatial = true,
            absence_is_death = false,
        })
        return encoded_summary("readable", {
            "actors=" .. actor_count,
            "scanned=" .. scanned,
            "candidates=" .. text_candidates,
            "location_reads=" .. location_reads,
            "matched_objects=" .. matched_objects,
            "cached=" .. #ids,
            "ids=" .. table.concat(ids, ","),
            "unmapped_fieldboss=" .. unmapped_fieldboss,
        })
    end

    function self:identity_snapshot(ctx)
        local ids = cached_ids()
        if #ids == 0 then return encoded_summary("no_cached_actor", { "cached=0" }) end
        local parts = {}
        local removed = 0
        for _, boss_id in ipairs(ids) do
            local entry = self.cached[boss_id]
            local identity_ok, identity = safe_call(function()
                return self.matcher:identity(entry.actor)
            end)
            if not identity_ok or type(identity) ~= "table" then
                removed = removed + 1
                self.cached[boss_id] = nil
                ctx.actor_sample({
                    source = "cached_actor",
                    sample = "identity_refresh",
                    boss_id = boss_id,
                    status = "lua_error",
                    value_type = "identity",
                    value = clean_text(identity, 500),
                })
            else
                local rematch_ok, rematched_id, rematch = pcall(function()
                    local id, info = self.matcher:match(identity)
                    return id, info
                end)
                if rematch_ok and rematched_id == boss_id and type(rematch) == "table" then
                    entry.identity = identity
                    entry.match = rematch
                    ctx.actor_sample(merge_fields({
                        source = "cached_actor",
                        sample = "identity_refresh",
                        boss_id = boss_id,
                        status = "valid",
                        value_type = "identity",
                        value = "identity_refreshed",
                    }, actor_fields(entry, identity)))
                    parts[#parts + 1] = tostring(boss_id)
                        .. "@" .. tostring(identity.x or "?")
                        .. "," .. tostring(identity.y or "?")
                        .. ":" .. tostring(entry.match and entry.match.mode or "unknown")
                else
                    removed = removed + 1
                    self.cached[boss_id] = nil
                    ctx.actor_sample(merge_fields({
                        source = "cached_actor",
                        sample = "identity_refresh",
                        boss_id = boss_id,
                        status = rematch_ok and "identity_mismatch" or "lua_error",
                        value_type = "identity",
                        value = rematch_ok
                            and ("rematched_id=" .. tostring(rematched_id or "none"))
                            or clean_text(rematched_id, 500),
                    }, actor_fields(entry, identity)))
                end
            end
        end
        if #parts == 0 then
            return encoded_summary("no_cached_actor", { "removed=" .. removed })
        end
        parts[#parts + 1] = "removed=" .. removed
        return encoded_summary("readable", parts)
    end

    function self:sample_property(ctx, property_name)
        local safety = ctx.config and ctx.config.safety or {}
        if safety.property_reads ~= true then
            return encoded_summary("unsupported", { "property_reads=disabled_by_safety_policy" })
        end
        local ids = cached_ids()
        if #ids == 0 then return encoded_summary("no_cached_actor", { "property=" .. property_name }) end
        ctx.phase("actor_property_" .. property_name)
        local parts = {}
        for _, boss_id in ipairs(ids) do
            local entry = self.cached[boss_id]
            local ok, value = safe_call(function() return entry.actor[property_name] end)
            local value_text, kind = scalar_text(ok, value)
            local status = ok and (value == nil and "nil" or "readable") or "lua_error"
            ctx.actor_sample(merge_fields({
                source = "cached_actor",
                sample = "property",
                boss_id = boss_id,
                field = property_name,
                status = status,
                value_type = kind,
                value = clean_text(value_text, 500),
                dead_candidate = ok and bool_candidate(value) or false,
            }, actor_fields(entry, entry.identity)))
            parts[#parts + 1] = tostring(boss_id) .. "=" .. clean_text(value_text, 120)
        end
        return encoded_summary("readable", parts)
    end

    function self:sample_method(ctx, method_name)
        local safety = ctx.config and ctx.config.safety or {}
        if safety.exact_no_argument_method_calls ~= true then
            return encoded_summary("unsupported", { "method_calls=disabled_by_safety_policy" })
        end
        local ids = cached_ids()
        if #ids == 0 then return encoded_summary("no_cached_actor", { "method=" .. method_name }) end
        ctx.phase("actor_method_" .. method_name)
        local parts = {}
        for _, boss_id in ipairs(ids) do
            local entry = self.cached[boss_id]
            local lookup_ok, method = safe_call(function() return entry.actor[method_name] end)
            local call_ok, value = false, nil
            local status = "unsupported"
            if not lookup_ok then
                value = method
                status = "lua_error"
            elseif method == nil then
                value = "method_not_available"
                status = "unsupported"
            else
                call_ok, value = safe_call(function() return method(entry.actor) end)
                status = call_ok and (value == nil and "nil" or "readable") or "lua_error"
            end
            local value_text, kind = scalar_text(lookup_ok and (method == nil or call_ok), value)
            ctx.actor_sample(merge_fields({
                source = "cached_actor",
                sample = "method",
                boss_id = boss_id,
                field = method_name,
                status = status,
                value_type = method == nil and "method" or kind,
                value = clean_text(value_text, 500),
                dead_candidate = call_ok and bool_candidate(value) or false,
            }, actor_fields(entry, entry.identity)))
            parts[#parts + 1] = tostring(boss_id) .. "=" .. status .. ":" .. clean_text(value_text, 120)
        end
        return encoded_summary("readable", parts)
    end

    function self:sample_health_pair(ctx, current_name, maximum_name)
        local safety = ctx.config and ctx.config.safety or {}
        if safety.property_reads ~= true then
            return encoded_summary("unsupported", { "property_reads=disabled_by_safety_policy" })
        end
        local ids = cached_ids()
        if #ids == 0 then
            return encoded_summary("no_cached_actor", { "health=" .. current_name .. "/" .. maximum_name })
        end
        ctx.phase("actor_health_" .. current_name .. "_" .. maximum_name)
        local parts = {}
        for _, boss_id in ipairs(ids) do
            local entry = self.cached[boss_id]
            local ok_current, current = safe_call(function() return entry.actor[current_name] end)
            local ok_maximum, maximum = safe_call(function() return entry.actor[maximum_name] end)
            local current_number = ok_current and tonumber(current) or nil
            local maximum_number = ok_maximum and tonumber(maximum) or nil
            local dead_candidate = current_number ~= nil
                and maximum_number ~= nil
                and maximum_number > 0
                and current_number <= 0
            local value = tostring(current_number or "nil") .. "/" .. tostring(maximum_number or "nil")
            ctx.actor_sample(merge_fields({
                source = "cached_actor",
                sample = "health_pair",
                boss_id = boss_id,
                field = current_name .. "/" .. maximum_name,
                status = (ok_current and ok_maximum) and "readable" or "partial_or_error",
                value_type = "numeric_pair",
                value = value,
                dead_candidate = dead_candidate,
            }, actor_fields(entry, entry.identity)))
            parts[#parts + 1] = tostring(boss_id) .. "=" .. value .. ":dead=" .. tostring(dead_candidate)
        end
        return encoded_summary("readable", parts)
    end

    function self:steps()
        local steps = {
            {
                name = "discover_loaded_world_boss_actors",
                kind = "custom",
                role = "loaded_world_boss_actor",
                purpose = "map_static_boss_ids_to_loaded_character_objects_by_exact_uid_or_semantic_spatial_identity",
                cadence = math.max(1, tonumber(options.actor_discovery_cadence) or 1),
                run = function(ctx) return self:discover(ctx) end,
            },
            {
                name = "refresh_loaded_world_boss_identity",
                kind = "custom",
                role = "loaded_world_boss_actor",
                purpose = "record_name_class_location_and_match_evidence_for_cached_boss_actors",
                cadence = 1,
                run = function(ctx) return self:identity_snapshot(ctx) end,
            },
        }

        local boolean_properties = {
            "bIsDead", "IsDead", "bDead", "Dead",
            "bIsDeath", "IsDeath", "bDie", "IsDie",
        }
        for _, property_name in ipairs(boolean_properties) do
            local captured_property = property_name
            steps[#steps + 1] = {
                name = "boss_actor_property_" .. captured_property,
                kind = "custom",
                role = "boss_death_state_candidate",
                purpose = "read_one_exact_death_property_on_matched_boss_actors",
                run = function(ctx) return self:sample_property(ctx, captured_property) end,
            }
        end

        local methods = { "IsDead", "IsDying", "GetIsDead", "IsActorBeingDestroyed" }
        for _, method_name in ipairs(methods) do
            local captured_method = method_name
            steps[#steps + 1] = {
                name = "boss_actor_method_" .. captured_method,
                kind = "custom",
                role = "boss_death_state_candidate",
                purpose = "call_one_exact_no_argument_death_method_on_matched_boss_actors",
                run = function(ctx) return self:sample_method(ctx, captured_method) end,
            }
        end

        local pairs_to_sample = {
            { "CurrentHP", "MaxHP" },
            { "CurrentHp", "MaxHp" },
            { "CurrentHealth", "MaxHealth" },
            { "HP", "MaxHP" },
            { "Health", "MaxHealth" },
        }
        for _, pair in ipairs(pairs_to_sample) do
            local captured_current = pair[1]
            local captured_maximum = pair[2]
            steps[#steps + 1] = {
                name = "boss_actor_health_" .. captured_current .. "_" .. captured_maximum,
                kind = "custom",
                role = "boss_health_state_candidate",
                purpose = "read_one_exact_health_pair_on_matched_boss_actors",
                run = function(ctx)
                    return self:sample_health_pair(ctx, captured_current, captured_maximum)
                end,
            }
        end
        return steps
    end

    return self
end

return M
