local M = {}

local GLOBAL_KEY = "__DSWDP_BOSS_KILL_PROBE_V1"

local function clean(value, limit)
    local text = tostring(value == nil and "" or value):gsub("[\r\n\t]+", " ")
    limit = tonumber(limit) or 1200
    if #text > limit then return text:sub(1, limit) .. "..." end
    return text
end

local function valid(object)
    if object == nil then return false end
    local ok, result = pcall(function() return object:IsValid() end)
    return ok and result == true
end

local function full_name(object)
    if object == nil then return "" end
    local value = ""
    pcall(function() value = object:GetFullName() end)
    if value == "" then pcall(function() value = object:GetName() end) end
    return clean(value, 1000)
end

local function class_name(object)
    if object == nil then return "" end
    local value = ""
    pcall(function()
        local cls = object:GetClass()
        if cls ~= nil then value = cls:GetFullName() end
    end)
    return clean(value, 700)
end

local function unwrap_object(value)
    if valid(value) then return value end
    if value ~= nil then
        local ok, got = pcall(function() return value:get() end)
        if ok and valid(got) then return got end
    end
    return nil
end

local function read_candidate(object, name)
    if object == nil then return nil end
    local ok, value = pcall(function() return object[name] end)
    if not ok or value == nil then return nil end
    local kind = type(value)
    if kind == "string" or kind == "number" or kind == "boolean" then
        return tostring(value)
    end
    local ok_get, got = pcall(function() return value:get() end)
    if ok_get then
        local got_kind = type(got)
        if got_kind == "string" or got_kind == "number" or got_kind == "boolean" then
            return tostring(got)
        end
    end
    if valid(value) then return full_name(value) end
    if ok_get and valid(got) then return full_name(got) end
    return nil
end

local function json_escape(value)
    local s = tostring(value or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\r", "\\r")
    s = s:gsub("\n", "\\n")
    s = s:gsub("\t", "\\t")
    return s
end

local function atomic_write(path, text)
    local temp = path .. ".tmp"
    local f, err = io.open(temp, "w")
    if not f then return false, err or "open_failed" end
    local ok, write_err = pcall(function()
        f:write(text)
        f:flush()
    end)
    pcall(function() f:close() end)
    if not ok then
        os.remove(temp)
        return false, write_err or "write_failed"
    end
    local backup = path .. ".bak"
    os.remove(backup)
    local moved = os.rename(path, backup)
    local renamed, rename_err = os.rename(temp, path)
    if not renamed then
        if moved then os.rename(backup, path) end
        os.remove(temp)
        return false, rename_err or "rename_failed"
    end
    if moved then os.remove(backup) end
    return true
end

local function next_daily_reset(hour, minute)
    local now = os.time()
    local t = os.date("*t", now)
    t.hour = tonumber(hour) or 9
    t.min = tonumber(minute) or 0
    t.sec = 0
    local reset = os.time(t)
    if reset <= now then
        reset = reset + 24 * 60 * 60
    end
    return reset
end

local function identity_text(object)
    if object == nil then return "" end
    local parts = { full_name(object), class_name(object) }
    local fields = {
        "UIDName", "UidName", "ActorUIDName", "SpawnUIDName",
        "UID", "Uid", "ActorUID", "SpawnActorUID", "SectionUID",
        "CID", "Cid", "ActorCID", "MonsterCID", "LevelCID",
        "GroupID", "GroupId", "TableKey", "Name"
    }
    for _, field in ipairs(fields) do
        local value = read_candidate(object, field)
        if value and value ~= "" then parts[#parts + 1] = field .. "=" .. value end
    end
    return table.concat(parts, " | ")
end

function M.new(catalog, options, mod_dir)
    options = options or {}
    local registry = rawget(_G, GLOBAL_KEY)
    if type(registry) ~= "table" then
        registry = { installed = false, target = nil }
        rawset(_G, GLOBAL_KEY, registry)
    end

    local self = {
        catalog = catalog,
        options = options,
        mod_dir = mod_dir,
        registry = registry,
        logger = nil,
        completed = {},
        sequence = 0,
    }

    self.cache_path = mod_dir .. "\\" .. (options.cache_relative_path or "runtime\\state\\boss-kill-cache.json")

    local function match_boss(text)
        local normalized = catalog.normalize(text)
        if normalized == "" then return nil, nil end
        for _, item in ipairs(catalog.tokens or {}) do
            if normalized:find(item.token, 1, true) then
                return catalog.by_id[item.boss_id], "exact_uid_name"
            end
        end
        for _, item in ipairs(catalog.semantic_tokens or {}) do
            if normalized:find(item.token, 1, true) then
                return catalog.by_id[item.boss_id], "semantic_token"
            end
        end
        for _, record in ipairs(catalog.records or {}) do
            if tostring(text):find(tostring(record.uid), 1, true)
                or tostring(text):find(tostring(record.section_uid), 1, true)
                or (record.group_id ~= "0" and tostring(text):find(tostring(record.group_id), 1, true))
                or tostring(text):find(tostring(record.level_cid), 1, true)
            then
                return record, "numeric_identity"
            end
        end
        return nil, nil
    end

    function self:load_cache()
        local f = io.open(self.cache_path, "r")
        if not f then return end
        local text = f:read("*a") or ""
        f:close()
        local now = os.time()
        for boss_id, killed_at, available_at in text:gmatch(
            '"(%d+)"%s*:%s*{%s*"killed_at"%s*:%s*(%d+)%s*,%s*"available_at"%s*:%s*(%d+)'
        ) do
            if tonumber(available_at) and tonumber(available_at) > now then
                self.completed[tonumber(boss_id)] = {
                    killed_at = tonumber(killed_at),
                    available_at = tonumber(available_at),
                }
            end
        end
    end

    function self:write_cache()
        local ids = {}
        for boss_id in pairs(self.completed) do ids[#ids + 1] = boss_id end
        table.sort(ids)
        local lines = {
            "{",
            '  "schema": 1,',
            '  "updated_at": ' .. tostring(os.time()) .. ",",
            '  "targets": {',
        }
        for index, boss_id in ipairs(ids) do
            local state = self.completed[boss_id]
            local record = self.catalog.by_id[boss_id]
            lines[#lines + 1] = '    "' .. tostring(boss_id) .. '": {'
            lines[#lines + 1] = '      "killed_at": ' .. tostring(state.killed_at) .. ","
            lines[#lines + 1] = '      "available_at": ' .. tostring(state.available_at) .. ","
            lines[#lines + 1] = '      "respawn_cycle_id": ' .. tostring(record and record.respawn_cycle_id or 106) .. ","
            lines[#lines + 1] = '      "name": "' .. json_escape(record and record.name or "") .. '",'
            lines[#lines + 1] = '      "source": "OnBPMonsterDied"'
            lines[#lines + 1] = "    }" .. (index < #ids and "," or "")
        end
        lines[#lines + 1] = "  }"
        lines[#lines + 1] = "}"
        return atomic_write(self.cache_path, table.concat(lines, "\n") .. "\n")
    end

    function self:record_kill(record, match_mode, source_object, all_text)
        local now = os.time()
        local existing = self.completed[record.boss_id]
        if existing and existing.available_at > now then
            if self.logger then
                self.logger:write("BOSS_KILL_DUPLICATE_IGNORED", {
                    boss_id = record.boss_id,
                    name = record.name,
                    available_at = existing.available_at,
                })
            end
            return
        end
        local available_at = next_daily_reset(
            self.options.daily_reset_hour,
            self.options.daily_reset_minute
        )
        self.completed[record.boss_id] = {
            killed_at = now,
            available_at = available_at,
        }
        local ok, err = self:write_cache()
        if self.logger then
            self.logger:write("WORLD_BOSS_KILL_CONFIRMED", {
                boss_id = record.boss_id,
                name = record.name,
                uid = record.uid,
                uid_name = record.uid_name,
                level_cid = record.level_cid,
                group_id = record.group_id,
                section_uid = record.section_uid,
                respawn_cycle_id = record.respawn_cycle_id,
                match_mode = match_mode,
                source_object = full_name(source_object),
                source_class = class_name(source_object),
                killed_at = now,
                available_at = available_at,
                cache_write_ok = ok,
                cache_write_error = err or "none",
                identity = clean(all_text, 1600),
            })
        end
    end

    local function callback(context, ...)
        local target = self.registry.target
        if target == nil then return end
        local ok, err = xpcall(function()
            target.sequence = target.sequence + 1
            local candidates = {}
            local context_object = nil
            pcall(function() context_object = unwrap_object(context) end)
            if context_object == nil then
                pcall(function() context_object = unwrap_object(context:get()) end)
            end
            if context_object ~= nil then candidates[#candidates + 1] = context_object end

            local argc = select("#", ...)
            for i = 1, argc do
                local value = select(i, ...)
                local object = unwrap_object(value)
                if object ~= nil then candidates[#candidates + 1] = object end
            end

            local combined = {}
            local matched_record, matched_mode, matched_object
            for _, object in ipairs(candidates) do
                local text = identity_text(object)
                combined[#combined + 1] = text
                local record, mode = match_boss(text)
                if record ~= nil then
                    matched_record = record
                    matched_mode = mode
                    matched_object = object
                    break
                end
            end

            if matched_record ~= nil then
                target:record_kill(
                    matched_record,
                    matched_mode,
                    matched_object,
                    table.concat(combined, " || ")
                )
            elseif target.logger then
                target.logger:write("MONSTER_DIED_NON_BOSS_IGNORED", {
                    sequence = target.sequence,
                    candidate_count = #candidates,
                    argument_count = argc,
                    identity = clean(table.concat(combined, " || "), 1200),
                })
            end
        end, debug.traceback)
        if not ok and target and target.logger then
            target.logger:error("BOSS_KILL_CALLBACK_ERROR", {
                error = target.logger:trace(err),
            })
        end
    end

    function self:install(ctx)
        self.logger = ctx.logger
        self.registry.target = self
        self:load_cache()
        self:write_cache()

        if self.registry.installed then
            return {
                status = "readable",
                value_type = "boss_kill_hook",
                value = "already_installed",
                fingerprint = "boss_kill_hook:installed",
            }
        end
        if type(RegisterHook) ~= "function" then
            return {
                status = "unsupported",
                value_type = "boss_kill_hook",
                value = "RegisterHook_unavailable",
                fingerprint = "boss_kill_hook:unavailable",
            }
        end

        local path = self.options.hook_path or "/Script/DS.DMonsterSpawnActor:OnBPMonsterDied"
        local ok, pre_id, post_id = pcall(function()
            return RegisterHook(path, callback, function() end)
        end)
        if not ok then
            ctx.log("BOSS_KILL_HOOK_FAILED", {
                path = path,
                error = clean(pre_id, 1000),
            })
            return {
                status = "call_failed",
                value_type = "boss_kill_hook",
                value = clean(pre_id, 400),
                fingerprint = "boss_kill_hook:failed",
            }
        end

        self.registry.installed = true
        self.registry.pre_id = pre_id
        self.registry.post_id = post_id
        ctx.log("BOSS_KILL_HOOK_REGISTERED", {
            path = path,
            pre_id = pre_id or "nil",
            post_id = post_id or "nil",
            whitelist_count = #self.catalog.records,
            cache_path = self.cache_path,
            independent_of_radar_hotkeys = true,
        })
        return {
            status = "readable",
            value_type = "boss_kill_hook",
            value = "registered",
            fingerprint = "boss_kill_hook:installed",
        }
    end

    function self:steps()
        return {
            {
                name = "install_nine_world_boss_kill_hook",
                kind = "custom",
                role = "persistent_world_boss_kill_monitor",
                purpose = "listen_only_to_DMonsterSpawnActor_OnBPMonsterDied_and_cache_whitelisted_bosses",
                once_per_session = true,
                run = function(ctx) return self:install(ctx) end,
            },
        }
    end

    return self
end

return M
