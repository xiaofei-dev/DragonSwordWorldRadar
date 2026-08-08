# 架构

## 两类插件接口

### 1. 运行时 Lua 模块

目录：`scripts/modules/`

用于精确类存在性、属性快照、受限对象样本、UI 样本和精确 Hook。所有模块通过 `probe_manager` 分步执行，并由 checkpoint/quarantine 处理未完成步骤。默认只启用 `healthcheck`。

### 2. 外部 PowerShell/Python 模块

目录：`tools/modules/<id>/Run.ps1`

由 `ModuleHost.ps1` 根据 `config/modules.json` 和活动 profile 调用。每个模块获得隔离 Context、current 输出目录、run 输出目录和统一 result.json。

## 执行触发器

- `monitor_start`：游戏出现后执行一次静态/只读模块。
- `game_exit`：退出后执行存档快照等模块。
- `collect`：生成诊断包前重跑离线模块。
- `manual`：开发者手动调用。

## 默认突击流水线

```text
static_inventory
    → pak_static（精确 compact entry 解码）
    → assault_catalog（精确值关联，不解释语义）
    → pak_decoder_evidence（仅关键文件缺失时）
```

最终 Radar 的动态状态不在本框架中重复实现：直接复用现有 `tb_actor_respawn` tracker。

## 插拔方式

- 新方法：复制 `examples/external-modules/Run.ps1`。
- 新静态主题：复制 `config/static-targets/assault.json`。
- 新运行时探针：复制 `examples/runtime-modules/example_probe.lua`，再显式开启对应 safety flag。

## 配置分层

- `config/suite.json`：游戏布局、Monitor 和诊断策略。
- `config/profiles/*.json`：启用哪些外部模块以及使用哪个 `static_target_profile`。
- `config/static-targets/*.json`：静态 XML 文件、根节点和关联策略。
- `scripts/config.lua`：运行时 Lua 白名单与安全开关。

这种分层允许未来新增宝箱、任务、天气、NPC 或其他主题，而不改 ModuleHost。
