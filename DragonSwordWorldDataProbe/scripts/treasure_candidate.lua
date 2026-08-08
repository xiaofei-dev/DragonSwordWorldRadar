return {
    enabled = false,

    -- Populated only after discovery confirms a read-only bool-returning function.
    function_path = "",
    owner_cdo_path = "/Script/DS.Default__DETUtil",
    call_shape = "", -- world_id | id_only | world_id_bool
    extra_bool = false,

    -- Validation requires both sides. Never infer "opened" from catalog absence.
    known_opened_ids = {},
    known_unopened_ids = {},

    expected = {
        opened = true,
        unopened = false,
    },
}
