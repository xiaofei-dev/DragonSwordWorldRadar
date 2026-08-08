# 1.0.23

Assault Kind/Place decoding is complete. The remaining catalog blocker in 1.0.22
was filename normalization: build-pinned support XML retained extraction prefixes
such as `006_SectionMonsterData.xml` while the parser requests canonical names.
1.0.23 creates canonical copies before parsing.

Treasure reference discovery now resolves Function owners from the function path
itself, because ObjectDump Function records do not carry `[owr:]` in this build.
It also inventories query-shaped generic Save/Prop/State functions and the live
DPropDataTable class without calling any discovered function.
