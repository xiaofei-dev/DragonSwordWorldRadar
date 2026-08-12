local OWNER_CANARY = "OWNER_AUTHORIZED_RUNTIME_CANARY"

-- The native adapter uses UE4SS's native EngineTick post callback as its
-- bounded game-thread pulse. This Lua entry point intentionally schedules no work.
print(string.format("[%s] Read-only DropItemActor closed-loop diagnostic loaded; Lua scheduler disabled\n", OWNER_CANARY))
