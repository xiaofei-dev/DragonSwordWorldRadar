local M = {}

local function split_path(text)
    local result = {}
    for part in tostring(text or ""):gmatch("[^%.]+") do result[#result + 1] = part end
    return result
end

function M.class_presence(class_name, role, cadence)
    return {
        name = "class_" .. tostring(class_name), kind = "class", class = class_name,
        path = {}, role = role or "schema", purpose = "exact_class_presence",
        cadence = cadence or 1,
    }
end

function M.property(class_name, dotted_path, role, cadence)
    return {
        name = "property_" .. tostring(class_name) .. "_" .. tostring(dotted_path):gsub("[^%w]", "_"),
        kind = "property", class = class_name, path = split_path(dotted_path),
        role = role or "schema", purpose = "exact_property_read", cadence = cadence or 1,
    }
end

function M.method(class_name, object_path, method, role, cadence)
    return {
        name = "method_" .. tostring(class_name) .. "_" .. tostring(method),
        kind = "method", class = class_name, path = split_path(object_path), method = method,
        role = role or "runtime", purpose = "exact_no_argument_method_call", cadence = cadence or 1,
    }
end

function M.custom(name, role, purpose, run, options)
    options = options or {}
    return {
        name = name, kind = "custom", role = role, purpose = purpose, run = run,
        cadence = options.cadence or 1, once_per_session = options.once_per_session == true,
    }
end

return M
