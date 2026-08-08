# 安全策略

默认 profile 的运行时只执行 healthcheck。外部静态模块只读 EXE/PAK。

明确禁止：
- 通用 `Actor:ReceiveDestroyed`、`Actor:ReceiveEndPlay` Hook；
- `FindAllOf("Character")` 或未知大类全量枚举；
- 未知 `TMap/TArray/UScriptStruct` 猜测遍历；
- 修改任务、Actor、数据库或游戏文件；
- 邻接 PAK entry 替代目标文件；
- 无变化时高频写 JSON/文件；
- 用 F7/F8 控制数据监听生命周期。

实验模块放在 `scripts/experimental`、`tools/experimental`、`native/experimental`，不会被默认 profile 加载。
