#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Types/DeviceTypes.h"
#include "MothDeviceIcon.generated.h"

/** Load-compatible legacy widget. New UI uses Designer images; this class paints nothing. */
UCLASS()
class MOTHEFFECT_API UMothDeviceIcon : public UWidget
{
	GENERATED_BODY()
public:
	void SetDeviceKind(EDeviceKind Kind);
	static FLinearColor GetDeviceColor(EDeviceKind Kind);
	static FText GetDeviceName(EDeviceKind Kind);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	UPROPERTY(EditAnywhere, Category="Device") EDeviceKind DeviceKind = EDeviceKind::Launcher;
};
