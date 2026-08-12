DragonSwordWorldDataProbe 1.0.52 — 模块化多方式数据采集框架

目标
- 将此前的静态 PAK、存档、运行时反射、对象快照、UI、Hook、原生数据库实验和离线分析统一到一个可插拔框架。
- 默认配置只运行低风险方法：Lua 健康检查、静态 PAK 提取、突击静态关联分析。
- Boss 功能不修改；现有 Radar 的 tb_actor_respawn 与刷新规则继续作为唯一 Boss 动态状态实现。

默认流程
1. Lua 只加载 healthcheck，并启动一个隐藏外部 Monitor。
2. static_inventory 确认游戏路径和构建指纹。
3. pak_static 使用 PakReaderCore.exe + ooz.exe，从 PAK 定向提取 GeneratedGameData XML。
4. assault_catalog 对 Kind/Place/怪物/生成条件做“精确值关联”，不猜字段语义。
5. 如果关键 XML 仍缺失，pak_decoder_evidence 保存精确目录记录、解析错误和 PakReaderCore IL。
6. Collect-Diagnostics.cmd 汇总所有模块结果为一个 ZIP。

命令
- Start-Monitor.cmd：手动启动单一隐藏 Monitor。Lua 默认也会自动启动，互斥锁会阻止重复实例。
- Collect-Diagnostics.cmd：重新运行 collect 模块并生成统一诊断 ZIP。

安装
1. 完整删除旧 DragonSwordWorldDataProbe 文件夹。
2. 将本文件夹放入 Win64\Mods\DragonSwordWorldDataProbe。
3. 保持 enabled.txt=1。
4. 进入游戏世界；默认模块不扫描 Actor、不注册 Hook。
5. 正常退出后运行 Collect-Diagnostics.cmd。

扩展
- 运行时 Lua 模块：scripts/modules；通过 scripts/config.lua 的白名单启用。
- 外部模块：tools/modules/<id>/Run.ps1；在 config/modules.json 和 profile 中注册。
- 静态目标：复制 config/static-targets/assault.json，定义文件名、预期 XML 根节点和关联策略。

重要安全默认值
- FindAllOf=false
- native_hooks=false
- ReceiveDestroyed/ReceiveEndPlay 明确禁止
- mutate_game_state=false
- 未知 TMap/TArray 不枚举

详见 docs/METHODS.md、ARCHITECTURE.md、MODULE_API.md、EXTENSION_RECIPES.md、VERSION_HISTORY.md、ASSAULT_CAPTURE_PLAN.md、SAFETY.md。

1.0.2：保留模块化外壳，但 PAK 后端恢复为之前已实测成功的 PakReaderCore + ooz 提取器。

1.0.3：把下一步合并进一次采集。
- 先运行已验证的 PAK 提取器；
- Kind/Place/Respawn 缺失时，用真实 numberMask + 精确目录 offset 做一次标准 compact-entry 解码；
- 禁止关键表使用相邻 offset 或 0..255 mask 猜测；
- 若仍失败，当次诊断自动包含 decoder_evidence 原始 bytes 与 PakReaderCore IL，不需要再装第二个补丁。

1.0.4：关闭 Boss 采集/监听；新增 mole_game_discovery 土拨鼠/地鼠静态 PAK + ObjectDump 双发现模块。

1.0.5：
- Mole 土拨鼠范围固定为全部 40 个 `MiniGame_Mole_12001–12040`；
- 新增 Mole 完成状态发现模块；
- 突击任务采集继续保留；
- PAK 静态提取增加共享 fingerprint cache，Monitor/诊断不再重复全量读取同一 PAK；
- Collect-Diagnostics 默认直接打包已有结果，仅缺模块结果时补跑；
- 控制台显示 1/3、2/3、3/3 进度。

1.0.6 审计版：
- Boss 监听保持关闭；
- 游戏运行期间不执行重 PAK 扫描，静态模块在退出游戏后运行；
- Monitor 与 Collect 通过全局 mutex 串行，避免重复/冲突扫描；
- PAK cache 含 extractor schema，避免升级后复用旧 decoder 结果；
- partial cache 保持 partial，不再误报 success；
- 单次共享 PAK 扫描同时覆盖突击与 MiniGameData/Mole；
- Mole 12001–12040 正则与精确数字反查已修复；
- Collect 会复用同版本、同游戏 build 的模块结果。

1.0.7：
- 修复 `001_MiniGameData.xml` / `004_UnexpectedMissionWorldData.xml` 等编号前缀识别；
- 游戏内只读查询 12001–12040：DETUtil.CIsClearMiniGameInStandAlone；
- 12001 先作为 canary，成功后才查询剩余 39 个；
- 结果写入 `runtime/reports/mole-completion-state.tsv`；
- Kind/Place/Respawn 增加严格的 16-byte 单块 XOR compact fallback；
- Boss 监听继续关闭。

1.0.8 HOTFIX：修复 Mole 扫描约 1 分钟后反复弹出窗口；完成状态函数本身已由 12001–12018 实测验证。

1.0.9：Mole 改为 canary+bulk；删除 AssaultStaticProbe 旧内部 mutex；强制新 XOR16 静态采集。

1.0.10：
- 突击 Kind/Place 主路线改为游戏内 DUnexpectedMissionTable 真表快照；
- PAK XOR16 变成验证/备份，不再是唯一阻塞点；
- 修复 1.0.9 遗留的 `$created=false` 假 singleton；
- Place.MissionKindData 主关联改为 Kind.GroupID；
- 新增 CUnexpectedMissionInStandAlone 原始状态快照，暂不猜语义。

1.0.11：
- pak_static 从正常流程移除，只保留手动验证；
- 正常退出/诊断不再卡在 pak_static；
- 突击主来源是 DUnexpectedMissionTable；
- 如需验证 PAK，退出游戏后运行 Run-Pak-Verification.cmd，硬超时 180 秒。

1.0.12：
- 突击运行时表读取改成 StructProperty 反射自发现，不再假定第一层字段叫 Data；
- 新增 assault-runtime-schema.tsv；
- assault_catalog 删除 pak_static_result.json 硬依赖；
- PAK 继续只是手动验证。

1.0.13：
- DataProbe 进入突击专用模式，Mole/Boss 采集关闭；
- 修正 UE4SS Reflection() 属性读取；
- Place/Kind 失败会自动后续重试；
- 条件查询改为每批最多 32 个；
- 同 build 内置已验证的 SectionMonster/ActorPosition/SpawnCondition 支持表，不再需要 PAK；
- 新增活动突击 TargetPosition/Radius/Timer 快照。

1.0.14：
- 补回之前物理删除的 Mole 静态 PAK 提取器及工具副本；
- 被替换过的关键 Mole/突击/PAK 实现归档到 archive/legacy-snapshots；
- 历史方法以后只关闭，不删除；
- 当前默认仍然只运行突击采集，不恢复旧模块到默认流程。

1.0.15：
- 历史模块继续全部保留；
- 新增 assault_kind_place_fast 独立模块，只补 Kind/Place 两张表；
- 运行时真表优先，真表成功则完全跳过 fast PAK；
- fast PAK 限定当前 build、只扫两张表、60 秒硬超时；
- 新增 assault-runtime-access.tsv / functions.tsv，进一步定位 UScriptStruct MapProperty。

1.0.16：修复 access_probe Lua 作用域错误和 fast Kind/Place 的 WinPS 5.1 inline-if 错误；研究路线不变。

1.0.17：修复 fast Kind/Place 候选筛选为 0 的确定性 bug；这版会第一次真正执行两张表的 XOR16 解码，并完整打包 decoder evidence。

1.0.18：XOR16 进入 DataEntry header 精确诊断/校正阶段；只有唯一且 metadata 完全一致的 nearby header 才会用于解压。

1.0.19：仅修复 WinPS 5.1 不支持 `1u` 无符号字面量的问题；XOR16/DataEntry 逻辑完全不变。

1.0.20：修复 DataEntry 调试快照读取不存在的 PayloadOffset，避免诊断代码打断真实 metadata 匹配；decoder 语义不变。

1.0.21：
- 保留现有突击采集，同时新增宝箱原生状态接口发现模块；
- 游戏内只枚举 DETUtil 函数/签名，绝不调用未知函数；
- 退出后扫描完整 ObjectDump 并排名 Treasure/TreasureBox/Chest 候选；
- 内置 1693 条宝箱静态 catalog 和 tb_treasure_box 位图真值模型；
- 候选验证器已准备但默认关闭，必须同时有已开与未开 ID 才允许启用。

1.0.22：突击 Kind/Place XOR16 已闭合并修复成功结果误落入 legacy fallback；宝箱改走完整 ObjectDump TreasureBox 引用图/owner 函数发现。

1.0.24：突击 40/40 已精确闭合到唯一 Actor/UID/XYZ，并开始一次性 CUnexpectedMissionInStandAlone 双状态采样；宝箱继续只读定位 DPropDataTable。

1.0.25：修复突击 false->ERR；宝箱研究收窄为 save_id -> opened state，重点搜世界宝箱 Actor/Component 与 Save/Player/WorldState 状态容器，UI/冒险手册降权。

1.0.26：突击默认关闭并冻结，完整数据/模块保留；正常管线只研究宝箱 Actor→Interaction/Prop state→persistent opened state。

1.0.27：加入宝箱真实 Actor 状态/交互 hook/消失变化采集；临时恢复突击 40-ID 低频状态迁移采样，用一次任务确认 bool 语义。未知宝箱函数仍不调用。

1.0.28：Collect-Diagnostics 可在游戏运行时提前启动，等待游戏退出后自动继续；控制台细分显示游戏等待、自动 Monitor、ModuleHost、模块子步骤、复制、ObjectDump、打包和 ZIP 校验阶段。

1.0.29：宝箱停止猜 DsAnimationProp，直接枚举 TreasureBoxPropDataMap.BlueprintPath，反推真实 Blueprint Actor 类并在 ObjectDump 中找其状态函数/属性；突击继续低频迁移采样。保留 1.0.28 自动等待游戏退出诊断。

1.0.30：突击新增 Quest Trigger/ISRegister/QuestSystem、RespawnCycle 105、Weather/DaySwitch 三路相关性采集；CUnexpectedMissionInStandAlone 不再当显示条件。

1.0.31：1.0.30 游戏内新 Quest Trigger/环境探针全部回退关闭；运行时恢复到已验证稳定的 Treasure Blueprint + 突击 bool 采样。RespawnCycle/Quest/天气研究仅在退出游戏后运行。

1.0.32：修复 1.0.30 崩溃风险——突击从单步 80 次原生调用降为每批 10 个 ID、只查 ISAll=false，fresh handles + 每次 IsValid + 首次异常整模块停用。同时修复 RespawnCycleData 命名空间解析，105 已确认 DAILY。

1.0.33：宝箱改走退出后精确 PAK 静态表：PropTreasureBoxData + SectionTreasureBoxData，直接拿 BlueprintPath/GeneratedClass；运行时不再碰 TreasureBoxPropDataMap.Data。RespawnCycle 105 parser 同时按真实 XML 层级修复。

1.0.34：修 BluePrintPath，确认 11 个真实 TreasureBox Blueprint 类；运行时只扫这 11 类并对玩家 25m 内静态点做 Actor 存在/连续缺失诊断。连续 3 次缺失才标 suspected_opened，Actor 重现立即自纠正。RespawnCycle XML 改用 XmlDocument.Load 原始 UTF-8。
1.0.36: Treasure now records exact Actor coordinates before player initialization, preserves 64-bit UIDs as strings, uses independent fast sampling and loaded-zone-gated presence/absence transitions. Assault queued-only checkpoints recover without quarantine; native-phase recovery remains fail-closed. Boss research remains frozen.
1.0.37: Adaptive Treasure sampling replaces full 11-class polling. Nearby points scan only required exact classes; idle sampling rotates one class per pass. This preserves the confirmed presence-to-absence chain while removing the 1.0.36 performance regression.
1.0.38: Treasure runtime research is archived and disabled while all source, catalogs, and historical evidence remain preserved. Assault becomes the only automatic runtime probe and is staggered to one read-only PlaceID query per tick to reduce frame-time spikes. Boss remains frozen.
1.0.39: Assault trigger-state polling is archived after its semantics were confirmed. Runtime returns to a 60-second framework-only health check with no game-object query. Treasure external collectors are also disabled. Remaining Assault research is restricted to availability, DAILY reset, DaySwitch, weather, and time evidence.
1.0.40: Add a bounded 10-second loaded-object comparison for conditioned PlaceID 104 and control PlaceID 126. Only two exact generated monster classes are enumerated. Exact UIDName/CID/static-coordinate matching is recorded; absence remains loaded-region evidence only.
1.0.41: Extend the two-target report with read-only DsEnvironmentManager CurrentWeatherState and CurrentWeatherBTState scalars. Write a new v2 report schema while the production Radar diagnostic records the same 40 target CIDs from tb_actor_respawn.
1.0.42: Add DGameSingleton TimeOfDay/LoginStartTimeOfDay, sky TimeOfDay/RealTimeOfDay, and environment custom-time/teleport scalars. The v3 report distinguishes continuous global time from region-local time or weather changes across teleport.
1.0.43: Replace the completed PlaceID 126 control with PlaceID 109 / CID 148, the only other Assault target whose raw SectionMonster GroupID is zero. Preserve exactly two bounded exact-class scans and write separate v4 reports for special-spawn comparison.
1.0.44: Add a build-pinned post-exit RevealCycleData extractor. Parse RevealIngametime and HideIngametime, isolate rows near the observed CID 143 23:00 reveal boundary, and preserve runtime MonsterSpawnBase TableKey_RevealCycle as the remaining identity-binding step.
1.0.45: Make the external post-exit Monitor fail closed against the exact `DragonSwordWorldDataProbe : 1` entry in `mods.txt`. Disabling the mod while the game is running now stops the Monitor before collection; manual diagnostics remain independent.
1.0.46: Add PlaceID 120 / CID 106 Quaku as an ordinary-spawn control using the observed exact `DsMon_Goblin_s_C` class plus UIDName, CID, and static-coordinate identity constraints. Keep MonsterSpawnBase RevealCycle binding explicitly unresolved.
1.0.47: Add a bounded `DsPCTargetingComponent.LockOnTarget` route for CID 106. Record the actual generated class only after exact UIDName or CID-plus-coordinate validation; do not enumerate Character or Actor.
1.0.48: Enumerate only exact `DsPCTargetingComponent` objects, inspect valid nonempty `LockOnTarget` values, and accept only identity-validated CID 106. Character and Actor enumeration remain disabled.
1.0.49: Record bounded read-only identity summaries for up to three nonempty lock-on targets while retaining strict CID 106 promotion. Sunny, fog, and rain observations all shared weather scalars `2/3`, so those fields are not treated as visible weather type.
1.0.50: Resolve locked defence components through proven `ActorComponent.GetOwner()` and record the valid owner Actor identity. Add bounded object-name evidence for weather actor, current payload, default FX, and forced weather without invoking weather functions or enumerating containers.
1.0.51: Hotfix the 1.0.50 startup crash by disabling runtime dereference of weather actor, current payload, default FX, and forced-weather object references. Retain stable scalar time/raw-weather reads and conditional lock-target owner resolution.
1.0.52: Replace the provisional CID 106 class with runtime-observed `DsMon_Goblin_berserker_Named_C`. The ordinary control can now be identified without lock-on by exact class plus the existing static-coordinate bound; RevealCycle binding remains unresolved.
1.0.53:
- Added bounded `MonsterSpawnBase` discovery for three known Assault targets.
- Reads exact `TableKey.Key` values for monster, spawn condition, respawn cycle, and reveal cycle.
- Caches fixed spawners; incomplete discovery retries at most once per 30 seconds.
- No generic Actor/Character scan, unknown function call, hook, or mutation is used.
1.0.55:
- Added deterministic XOR12 uncompressed PAK entry extraction.
- Confirmed five RevealCycleData rows, including cemetery skeleton hours 23 through 6.
- Preserved CID 143 to cycle 10001 as an evidence-labeled inference pending direct asset binding.
- Runtime remains healthcheck-only; MonsterSpawnBase enumeration remains prohibited.
