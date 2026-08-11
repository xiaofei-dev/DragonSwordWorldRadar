return {
    -- Unified DragonSwordWorldRadar layer control.
    show_treasures = true,
    show_bosses = true,
    -- Assault targets are generated from the installed game's current PAK.
    -- Set this to false to disable only the Assault layer.
    -- Diagnostic reproduction build: keep Assault enabled so the bounded F7
    -- crash trace can identify the last completed native boundary.
    show_assaults = true,
    show_moles = true,
    -- Stability-first isolated one-shot capture after F7. Static checks cannot
    -- prove native safety; use F8 for the in-game A/B boundary.
    show_world_status = true,

    -- F7 enables configured marker and world-status layers; F8 temporarily
    -- disables every mod feature for FPS comparison without rewriting config.
    refresh_key = "F7",
    toggle_key = "F8",
    auto_start_overlay = true,

    -- Treasure behavior and display settings.
    world_map_markers = true,
    show_height = true,
    show_treasure_types = false,
    text_scale = 1.0,

    -- Normal use log: startup, mode-independent lifecycle messages, warnings,
    -- and errors only. No periodic performance counters are collected.
    use_logging = true,

    -- Debug log: producer timings, bridge rates, Overlay frame pacing,
    -- CPU/memory samples, geometry transitions, and save/Boss diagnostics.
    -- Pre-release builds keep file-based performance diagnostics enabled so
    -- gameplay test sessions remain comparable. Disable this for the final
    -- release after performance acceptance is complete.
    debug_logging = true,
    diagnostic_perf_interval_seconds = 5,

    -- A/B switch for the Overlay's Windows timer-resolution request. False
    -- uses the process/system default; true requests timeBeginPeriod(1).
    -- Restart the game after changing this value.
    high_resolution_timer = false,

    -- Backward-compatible aliases retained for existing user config files.
    diagnostic_logging = true,
    diagnostic_verbose = false,
}
