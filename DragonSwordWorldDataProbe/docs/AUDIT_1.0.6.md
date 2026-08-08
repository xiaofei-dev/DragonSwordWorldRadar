# DragonSwordWorldDataProbe 1.0.6 发布前完整审计

## 结论
1.0.5 **不建议作为最终采集基线直接继续使用**。审计发现多项结构性问题，已在 1.0.6 修复。
1.0.6 保留模块化设计、Boss 关闭策略、突击 exact decoder 与 Mole 40 点研究路线。

## 1.0.5 发现并修复的问题

### 高优先级
1. `pak_static` 在 cache hit 时无条件写 `success`，即使 Kind/Place 仍缺失。
   - 修复：缓存结果恢复真实 `success/partial/blocked` 和 blocker。
2. cache key 只依赖 EXE/PAK；decoder/采集器升级后可能错误复用旧 partial cache。
   - 修复：cache fingerprint 加入 `pak-static-v4-assault-minigame`。
3. `ModuleHost` 不清理 `current/result.json`；子模块异常退出时可能读取上一轮旧 success。
   - 修复：每次真实执行前原子清空 current 目录。
4. Monitor 和 Collect 可能同时启动 PAK 流程。
   - 修复：ModuleHost 全局 named mutex，最长等待 20 分钟。
5. 重型 PAK 提取在 `monitor_start` 运行，可能与游戏争用磁盘/CPU。
   - 修复：重模块改到 `game_exit/collect`；游戏运行时只有 Lua healthcheck。

### Mole
6. shared PAK extractor 原先没有 `MiniGameData` / Mole 关键词；Mole 模块虽然复用 cache，却拿不到最关键静态表。
   - 修复：同一次 PAK 扫描加入 MiniGameData、Mole/MiniGame 目标。
7. `MiniGame_Mole_12001–12040` 精确 regex 少了一位数字。
   - 修复：`MiniGame_Mole_120(0[1-9]|[1-3][0-9]|40)`。
8. 代码构建了 12001–12040 数组但没有用于静态数据关联。
   - 修复：对全部成功 XML 的每个字段执行 12001–12040 精确数值反查，输出 crossrefs。
9. `mole_state_discovery` 只在 collect 触发，但 Collect 的必需结果检查未包含它。
   - 修复：所有 Mole 状态研究模块进入 `game_exit/collect` 主流水线。

### 一致性
10. Lua config、Monitor 日志、Collect summary 仍显示旧版本。
    - 修复：统一为 1.0.6。
11. release metadata 的默认 profile/modules 仍是旧 assault-only 信息。
    - 修复：同步为 `assault-mole-research`。

## 当前默认安全面
- `ReceiveDestroyed`：关闭
- `ReceiveEndPlay`：关闭
- `FindAllOf("Character")`：关闭
- 全局 Hook：关闭
- 游戏状态修改：关闭
- Boss 研究监听：关闭
- 游戏运行时重 PAK 扫描：关闭
- 运行时默认模块：healthcheck

## 当前突击路线
`UnexpectedMissionWorld/Place/Kind -> MissionValue -> Monster/Spawn -> Condition/Weather/Respawn -> 现有 tb_actor_respawn`

关键 Kind/Place 如果仍无法精确解码，同一 PAK 结果内保留 decoder evidence；禁止相邻 offset 替代和关键表 mask 穷举。

## 当前 Mole 路线
- 已知节点：`MiniGame_Mole_12001`–`12040` 共 40 个
- 已知类：`DsMiniGameNode_Mole`, `DsMiniGameMoleAsset`, `DsMiniGameMoleComponent`, `DsMiniGameMoleData`, `DMiniGameTable`
- 静态主目标：`MiniGameData.xml` + 所有 12001–12040 数值 crossref
- 最终还需确认：持久完成状态（未完成显示、完成隐藏）

## 静态验证
- JSON：全部可解析
- Python：全部 `.py` 通过 `py_compile`
- Manifest：重建
- ZIP CRC：最终打包后验证
- 危险默认：静态扫描确认关闭

## 验证边界
当前构建环境不能执行 Windows PowerShell 5.1，也不能启动游戏。
因此 1.0.6 仍需要一次 Windows/game runtime gate：
1. 进入游戏；
2. 正常退出；
3. 等 Monitor 完成 post-exit modules；
4. 运行 Collect-Diagnostics；
5. 检查 module result 与诊断 ZIP。
