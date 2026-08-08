local M = {}

function M.new(options)
    local self = {
        state_dir = options.state_dir,
        pending_path = options.pending_path,
        quarantine_path = options.quarantine_path,
        unsupported_path = options.unsupported_path,
        schema = tonumber(options.schema) or 1,
    }

    local function clean(value)
        return tostring(value == nil and "" or value):gsub("[\r\n=]", " ")
    end

    local function safe_id(value)
        return clean(value):gsub("[^%w_%-]", "_")
    end

    local function state_path(probe_id)
        return self.state_dir .. "\\probe-state-" .. safe_id(probe_id) .. ".txt"
    end

    local function read_map(path)
        local result = {}
        local file = io.open(path, "r")
        if not file then return result end
        for line in file:lines() do
            local key, value = line:match("^([^=]+)=(.*)$")
            if key then result[key] = value end
        end
        file:close()
        return result
    end

    local function write_lines_atomic(path, lines)
        local temp = path .. ".tmp"
        local file, err = io.open(temp, "w")
        if not file then return false, err or "open_failed" end
        local ok, write_err = pcall(function()
            for _, line in ipairs(lines or {}) do file:write(line, "\n") end
            file:flush()
        end)
        pcall(function() file:close() end)
        if not ok then
            os.remove(temp)
            return false, write_err or "write_failed"
        end
        local backup = path .. ".bak"
        os.remove(backup)
        local target_moved = os.rename(path, backup)
        local renamed, rename_err = os.rename(temp, path)
        if not renamed then
            if target_moved then os.rename(backup, path) end
            os.remove(temp)
            return false, rename_err or "rename_failed"
        end
        if target_moved then os.remove(backup) end
        return true
    end

    local function write_map(path, data)
        local keys = {}
        for key, _ in pairs(data or {}) do keys[#keys + 1] = key end
        table.sort(keys)
        local lines = {}
        for _, key in ipairs(keys) do
            lines[#lines + 1] = clean(key) .. "=" .. clean(data[key])
        end
        return write_lines_atomic(path, lines)
    end

    local function read_set(path)
        local result = {}
        local file = io.open(path, "r")
        if not file then return result end
        local first = file:read("*l")
        local file_schema = first and tonumber(first:match("^#schema=(%d+)$")) or nil
        if file_schema ~= self.schema then
            file:close()
            return result
        end
        for line in file:lines() do
            if line ~= "" and line:sub(1, 1) ~= "#" then result[line] = true end
        end
        file:close()
        return result
    end

    local function write_set(path, values)
        local keys = {}
        for key, enabled in pairs(values or {}) do
            if enabled then keys[#keys + 1] = clean(key) end
        end
        table.sort(keys)
        local lines = { "#schema=" .. tostring(self.schema) }
        for _, key in ipairs(keys) do lines[#lines + 1] = key end
        return write_lines_atomic(path, lines)
    end

    local function add_set(path, key)
        local current = read_set(path)
        key = clean(key)
        if current[key] then return true end
        current[key] = true
        return write_set(path, current)
    end

    function self:load_state(probe_id)
        local state = read_map(state_path(probe_id))
        local schema = tonumber(state.schema) or 0
        if state.probe ~= probe_id or schema ~= self.schema then
            return { probe = probe_id, schema = self.schema, pass = 1, index = 1 }
        end
        return {
            probe = probe_id,
            schema = self.schema,
            pass = math.max(1, tonumber(state.pass) or 1),
            index = math.max(1, tonumber(state.index) or 1),
        }
    end

    function self:save_state(state)
        state.schema = self.schema
        return write_map(state_path(state.probe), state)
    end

    function self:read_pending()
        local pending = read_map(self.pending_path)
        if not pending.step_id or pending.step_id == "" then return nil end
        return pending
    end

    function self:write_pending(record)
        record.schema = self.schema
        return write_map(self.pending_path, record)
    end

    function self:clear_pending() os.remove(self.pending_path) end
    function self:load_quarantine() return read_set(self.quarantine_path) end
    function self:add_quarantine(step_id) return add_set(self.quarantine_path, step_id) end
    function self:load_unsupported() return read_set(self.unsupported_path) end
    function self:add_unsupported(step_id) return add_set(self.unsupported_path, step_id) end

    return self
end

return M
