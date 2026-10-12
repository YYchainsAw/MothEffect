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
	void SetDeviceKind(EDeviceKind Kind);
	static FLinearColor GetDeviceColor(EDeviceKind Kind);
	static FText GetDeviceName(EDeviceKind Kind);
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	EDeviceKind DeviceKind = EDeviceKind::Launcher;
	TSharedPtr<class SMothDeviceIcon> Icon;
};
