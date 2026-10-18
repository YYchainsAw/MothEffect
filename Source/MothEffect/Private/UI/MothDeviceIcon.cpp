#include "UI/MothDeviceIcon.h"
#include "Widgets/SNullWidget.h"

FLinearColor UMothDeviceIcon::GetDeviceColor(EDeviceKind Kind)
{
	return FLinearColor::FromSRGBColor(Kind == EDeviceKind::Launcher ? FColor(83,214,220)
		: Kind == EDeviceKind::Bomb ? FColor(255,152,92) : FColor(181,146,239));
}
FText UMothDeviceIcon::GetDeviceName(EDeviceKind Kind)
{
	return FText::FromString(Kind == EDeviceKind::Launcher ? TEXT("抬升器") : Kind == EDeviceKind::Bomb ? TEXT("爆裂器") : TEXT("发射器"));
}
TSharedRef<SWidget> UMothDeviceIcon::RebuildWidget() { return SNullWidget::NullWidget; }
void UMothDeviceIcon::SetDeviceKind(EDeviceKind Kind) { DeviceKind=Kind; }
