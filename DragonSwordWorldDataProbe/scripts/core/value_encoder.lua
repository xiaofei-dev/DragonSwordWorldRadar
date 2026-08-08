local M = {}

local function clean(text, limit)
    text = tostring(text or ""):gsub("[\r\n\t]+", " ")
    if #text > limit then return text:sub(1, limit) .. "..." end
    return text
end

local function primitive(kind, value)
    local text = tostring(value)
    return {
        status = "readable",
        value_type = kind,
        value = text,
        fingerprint = kind .. ":" .. text,
    }
end

local function table_summary(value, limit, max_items)
    local count = 0
    local primitive_parts = {}
    local ok, err = pcall(function()
        for key, item in pairs(value) do
            count = count + 1
            if #primitive_parts < max_items then
                local key_kind = type(key)
                local item_kind = type(item)
                local key_text = (key_kind == "string" or key_kind == "number") and tostring(key) or "<" .. key_kind .. ">"
                local item_text
                if item_kind == "string" or item_kind == "number" or item_kind == "boolean" then
                    item_text = tostring(item)
                else
                    item_text = "<" .. item_kind .. ">"
                end
                primitive_parts[#primitive_parts + 1] = clean(key_text, 80) .. "=" .. clean(item_text, 120)
            end
            if count >= 4096 then break end
        end
    end)
    if not ok then
        return {
            status = "encode_error",
            value_type = "table",
            value = "table_iteration_error",
            fingerprint = "table:error",
            error = clean(err, limit),
        }
    end
    local text = "count=" .. tostring(count)
    if #primitive_parts > 0 then text = text .. " sample=[" .. table.concat(primitive_parts, ",") .. "]" end
    return {
        status = "complex_present",
        value_type = "table",
        value = clean(text, limit),
        fingerprint = "table:count=" .. tostring(count) .. ":sample=" .. clean(table.concat(primitive_parts, ","), limit),
    }
end

function M.encode(value, options)
    options = options or {}
    local limit = tonumber(options.limit) or 1600
    local max_items = tonumber(options.max_items) or 16
    if value == nil then
        return { status = "nil", value_type = "nil", value = "nil", fingerprint = "nil" }
    end
    local kind = type(value)
    if kind == "boolean" or kind == "number" then return primitive(kind, value) end
    if kind == "string" then
        local text = clean(value, limit)
        return { status = "readable", value_type = "string", value = text, fingerprint = "string:" .. text }
    end
    if kind == "table" then return table_summary(value, limit, max_items) end
    -- Never call tostring on UE4SS userdata. Presence is still useful and safe.
    return {
        status = "complex_present",
        value_type = kind,
        value = "<" .. kind .. "-present>",
        fingerprint = kind .. ":present",
    }
end

function M.length(value)
    if value == nil then return M.encode(nil) end
    local kind = type(value)
    local ok, result = pcall(function() return #value end)
    if not ok then error(result) end
    return primitive("length", result)
end

return M