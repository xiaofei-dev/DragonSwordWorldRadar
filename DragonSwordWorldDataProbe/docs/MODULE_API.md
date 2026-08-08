# 模块接口

## 外部模块

1. 创建 `tools/modules/<id>/Run.ps1`。
2. 在 `config/modules.json` 注册 ID、触发器、顺序、超时和风险等级。
3. 在活动 profile 的 `external_modules` 中加入 ID。
4. 读取 `ContextPath`，调用 `Import-ModuleContext`。
5. 所有输出写入 `ctx.output_dir`；证据日志写入 `ctx.run_output_dir`。
6. 必须调用 `Write-ModuleResult`，状态为 success/partial/blocked/error/skipped。

禁止模块直接修改游戏文件或复用其他模块的临时目录。派生结论必须保留原始证据路径。

## 运行时模块

1. 模块返回 `{id, enabled, steps}`。
2. 在 `scripts/config.lua` 的 `probe_modules` 和 `probe_order` 注册。
3. 默认安全策略不会允许 FindAllOf、属性读取或 Hook；必须显式开启相应 flag。
4. `targeted_hook` 拒绝包含 ReceiveDestroyed/ReceiveEndPlay 的路径。

## 静态目标配置

每个 target 至少包含：

```json
{"file":"UnexpectedMissionKindData.xml","required":true,"expected_root":"UnexpectedMissionKindDataMap","role":"kind"}
```

`expected_root` 是强校验，不匹配即拒绝，防止相邻 PAK entry 冒充目标文件。

## Profile 选择静态主题

活动 profile 使用：

```json
{"static_target_profile":"assault"}
```

`ModuleHost.ps1` 会把它解析成 `config/static-targets/<id>.json` 并传给模块；不再硬编码突击路径。

## 运行结果隔离

ModuleHost 只接受当前 run 目录新生成的 `result.json`，不会把上一次 current 结果误当作本次成功。current 目录仍可供模块自己做构建指纹缓存。
