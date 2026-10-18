#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MothTutorialConfig.generated.h"

/** Editable teaching copy and world-space placement. Default values preserve T18's arena. */
UCLASS(BlueprintType)
class MOTHEFFECT_API UMothTutorialConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 1|Copy", meta=(MultiLine=true))
	FText BombAndLiftInstruction = FText::FromString(TEXT("射击橙色爆裂器，炸毁方块；再射击青色抬升器，把箱子抬起。爆炸也会伤到自己，保持距离。"));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 1|Placement") FVector BombLocation = FVector(-2200,-850,40);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 1|Placement") FVector BlastTargetLocation = FVector(-2020,-850,40);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 1|Placement") FVector LauncherLocation = FVector(-1700,-1200,40);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 1|Placement") FVector LiftTargetLocation = FVector(-1700,-1200,130);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 2|Copy", meta=(MultiLine=true))
	FText AirborneInstruction = FText::FromString(TEXT("E 拾取紫色发射器，左键投出。松开左键，恢复后重新按下，射击空中道具。启动时瞄准范围内最近的敌人；没有敌人时沿来弹的水平方向发射。方向锁定后不跟随准星；失败会补充。"));
	/** Local player-space offset: X forward, Y right, Z up; applied again on each retry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 2|Placement") FVector EmitterPlayerOffset = FVector(140,0,-50);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 3|Copy", meta=(MultiLine=true))
	FText IndirectInstruction = FText::FromString(TEXT("沿紫色箭头从左向右射击发射器，让它的弹丸启动前方爆裂器，间接炸毁方块。方向锁定后，移动准星不会改变连锁。"));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 3|Placement") FVector ChainEmitterLocation = FVector(-2200,-1200,40);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 3|Placement") FVector ChainBombLocation = FVector(-1600,-1200,40);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 3|Placement") FVector ChainTargetLocation = FVector(-1420,-1200,40);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 3|Placement") FVector ArrowLocation = FVector(-2520,-1200,80);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Step 3|Placement") FRotator ArrowRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback") FText ResetFeedback = FText::FromString(TEXT("已重置本步道具。"));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback") FText InvalidAirHitFeedback = FText::FromString(TEXT("请先投出，再射击空中的发射器。已补充道具。"));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Feedback") FText RefillFeedback = FText::FromString(TEXT("已补充发射器；松开左键后，再试一次。"));
};
