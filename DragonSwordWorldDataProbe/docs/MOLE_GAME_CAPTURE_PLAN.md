# Mole 土拨鼠小游戏完整采集目标

范围仅指 Mole 这一类小游戏，已知全世界节点：
- `MiniGame_Mole_12001`
- ...
- `MiniGame_Mole_12040`

共 40 个。

已确认类/结构：
- `DsMiniGameNode_Mole`
- `DsMiniGameMoleAsset`
- `DsMiniGameMoleComponent`
- `DsMiniGameMoleData`
- `DMiniGameTable`
- `EMoleType`
- `RoundSetting`

1.0.5 收集：
1. 12001–12040 的全部 ObjectDump 节点；
2. DMiniGameTable / Mole 类 Schema；
3. 静态 XML 中 UID/CID/GroupID/SectionUID/坐标/SpawnCondition/Respawn 等候选；
4. 持久完成状态候选字段/类。

最终生产逻辑：
- 未完成 Mole 节点：地图显示；
- 已完成 Mole 节点：地图隐藏。

不需要采集 Mole 内部每轮 DPS 或击杀过程；回合、Spawner、怪物数组仅在需要解释完成状态时作为辅助证据。

禁止：
- 全量 Character 扫描；
- ReceiveDestroyed / ReceiveEndPlay；
- 以“当前 Actor 是否加载”作为全地图完成状态。
