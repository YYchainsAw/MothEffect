# Moth Effect：T21 Windows 内部测试包与验收

版本 v0.1 · 2026-10-13 · 状态：Development包、包内自动化与干净解压启动通过；真人操作/录屏待验收

T21 使用当前可运行原型做独立 Windows 内部测试。构建、Cook、Pak/IoStore、归档、压缩/重新解压和包内程序均已实际执行；详细结果见 [T21 报告](../06_Test_Doc/T21_Report_2026-10-13.md)。这是供你继续改机制/UI的内部基线，正式候选包与平台提交分别由 T25/T27 验收。

## 现在如何试玩

当前完整压缩包：

`Builds/T21/MothEffect_T21_20261013_220904_1dbde1e.zip`

已经重新解压并验证的启动入口：

`Builds/T21/Extracted_20261013_220904_1dbde1e/Windows/MothEffect.exe`

关闭 Unreal Editor 后双击该入口即可；也可把 ZIP 完整解压到自己的新目录，运行其中 `Windows/MothEffect.exe`。保留整套 Windows 文件，不能只复制单个 EXE。新电脑若缺运行库，包内 `Engine/Extras/Redist/en-us` 提供已归档的 UEPrereq 安装程序。

默认启动为主竞技场标题页，先选“开始教学”，完成三步后进入战斗，最后通关三波。当前界面与模型均可由你后续替换；这个包用于验收当前代码/资产组合，修改源项目后需要重新构建。

## 当前包与提交的关系

本包为 UE5.8.0-55116800 / Win64 Development，代码构建基线 `1dbde1e`，玩法参数 v0.18。之后的 PR 记录文档提交不改变打包输入；构建前后跟踪的 Source/Config/Content/MothEffect 输入 SHA256 无变化。

本包读取了你当前未提交的 `BP_MothPlayer.uasset`，SHA256 为 `A3DE76DA7F3440B048CA5EA9668E41F4D11414D7F2E9CCBA31743777584D0AF8`。该蓝图仍留在工作区，未保存、暂存或推入 [PR #47](https://github.com/YYchainsAw/MothEffect/pull/47)。因此包体验证范围与 PR 的纯提交树分别记录；最终候选包应先确认并冻结实际玩家资产版本。

完整 ZIP 校验：`419291d94201172a9df1a81e1b4d86bc37c5ce601252d658c5138745b6f2977d`。包内四十四件运行文件压缩与解压 SHA256 一致，运行产生的 `MothEffect/Saved` 未混入 ZIP；[包清单](../06_Test_Doc/Evidence/T21_Package_Manifest_2026-10-13.json) 记录各文件、尺寸和哈希。

## 你需要完成的四组操作

以下均**待用户验收**，使用该包、实际 Murdock 玩家和正常窗口/声音，建议保存完整操作录像。

1. **从标题到完整通关**：中文/图标/按钮无裁切，真实鼠标/键盘可用；实际完成爆炸/抬升、投出后空中启动、P01→D02间接连锁的教学。进入正式战斗后通关4/6/8三波，观察批次空窗、警戒圈占位延迟、固定掉落和胜利页。对应 TC20–TC22。
2. **玩家与弹药组合**：走动/跳跃/瞄准/开火/换弹/携带组合；持物左键只投掷，恢复后松键再射击；空弹匣不自动换弹，R恢复，Held/满匣不换弹；近墙枪口、持物、相机与上身动画正确。对应 TC24、TC34–TC38。
3. **暂停、死亡与重开**：持物、Active D03、柱子、弹丸或换弹时暂停/恢复、切出窗口/回来；战斗死亡与教学死亡正常结束。胜利/失败/暂停返回标题，至少连续重开十次，无旧对象、错误波数、重复事件或继续射击。对应 TC23与玩家专项。
4. **退出与重新启动**：标题、失败、胜利状态退出，程序不残留；从同一 EXE 再开，回到干净标题。核对音效听感、方向/过期警告、瞄准遮挡与当前机器表现。对应 TC25；自动 benchmark退出不能替代按按钮退出。

当前自动化已在真正打包 EXE 中通过十二项，覆盖三步教学、波次/胜负/十次地图重载、主场地物理/导航及表现清理。夹具部分使用原生测试玩家或直接提交入口，不把它写成实际键鼠/角色操作录像；外部盲玩 TC26 留 T22，通过前完整 G3 保持待验收。

## 重建与日志

在工程根目录运行 [build_windows_test.ps1](../../scripts/build_windows_test.ps1)。先保存并关闭 Editor；脚本构建 Development Game、Cook主地图、Stage/Archive，记录提交、未提交路径、前后输入 SHA256 与 UAT退出码。默认新建带香港时间与提交号的输出目录，不覆盖原测试包。

实际首次记录：`Saved/Automation/T21/Package_20261013_220904`，其中 `BuildInputs.json`、`BuildResult.json` 与 `UATConsole.log` 为原始证据。包内十二项测试的 `Packaged1/index.json` 与 `Packaged1.log` 留在 `Saved/Automation/T21`；原始环境错误与告警不删除。

[archive_windows_test.py](../../scripts/archive_windows_test.py) 对完整 Windows 目录压缩、重新解压并逐文件核对，排除运行 Saved，不删除已有目标。脚本参数见 `--help`，构建记录通过 `--build-inputs` 指定；使用新的 ZIP 与解压目录。

当前 Cook 有8条原有 Rig/旧输入提示；包内测试导出 errors=0/warnings=82。原始包内日志另有 HTML报告模板未归档的诊断 error，原生JSON报告仍成功生成；不影响游戏资源，查看 [归档日志](../06_Test_Doc/Evidence/T21_Verification_2026-10-13.log) 的实际范围。Murdock动画除零/空粒子提示与历史 Paragon缺依赖恢复边界继续保留；首次打包通过不等于新机器完整源工程恢复或资源授权核验已完成。

用户完成四组操作后再根据实测更新 [#23](https://github.com/YYchainsAw/MothEffect/issues/23)。后续改机制/UI时同步实际配置、规则文档与受影响回归；T14–T20 原待验收范围不自动扩展通过。
