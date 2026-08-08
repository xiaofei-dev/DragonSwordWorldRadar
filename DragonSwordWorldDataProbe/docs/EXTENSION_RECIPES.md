# 扩展配方

## 新增一个静态数据主题

1. 复制 `config/static-targets/assault.json` 为 `<topic>.json`。
2. 为每个 XML 指定 `file`、`required`、`expected_root`、`role`。
3. 复制 profile，并把 `static_target_profile` 改为 `<topic>`。
4. 复用 `pak_static`；若需要关联分析，复制 `assault_catalog` 模块和 Python parser。
5. 所有派生关联必须输出原始表、行号、字段和值。

## 新增外部采集模块

1. 复制 `examples/external-modules/Run.ps1` 到 `tools/modules/<id>/Run.ps1`。
2. 在 `config/modules.json` 注册 ID、触发器、顺序、超时和风险。
3. 在 profile 的 `external_modules` 中显式启用。
4. 只写 `ctx.output_dir` 和 `ctx.run_output_dir`。
5. 最终调用 `Write-ModuleResult`。

## 新增运行时 Lua 模块

1. 复制 `examples/runtime-modules/example_probe.lua`。
2. 使用 `scripts/core/module_api.lua` 构造 class/property/method/custom step。
3. 在 `scripts/config.lua` 注册模块路径和执行顺序。
4. 只有在目标类/属性明确后，才开启相应 safety flag。
5. `FindAllOf` 必须有类白名单和对象数量上限；Hook 必须有精确 UFunction，并禁止 Actor 生命周期通用 Hook。

## 新增存档研究

- 原始快照：启用 `save_snapshot`，只在 `game_exit/collect` 复制 DB/WAL/SHM/BAK/SAV。
- 逻辑表读取：先在独立模块中验证 SQLCipher/游戏构建，再读取复制快照；不要持有或关闭游戏连接。
- 对已有 Radar 状态源，只复用，不并行创建第二个轮询器。
