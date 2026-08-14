local OWNER_CANARY = "OWNER_AUTHORIZED_NATIVE_VISIBILITY_F_INPUT"

-- The native adapter uses UE4SS's native EngineTick post callback as its
-- bounded game-thread pulse. This Lua entry point intentionally schedules no work.
print(string.format("[%s] Native visibility/F-input marker loaded; Lua scheduler disabled\n", OWNER_CANARY))
