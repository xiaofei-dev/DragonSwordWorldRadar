# 历史采集版本与归并结果

本文件把此前分散的采集 Mod 归并为当前模块，而不是要求继续安装旧版本。

| 历史方向/版本 | 得到的有效信息 | 失败或边界 | 在 1.0.0 中的位置 |
|---|---|---|---|
| Boss 静态探针 / `WorldBossStaticProbe` | PAK 索引、AES、Oodle、FieldBoss/SectionMonster 静态目录 | 早期脚本只覆盖遇到过的 compact entry | `pak_static`、`PakCore.ps1`、legacy 脚本 |
| Boss 存档监控 | `tb_actor_respawn` 与刷新规则，现有 Radar 已投入使用 | 构建相关、不能重复建立第二套 tracker | 现有 Radar 保持权威；源码在 `tools/experimental/save_database` |
| 任务 Controller/任务 UI 探针 | 确认任务类、列表和 Clear 控件存在 | 未得到世界界面可用的完整全局完成列表 | legacy Lua，仅作历史证据 |
| Actor 生命周期 Hook | 能触发对象销毁事件 | `ReceiveDestroyed/ReceiveEndPlay` 不是击杀信号，并复现原生崩溃 | 默认拒绝，legacy 隔离 |
| 全量 Character/对象扫描 | 当前已加载区域样本 | World Partition 不覆盖全地图；可能掉帧 | `bounded_object_snapshot`，默认关闭、必须白名单 |
| DPS/伤害事件思路 | 单次战斗目标和死亡线索 | 完整 DPS 聚合/UI 很重，且 Boss 已无需该路线 | 不作为默认模块 |
| 任务完成状态路线 | “完成=当天不再显示”的语义 | 读取入口未完整；对于当前目标可由 Actor 持久状态替代 | 不属于最终突击链 |
| SQLCipher 常见密码/原生连接实验 | 排除了简单常见密钥；保留原生原型 | 未形成可直接加载的通用 DLL；ABI 风险 | `native/experimental` |
| 通用 UnrealPak/repak 搜索 | 验证可作为备用解包方法 | 本机未安装；该游戏已有专用 PakReaderCore+ooz | 不作为主依赖 |
| 突击 0.1 静态 PAK 探针 | 找到正式 World/Place/Kind 表，成功提取多个支持表 | Kind/Place compact entry 未解出 | `pak_static` 与 target profile |
| 突击 0.2 mask/邻接恢复 | 暴露 mask=27 等线索 | 邻接 entry 产生 XML 假阳性 | 明确禁止；脚本仅保存在 legacy |
| 突击 0.3 目标校验 | 阻止相邻 XML 冒充目标 | 仍未解决标准 compact entry | 被精确解码器取代 |
| 模块化 0.7 | 初步整合静态、存档、运行时和分析模块 | 接口/配置分散，PAK 解码仍不完整 | 1.0.0 架构来源 |

## 归并后的唯一主线

```text
静态 PAK 全集
  + 精确条件/目标关联
  + 现有存档动态状态 tracker
  + 必要时的白名单运行时验证
  = 可验证、低开销、全地图可用的数据链
```
