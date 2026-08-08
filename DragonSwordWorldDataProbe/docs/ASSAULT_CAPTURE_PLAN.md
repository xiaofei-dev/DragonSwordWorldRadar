# 突击/勇士数据的可靠采集方案

## 已确认

- 正式静态表：`UnexpectedMissionWorldData`、`UnexpectedMissionPlaceData`、`UnexpectedMissionKindData`。
- World 字段：`MapID`、`MissionCnt_Min/Max`、`DaySwitchID`。
- Place 字段：`ID`、`MapID`、`MissionKindData`、`MissionRange`。
- Kind 字段：`ID`、`GroupID`、`MissionType`、`MissionValue1/2/3`、`MissionCount`、`AcceptConditionType`、`AcceptConditionValue1/2/3`、`DataLayer`。
- 已成功提取的支持表包括 `SectionMonsterData`、`SpawnMonsterGroupData`、`ActorSpawnConditionData`、`NPCSpawnConditionData`、`MonsterCharacterData`、`ActorPositionData`。
- 最终动态状态应复用 `tb_actor_respawn`；任务完成 UI 不需要。

## PAK 解码器为什么改为标准解析

目录索引给出的 compact entry offset：

```text
UnexpectedMissionKindData  15620
UnexpectedMissionPlaceData 15636
UnexpectedMissionWorldData 15652
```

因此 Kind 和 Place 的记录长度都是 16 字节，World 是 12 字节。这符合标准紧凑 FPakEntry：4 字节 flags，随后按 bit 31/30/29 选择 32/64 位 offset/size；压缩槽位、加密标志、block count 和 block size 也编码在 flags 中。

本框架只使用：

1. 目录中的原始 offset；
2. 下一个更大的 compact offset 作为精确记录边界；
3. PAK 索引已经推导出的单一 `numberMask`；
4. 标准 bitfield 解析；
5. 解析长度必须等于目录记录长度；
6. `ReadDataEntry`/`ValidateEntry` 的数据头一致性；
7. AES/Oodle 解压；
8. XML 根节点必须精确等于配置值。

绝不使用相邻 offset、0..255 mask 穷举或“能解析成任意 XML 就算成功”。

## 语义关联

Kind/Place 提取成功后，`assault_catalog` 只做精确值交叉引用：

```text
Place.MissionKindData → Kind.ID
Kind.MissionValue1/2/3、GroupID
    ↔ SectionMonster UID/CID/GroupID/SectionUID/LevelCID
    ↔ SpawnMonsterGroup.ID/Position1..20
    ↔ MonsterCharacter.ID
    ↔ ActorPosition.ID/MapGroupID
Kind.AcceptConditionValue1/2/3
    ↔ Actor/NPCSpawnCondition.ID
    ↔ Weather/Climate/RespawnCycle.ID
```

`MissionType` 和 `AcceptConditionType` 的含义不会仅凭字段名猜测。输出分为：

- `confirmed_exact_unique`：值级关联唯一；只代表关联唯一，不自动代表玩法语义已经确认。
- `candidate_multiple`：同一值命中多条记录。
- `unresolved`：没有精确命中。

## 最终 Radar 模型

```text
静态突击激活规则和条件
+ 精确目标 Actor UID/CID/坐标/RespawnCycleID
+ 现有 tb_actor_respawn 死亡/冷却状态
= 仅显示当前激活且存活的突击目标
```

## 接入前停止条件

- 每个图标对应唯一静态 Actor 身份；
- 天气/开关条件有明确表或存档证据；
- 至少完成一次突击前后存档差异复现；
- 不依赖任务 UI，也不依赖当前地图分区是否加载；
- 不新增独立 60 秒刷新循环。

## 1.0.3 一次运行策略

`pak_static` 已把下一阶段合并：
1. proven extraction；
2. exact compact recovery；
3. exact table identity validation；
4. failure evidence capture；
5. assault_catalog correlation。

关键表不会执行 mask 穷举或相邻记录恢复。
若仍失败，当前诊断 ZIP 已足够继续实现精确 decoder。
