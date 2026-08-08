local M = {}

function M.new(options)
    local self = {
        log_path = options.log_path,
        error_path = options.error_path,
        status_path = options.status_path,
        observation_path = options.observation_path,
        change_path = options.change_path,
        static_catalog_path = options.static_catalog_path,
        actor_sample_path = options.actor_sample_path,
        lifecycle_path = options.lifecycle_path,
        session = options.session,
        max_len = options.max_len or 1600,
        headers = {},
    }

    local function safe_text(value)
        if value == nil then return "nil" end
        local kind = type(value)
        if kind == "string" or kind == "number" or kind == "boolean" then return tostring(value) end
        return "<" .. kind .. ">"
    end

    local function sanitize(value, limit)
        local text = safe_text(value):gsub("[\r\n\t]+", " ")
        limit = limit or self.max_len
        if #text > limit then return text:sub(1, limit) .. "..." end
        return text
    end

    local function append(path, line)
        if path == nil or path == "" then return false, "path_missing" end
        local file, err = io.open(path, "a")
        if not file then return false, err or "open_failed" end
        local ok, write_err = pcall(function()
            file:write(line, "\n")
            file:flush()
        end)
        pcall(function() file:close() end)
        if not ok then return false, write_err or "write_failed" end
        return true
    end

    local function make_line(event, fields)
        local parts = {
            "[" .. os.date("!%Y-%m-%dT%H:%M:%SZ") .. "]",
            "[" .. sanitize(event, 100) .. "]",
            "session=" .. sanitize(self.session, 100),
        }
        local entries = {}
        if type(fields) == "table" then
            for key, value in pairs(fields) do entries[#entries + 1] = { key = tostring(key), value = value } end
            table.sort(entries, function(a, b) return a.key < b.key end)
            for _, entry in ipairs(entries) do
                parts[#parts + 1] = sanitize(entry.key, 100) .. "=" .. sanitize(entry.value)
            end
        end
        return table.concat(parts, " ")
    end

    local function console(prefix, line)
        pcall(function() print(prefix .. line) end)
    end

    local function ensure_header(path, key, columns)
        if self.headers[key] then return true end
        local file = io.open(path, "r")
        if file then
            local first = file:read("*l")
            file:close()
            self.headers[key] = first ~= nil and first ~= ""
        end
        if not self.headers[key] then
            local ok, err = append(path, table.concat(columns, "\t"))
            if not ok then return false, err end
            self.headers[key] = true
        end
        return true
    end

    local function write_tsv(path, key, columns, values)
        local ok_header, header_err = ensure_header(path, key, columns)
        if not ok_header then return false, header_err end
        local row = {}
        for index = 1, #columns do row[index] = sanitize(values[index]) end
        return append(path, table.concat(row, "\t"))
    end

    function self:write(event, fields)
        local line = make_line(event, fields)
        local ok, err = append(self.log_path, line)
        if not ok then console("[DragonSwordWorldDataProbe][LOG_WRITE_FAILED] ", sanitize(err)) end
        console("[DragonSwordWorldDataProbe] ", line)
    end

    function self:error(event, fields)
        local line = make_line(event, fields)
        local ok1, err1 = append(self.log_path, line)
        local ok2, err2 = append(self.error_path, line)
        if not ok1 then console("[DragonSwordWorldDataProbe][LOG_WRITE_FAILED] ", sanitize(err1)) end
        if not ok2 then console("[DragonSwordWorldDataProbe][ERROR_WRITE_FAILED] ", sanitize(err2)) end
        console("[DragonSwordWorldDataProbe][ERROR] ", line)
    end

    function self:observation(fields)
        local columns = {
            "utc", "session", "pass", "step_id", "step_name", "probe", "class", "role",
            "operation", "path", "method", "purpose", "status", "value_type", "value",
            "fingerprint", "changed"
        }
        local row = {
            os.date("!%Y-%m-%dT%H:%M:%SZ"), self.session, fields.pass, fields.step_id,
            fields.step_name, fields.probe, fields.class, fields.role, fields.operation,
            fields.path, fields.method, fields.purpose, fields.status, fields.value_type,
            fields.value, fields.fingerprint, fields.changed,
        }
        local ok, err = write_tsv(self.observation_path, "observation", columns, row)
        if not ok then self:error("OBSERVATION_WRITE_FAILED", { error = err or "unknown" }) end
    end

    function self:change(fields)
        local line = make_line("PROBE_OBSERVATION_CHANGED", fields)
        local ok, err = append(self.change_path, line)
        if not ok then self:error("CHANGE_LOG_WRITE_FAILED", { error = err or "unknown" }) end
        console("[DragonSwordWorldDataProbe][PROBE_CHANGE] ", line)
    end

    function self:static_boss(fields)
        local columns = {
            "utc", "session", "boss_id", "name", "kind", "update_target",
            "map_id", "world_map_section_id", "switch_week_id",
            "x", "y", "z", "uid", "uid_name", "actor_tokens", "group_id", "section_uid",
            "respawn_cycle_id", "level_cid", "server_death_check"
        }
        local row = {
            os.date("!%Y-%m-%dT%H:%M:%SZ"), self.session,
            fields.boss_id, fields.name, fields.kind, fields.update_target,
            fields.map_id, fields.world_map_section_id, fields.switch_week_id,
            fields.x, fields.y, fields.z, fields.uid, fields.uid_name,
            type(fields.actor_tokens) == "table" and table.concat(fields.actor_tokens, ",") or fields.actor_tokens,
            fields.group_id, fields.section_uid, fields.respawn_cycle_id,
            fields.level_cid, fields.server_death_check,
        }
        local ok, err = write_tsv(self.static_catalog_path, "static_boss", columns, row)
        if not ok then self:error("STATIC_BOSS_WRITE_FAILED", { error = err or "unknown" }) end
    end

    function self:actor_sample(fields)
        local columns = {
            "utc", "session", "source", "sample", "boss_id", "field",
            "actor_name", "actor_full_name", "actor_class", "x", "y", "z",
            "candidate_kind", "location_status", "location_error",
            "match_mode", "match_token", "distance_xy", "distance_z",
            "status", "value_type", "value", "dead_candidate"
        }
        local row = {
            os.date("!%Y-%m-%dT%H:%M:%SZ"), self.session,
            fields.source, fields.sample, fields.boss_id, fields.field,
            fields.actor_name, fields.actor_full_name, fields.actor_class,
            fields.x, fields.y, fields.z,
            fields.candidate_kind, fields.location_status, fields.location_error,
            fields.match_mode, fields.match_token, fields.distance_xy, fields.distance_z,
            fields.status, fields.value_type, fields.value, fields.dead_candidate,
        }
        local ok, err = write_tsv(self.actor_sample_path, "actor_sample", columns, row)
        if not ok then self:error("ACTOR_SAMPLE_WRITE_FAILED", { error = err or "unknown" }) end
    end

    function self:lifecycle(fields)
        local columns = {
            "utc", "session", "event", "boss_id", "candidate_kind", "identity_match",
            "match_mode", "match_token", "distance_xy", "distance_z",
            "actor_name", "actor_full_name", "actor_class", "x", "y", "z",
            "argument_count", "arguments"
        }
        local row = {
            os.date("!%Y-%m-%dT%H:%M:%SZ"), self.session,
            fields.event, fields.boss_id, fields.candidate_kind, fields.identity_match,
            fields.match_mode, fields.match_token, fields.distance_xy, fields.distance_z,
            fields.actor_name, fields.actor_full_name, fields.actor_class,
            fields.x, fields.y, fields.z, fields.argument_count, fields.arguments,
        }
        local ok, err = write_tsv(self.lifecycle_path, "lifecycle", columns, row)
        if not ok then self:error("LIFECYCLE_WRITE_FAILED", { error = err or "unknown" }) end
    end

    function self:write_status(text)
        local temp = self.status_path .. ".tmp"
        local file, err = io.open(temp, "w")
        if not file then self:error("STATUS_WRITE_FAILED", { error = err or "open_failed" }); return false end
        local ok, write_err = pcall(function() file:write(text or ""); file:flush() end)
        pcall(function() file:close() end)
        if not ok then self:error("STATUS_WRITE_FAILED", { error = write_err or "write_failed" }); return false end
        local backup = self.status_path .. ".bak"
        os.remove(backup)
        local target_moved = os.rename(self.status_path, backup)
        local renamed, rename_err = os.rename(temp, self.status_path)
        if not renamed then
            if target_moved then os.rename(backup, self.status_path) end
            os.remove(temp)
            self:error("STATUS_RENAME_FAILED", { error = rename_err or "rename_failed" })
            return false
        end
        if target_moved then os.remove(backup) end
        return true
    end

    function self:trace(err)
        local text = safe_text(err)
        local ok, trace = pcall(function() return debug.traceback(text, 2) end)
        trace = ok and trace or text
        trace = tostring(trace):gsub("[\r\n\t]+", " ")
        if #trace > self.max_len * 4 then trace = trace:sub(1, self.max_len * 4) .. "..." end
        return trace
    end

    return self
end

return M
