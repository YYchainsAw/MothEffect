#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Types/DeviceTypes.h"
#include "MothDeviceIcon.generated.h"

/** Resolution independent UMG glyph: colour AND silhouette distinguish the three devices. */
UCLASS()
class MOTHEFFECT_API UMothDeviceIcon : public UWidget
{
	GENERATED_BODY()
public:
	virtual void SynchronizeProperties() override;
	void SetDeviceKind(EDeviceKind Kind);
	static FLinearColor GetDeviceColor(EDeviceKind Kind);
	static FText GetDeviceName(EDeviceKind Kind);
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	UPROPERTY(EditAnywhere, Category="Device") EDeviceKind DeviceKind = EDeviceKind::Launcher;
	TSharedPtr<class SMothDeviceIcon> Icon;
};
