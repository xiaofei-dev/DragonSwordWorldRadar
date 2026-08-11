local OWNER_CANARY = "OWNER_AUTHORIZED_RUNTIME_CANARY"

-- The native adapter uses the game's natural local input-frame UFunction as
-- its game-thread pulse. This Lua entry point intentionally schedules no work.
print(string.format("[%s] Native input-frame pulse enabled; Lua scheduler disabled\n", OWNER_CANARY))
