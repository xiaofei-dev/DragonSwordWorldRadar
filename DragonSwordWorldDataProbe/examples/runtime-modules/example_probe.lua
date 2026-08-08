local source = debug.getinfo(1, "S").source
local file = source:sub(1, 1) == "@" and source:sub(2) or source
local example_dir = file:match("^(.*)[/\\][^/\\]+$") or "."
local root = example_dir:match("^(.*)[/\\]examples[/\\]runtime%-modules$") or "."
local api = dofile(root .. "\\scripts\\core\\module_api.lua")

return {
    id = "example_probe",
    enabled = true,
    steps = {
        api.class_presence("DExampleClass", "example_schema", 1),
        api.property("DExampleClass", "ExampleField", "example_schema", 1),
        api.custom(
            "example_custom",
            "example",
            "demonstrate_a_bounded_read_only_custom_step",
            function(ctx)
                ctx.log("EXAMPLE_CUSTOM", { note = "replace with a bounded read-only collector" })
                return {
                    status = "readable",
                    value_type = "example",
                    value = "ok",
                    fingerprint = "example:ok",
                }
            end,
            { once_per_session = true }
        ),
    },
}
