local MOD_ID = "DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_1"

-- The native adapter owns the configurable toggle key (F9 by default), bounded native-selector scheduler,
-- target policy, mounted Rider route, one global pending action, and exact weak-identity confirmation.
-- This Lua entry point intentionally schedules no work and touches no UObject.
print(string.format("[%s] Native automatic pickup loaded; range unchanged, fish enabled, treasure boxes excluded; use the configured toggle key to enable or disable\n", MOD_ID))
