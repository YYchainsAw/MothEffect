#include "UI/MothDeviceIcon.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

class SMothDeviceIcon : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMothDeviceIcon) {} SLATE_END_ARGS()
	void Construct(const FArguments&) {}
	EDeviceKind Kind = EDeviceKind::Launcher;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(40, 40); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const FVector2D Size = Geometry.GetLocalSize();
		const float Scale = FMath::Min(Size.X, Size.Y) / 40.0f;
		const FVector2D Offset = (Size - FVector2D(40, 40) * Scale) * .5f;
		auto Stroke = [&](std::initializer_list<FVector2D> Points)
		{
			TArray<FVector2D> Vertices; for (FVector2D Point : Points) Vertices.Add(Offset + Point * Scale);
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Vertices,
				ESlateDrawEffect::None, UMothDeviceIcon::GetDeviceColor(Kind), true, 2.5f * Scale);
		};
		if (Kind == EDeviceKind::Launcher)
		{
			Stroke({{5,32},{35,32}}); Stroke({{20,28},{20,8}}); Stroke({{10,18},{20,8},{30,18}});
		}
		else if (Kind == EDeviceKind::Bomb)
		{
			TArray<FVector2D> Circle;
			for (int I=0; I<=24; ++I) { float A=I*2*PI/24; Circle.Add(Offset+(FVector2D(20,20)+FVector2D(FMath::Cos(A),FMath::Sin(A))*10)*Scale); }
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Circle, ESlateDrawEffect::None, UMothDeviceIcon::GetDeviceColor(Kind), true, 2.5f*Scale);
			Stroke({{20,3},{20,7}}); Stroke({{20,33},{20,37}}); Stroke({{3,20},{7,20}}); Stroke({{33,20},{37,20}});
			Stroke({{8,8},{11,11}}); Stroke({{29,29},{32,32}}); Stroke({{8,32},{11,29}}); Stroke({{29,11},{32,8}});
		}
		else
		{
			Stroke({{5,12},{22,12},{22,28},{5,28},{5,12}}); Stroke({{13,20},{36,20}}); Stroke({{28,12},{36,20},{28,28}});
		}
		return Layer;
	}
};

FLinearColor UMothDeviceIcon::GetDeviceColor(EDeviceKind Kind)
{
	return FLinearColor::FromSRGBColor(Kind == EDeviceKind::Launcher ? FColor(83,214,220)
		: Kind == EDeviceKind::Bomb ? FColor(255,152,92) : FColor(181,146,239));
}
FText UMothDeviceIcon::GetDeviceName(EDeviceKind Kind)
{
	return FText::FromString(Kind == EDeviceKind::Launcher ? TEXT("抬升器") : Kind == EDeviceKind::Bomb ? TEXT("爆裂器") : TEXT("发射器"));
}
TSharedRef<SWidget> UMothDeviceIcon::RebuildWidget() { SAssignNew(Icon, SMothDeviceIcon); Icon->Kind=DeviceKind; return Icon.ToSharedRef(); }
void UMothDeviceIcon::SetDeviceKind(EDeviceKind Kind) { DeviceKind=Kind; if (Icon) { Icon->Kind=Kind; Icon->Invalidate(EInvalidateWidgetReason::Paint); } }
void UMothDeviceIcon::ReleaseSlateResources(bool bReleaseChildren) { Super::ReleaseSlateResources(bReleaseChildren); Icon.Reset(); }
