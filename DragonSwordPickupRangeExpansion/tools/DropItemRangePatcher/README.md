# Drop-item range patcher

This .NET 8 tool performs the structured cooked-asset edit required for the
class-proven `DropItemActor` children. It uses pinned UAssetAPI 1.1.0 and the
exact Dragon Sword UE 5.3 mappings file to add only
`SphereOverlapComp.RelativeScale3D`.

The tool rejects source hash drift, rejects assets that do not round-trip with
binary equality, requires the reviewed one-property overlap component, and
verifies that `CapsulePhysicsComp` and `SphereHitComp` remain semantically
unchanged. It never patches either protected component.

The mappings file is an external build input and is not redistributed here.
The release build verifies its SHA-256 before invoking this tool.
