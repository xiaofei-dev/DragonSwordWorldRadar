# 当前可靠结论

- Boss 功能已经完成，不属于本次修改范围。
- Boss 的成功模型是静态目录 + `tb_actor_respawn`，不是任务 UI 或死亡 Hook。
- 突击使用独立的 UnexpectedMission World/Place/Kind 静态表。
- 当前主要技术点是准确提取 Kind/Place compact entries，并进行精确值关联。
- 天气条件可能由通用 `AcceptConditionType/Value*`、SpawnCondition、DataLayer 或 switch 表表达；在得到实际行前不下结论。
- 运行时内存无法覆盖未加载的世界分区，不能作为全图清单或全局存活状态来源。
