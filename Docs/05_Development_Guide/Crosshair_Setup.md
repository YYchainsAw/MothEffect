# Moth Effect：自定义准星、敌人命中与击杀反馈接入

版本 v0.2 · 2026-10-16 · 状态：用户准星已制作并保留在当前 Slate HUD；以下为原始接入说明

2026-10-16 当前界面已改用 [Slate_UI_Setup](Slate_UI_Setup.md)。用户的 `WBP_MothCrosshair` 包保持 SHA 一致，HUD 内 15 个事件节点及引脚/连接保持；真实敌人死亡专项确认用户击杀反馈被触发，暂停、继续与返回标题复位已验证。准星仍只作为 HUD 的一个嵌入子控件，未重复 Add to Viewport。原生文字准星、命中 X 和旧 `NativePaint` 绘制已移除，下文关于关闭原生绘制开关与保留旧必需 `CrosshairText` 的步骤是历史记录，不适用于当前树。原工程构建已通过，无需按本页历史状态重新制作现有蓝图；手动射击手感与完整整局仍待验收。

以下各节记录 2026-10-14 接口交付时的分工、表现建议及未运行状态，不覆盖上面的当前验证结果。

本轮按用户要求先交付 C++ 和接入说明，不运行 UE、编译或修改 `.uasset`。`WBP_MothCrosshair` 由用户在编辑器制作；本文中的动画时长、尺寸和颜色是表现起点，不改战斗数值。现有 HUD、菜单、教学和按钮继续使用 [UI_Setup](UI_Setup.md) 的接口。

## 1. 先加载接口与保存贴图

1. 用户已在编辑器导入三张贴图，目录按用户指定为 `/Game/MothEffect/UI/Textures`。保存这三张贴图及当前蓝图，确保编辑器工作已落盘。
2. Save All 后关闭 UE，用户自行完整编译 `MothEffectEditor / Development Editor / Win64`，成功后重新打开工程。新增反射事件与结构体需要加载新代码，不把旧编辑器会话或 Live Coding 中缺少事件当作蓝图配置问题。
3. 打开 `/Game/MothEffect/UI/WBP_MothHUD`，父类保持 `MothHUDWidget`。Event Graph 中应能添加本页四个 `On ...` 事件；出现节点只证明反射接口已加载，实际命中、动画和暂停仍待测试。
4. 主地图使用 `/Game/MothEffect/Maps/L_Arena_01`。若整个 HUD 都不出现，先按 [Murdock_Setup](Murdock_Setup.md) 检查实际玩家的一体枪设置和 Rifle 是否有效；创建准星子控件不能修复玩家/导演未就绪导致的 HUD 初始化失败。

## 2. 原生事件与反馈范围

`UMothHUDWidget` 向它的 Widget Blueprint 提供以下表现事件。蓝图只播放图像反馈，不再扣血、判死亡或用敌人 Destroyed 推断击杀。

| Blueprint 事件 | 输入 | 用途 |
| --- | --- | --- |
| `On Enemy Hit Confirmed` | `FMothEnemyHitFeedback Feedback` | 本地玩家归属的敌人有效伤害；根据 `bKilled` 播放普通命中或击杀 |
| `On Crosshair Feedback Reset` | 无 | 玩家来源变化、教学换步/重置或离开战斗时清理动画/计时器/临时状态；Victory 保留最后一击 |
| `On Crosshair Pause Changed` | `bool bPaused` | 暂停/续播准星动画 |
| `On Crosshair Page Changed` | `EMothUIPage Page` | 切换普通准星和短击杀反馈的显隐 |

事件的 C++ 名分别为 `OnEnemyHitConfirmed`、`OnCrosshairFeedbackReset`、`OnCrosshairPauseChanged`、`OnCrosshairPageChanged`，均由原生父类调用。`Feedback` 可用 `Break Moth Enemy Hit Feedback` 拆开：

| 字段 | 含义 |
| --- | --- |
| `HitId` | Guid；本次真实命中的 ID |
| `VictimId` | Integer；被命中敌人的 ID，敌人已清理后仍可用于本次反馈关联 |
| `AppliedDamage` | 实际扣除的生命值 |
| `RemainingHealth` | 伤害提交后的剩余生命 |
| `bKilled` | 本次伤害是否首次致死；致死事件只发一次 |
| `ImpactPoint` | 本次命中的世界位置；中心准星反馈可以不使用它 |

此事件只认 `EnemyBase` 的有效伤害，包含本地玩家归属机关及其连锁造成的伤害。打墙、射击 Dormant 机关、普通教学靶、被拒绝/重复的命中或敌人自身伤害不触发本事件。同一 `HitId` 对同一目标去重；一颗爆炸影响不同敌人可以分别反馈，不把整次爆炸当作只能反馈一个目标。普通命中与击杀使用同一入口，蓝图根据 `bKilled` 分支，无需另外订阅死亡或销毁事件。

## 3. 创建三图层准星子控件

在 `/Game/MothEffect/UI` 新建 **Widget Blueprint → User Widget**，命名 `WBP_MothCrosshair`。它不需要新的 C++ 父类，也不自动创建或 Add to Viewport；稍后作为 `WBP_MothHUD` 的子控件使用。

1. 根使用 Canvas Panel，命名 `CrosshairRoot`。设为 **Not Hit-Testable (Self & All Children)**，避免遮挡菜单鼠标和焦点。
2. 添加三个 Image，全部勾选 **Is Variable**。锚点都设屏幕中心，Alignment 设 `(0.5, 0.5)`；Brush Image 分别使用用户保存的普通准星、命中标记和击杀图标。
3. 按下表配置。Image 的 Color and Opacity Alpha 保持 1，淡入淡出只改 **Render Opacity**，避免两份 Alpha 相乘。

| Image 名 | 用途 | 位置 | 建议起步尺寸 | 初始 Render Opacity | 层级 |
| --- | --- | --- | --- | --- | --- |
| `CH` | 白色普通准星 | X=0、Y=0 | 24×24 | 1；页面事件再控制显隐 | Z=0 |
| `HM` | 中心命中标记；普通白色、击杀红色 | X=0、Y=0 | 36×36 | 0 | Z=1 |
| `KI` | 击杀图标 `T_KI_001`；Tint 白色，保持原图颜色 | X=0、Y=40 | 28×28 | 0 | Z=2 |

尺寸是 UMG 设计单位，可按实际贴图透明边界调整；不要通过 Actor 或相机偏移对齐准星。`CH` 只在 Gameplay / Tutorial 页面常驻；这里的“常驻”不包括标题、暂停或结果页。

## 4. 两个 UMG 动画

在子控件 Animations 面板创建以下动画，均只播放一次，不勾选循环或 Restore State。使用 Image 的 Render Opacity 轨道，结尾明确归零。

| 动画 | 总时长 | 轨道与关键帧 |
| --- | --- | --- |
| `HitFeedback` | 0.14 秒 | `HM`：0 秒 Opacity=1，0.04 秒仍为 1，0.14 秒为 0；可加从 1.15 到 1 的 Render Scale |
| `KillFeedback` | 0.60 秒 | `HM`：0 秒为 1，0.10 秒仍为 1，0.30 秒为 0，0.60 秒仍为 0；`KI`：0 秒为 1，0.35 秒仍为 1，0.60 秒为 0 |

`KillFeedback` 的命中标记显示 0.3 秒、下方图标显示 0.6 秒。动画本身不修改颜色；播放事件将 HM 设为红色，KI 的 Tint 保持白色以显示原图颜色，避免普通动画和击杀动画同时争写色值。

## 5. 子控件事件与状态

在 `WBP_MothCrosshair` 创建变量：`bKillActive`、`bPaused`、`CurrentPage`（`Moth UI Page` 枚举）、`KillEndTimerHandle`（Timer Handle）、`PausedAnimationTime`（Float）和 `PausedAnimation`（Widget Animation 引用）。需要区分“没有待续播动画”时可另外用 Boolean `bHasPausedAnimation`。

### PlayHitFeedback

创建自定义事件 `PlayHitFeedback`：

```text
若 bPaused，或 CurrentPage 不是 Gameplay / Tutorial：返回
若 bKillActive：返回
Stop Animation(HitFeedback)
HM → Set Color and Opacity(白色，Alpha=1)
Play Animation(HitFeedback, Start At Time=0, Num Loops=1, Restore State=false)
```

普通命中不打断正在显示的红色击杀。连续普通命中先停止旧 HitFeedback，再从头播放；不另接 Delay 隐藏标记。

### PlayKillFeedback

创建自定义事件 `PlayKillFeedback`：

```text
允许页面：Gameplay / Tutorial / Victory；其他页面或 bPaused 时返回
Clear and Invalidate Timer by Handle(KillEndTimerHandle)
Stop Animation(HitFeedback)
Stop Animation(KillFeedback)
bKillActive = true
HM → Set Color and Opacity(红色，Alpha=1)
KI → Set Color and Opacity(白色，Alpha=1；保持 T_KI_001 原图颜色)
Play Animation(KillFeedback, Start At Time=0, Num Loops=1, Restore State=false)
Set Timer by Event(Time=0.6, Looping=false) → 保存 Return Value 到 KillEndTimerHandle
Timer 回调 → FinishKillFeedback
```

`FinishKillFeedback` 令 `bKillActive=false`，HM / KI 的 Render Opacity 都归零；若当前为 Victory，结束后折叠整个子控件。再次击杀必须先清旧 Timer Handle 并从头刷新动画和 0.6 秒计时。不要用多个 Delay：旧 Delay 可能在第二次击杀尚未播完时把新图标关闭。Timer 使用当前 World 的游戏时间，世界暂停时不提前结束。

### SetCrosshairPage(Page)

创建带 `Moth UI Page` 输入的事件 `SetCrosshairPage`，先保存 `CurrentPage`，再按枚举处理：

| 页面 | 子控件行为 |
| --- | --- |
| Gameplay / Tutorial | 子控件为 Not Hit-Testable (Self & All Children)，CH 显示；HM / KI 保持动画状态 |
| Victory | CH 归零；若 `bKillActive` 则保留红色 HM 与原色 KI 并继续 KillFeedback；否则停止普通 HitFeedback、HM 归零并隐藏整个子控件 |
| Pause | CH 归零，隐藏整个子控件，但不重置 `bKillActive` 或清除 Timer；由暂停事件冻结动画 |
| Title / TutorialComplete / Defeat | 调用 `ResetStopAll`，再隐藏整个子控件 |

Victory 分支保留已经开始的击杀计时和 KI；HM 若仍处于击杀动画前 0.3 秒，应保留它当前的红色透明度。最后一名敌人致死时，菜单可在同一帧转成 Victory，所以 `PlayKillFeedback` 也接受 Victory；进入 Victory 时不调用 ResetStopAll。

### SetCrosshairPaused(bPaused)

创建带 Boolean 输入的事件 `SetCrosshairPaused`，避免对相同值重复处理：

1. 进入暂停前判断 `Is Animation Playing(KillFeedback)`，否则判断 HitFeedback。对实际播放的动画调用 `Pause Animation`，将返回的暂停位置保存到 `PausedAnimationTime`，动画引用保存到 `PausedAnimation`，设置 `bHasPausedAnimation=true`。
2. 保存 `bPaused=true`。不要用 Stop Animation 代替 Pause，也不清 KillEndTimerHandle；世界游戏时间会冻结该 Timer。
3. 继续时先保存 `bPaused=false`。若有暂停动画，用 `Play Animation(PausedAnimation, Start At Time=PausedAnimationTime, Num Loops=1, Restore State=false)` 续播，然后清暂停引用/标记。页面事件会恢复 Gameplay / Tutorial 的普通准星，Victory 仍只保留短击杀。

UMG 动画不应仅依靠 World Pause 推断自己已暂停；这条表现事件显式暂停动画。不要重新从 0 秒播放或重设整段 Timer，以免每次暂停都延长反馈。

### ResetStopAll

创建事件 `ResetStopAll`：Clear and Invalidate Timer by Handle → Stop All Animations → `bKillActive=false` → 清暂停动画引用/位置/标记 → HM / KI Render Opacity=0。CH 按 `CurrentPage` 为 Gameplay / Tutorial 时显示，其余隐藏；暂停时仍隐藏。子控件 Event Destruct 同样调用此清理，避免离开关卡后保留旧 Timer 回调。

## 6. 接入 WBP_MothHUD

1. 在 `WBP_MothHUD` Designer 中将 `WBP_MothCrosshair` 拖到 **RootOverlay** 下，命名 `CrosshairWidget`，勾选 Is Variable。它必须与 `HUD`、`Menu` 同级，不能放进其中之一。Overlay Slot 的水平/垂直对齐设 Fill，让它覆盖全画面；把它排在 Menu 后面，使短击杀图标在结果面板之上。如果用户已换成 Canvas 根，使用独立全屏层并将 Z Order 设在菜单之上。
2. 子控件及全部图像保持 Not Hit-Testable，不能抢菜单按钮的输入。
3. 保留原必需 `CrosshairText` 的名字、TextBlock 类型和 Is Variable，仅设 Visibility=Collapsed。不要删除/改名，原生 `BindWidget` 仍要绑定它。原准星不会再显示，新 Image 使用自己的视觉。
4. Class Defaults → **Moth / Feedback**：关闭 **Draw Default Hit Marker**（`bDrawDefaultHitMarker`），保留 **Draw Default Combat Feedback**（`bDrawDefaultCombatFeedback=true`），这样关闭原生白色命中 X，仍保留原生受伤边框。只有打算一起重做受伤边框时才关闭后者。
5. 在父控件 Event Graph 添加以下转发，Target 都是 Designer 中的 `CrosshairWidget` 引用，不使用 Get All Widgets Of Class，也不再次 Create Widget / Add to Viewport。

```text
Event On Enemy Hit Confirmed(Feedback)
  → Break Moth Enemy Hit Feedback
  → Branch(bKilled)
      True  → CrosshairWidget.PlayKillFeedback
      False → CrosshairWidget.PlayHitFeedback

Event On Crosshair Feedback Reset
  → CrosshairWidget.ResetStopAll

Event On Crosshair Pause Changed(bPaused)
  → CrosshairWidget.SetCrosshairPaused(bPaused)

Event On Crosshair Page Changed(Page)
  → CrosshairWidget.SetCrosshairPage(Page)
```

6. 编译并保存子控件、父控件和贴图。运行时四个事件由原生 HUD 通知；静态 Designer 预览可用临时预览值调位置，但不能据此记录实际伤害/死亡验收。

## 7. 当前待执行验收

本轮只有源码与说明交付，没有 C++ 编译结果、蓝图交付或 PIE 通过记录。用户完成编译和上面蓝图后，按下列实际范围记录：

1. **显示与普通命中**：教学/战斗中心 CH 常驻；射击正式敌人一次出现白 HM 约 0.14 秒。连续命中从头刷新；打墙、射击机关、普通靶或无效命中不出现此敌人反馈。
2. **致死与优先级**：击杀正式敌人显示红 HM 约 0.3 秒和下方 KI 约 0.6 秒。普通命中其他敌人不能覆盖正播放的红 Kill；连续击杀从头刷新，旧 Timer 不提前关闭新图标。玩家归属 D02 / P01 / 连锁击杀同样正确，非玩家来源不误报。
3. **最后一敌与菜单**：末波最后一名敌人致死进入 Victory，白 CH 隐藏，但红色 HM 与原色 KI 的短反馈可播完，之后隐藏；菜单按钮仍可点击和键盘操作。Title、TutorialComplete、Defeat 不残留命中标记。
4. **暂停与重开**：命中/击杀显示中 Esc 暂停，等待超过 0.6 秒，继续后从剩余位置和剩余时间完成，不从头重播。教学重置、返回标题、失败/重开和反复进入 PIE 后无旧动画、Timer 或重复响应。
5. **已有反馈与尺寸**：仅关闭 Draw Default Hit Marker 后原生受伤边框继续工作，无双白 X；1280×720、1920×1080、2560×1440 的三图层位置正确，无焦点遮挡。

新增接口涉及 [EnemyHitFeedback.h](../../Source/MothEffect/Public/Types/EnemyHitFeedback.h)、[MothHUDWidget.h](../../Source/MothEffect/Public/UI/MothHUDWidget.h)、[MothHUDWidget.cpp](../../Source/MothEffect/Private/UI/MothHUDWidget.cpp) 与 [MothFeedbackSubsystem.h](../../Source/MothEffect/Public/Feedback/MothFeedbackSubsystem.h)。验收要记录用户实际编译版本、保存资产、用例与结果，不把已写源码或旧 T19 验证扩展为本轮通过。
