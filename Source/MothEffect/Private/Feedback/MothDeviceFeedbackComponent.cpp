#include "Feedback/MothDeviceFeedbackComponent.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "Devices/DeviceBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"

UMothDeviceFeedbackComponent::UMothDeviceFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick=true;
	PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void UMothDeviceFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	const auto* Feedback=GetWorld()->GetSubsystem<UMothFeedbackSubsystem>();
	const auto* Profile=Feedback?Feedback->GetProfile():nullptr;
	if (!Profile) return;
	if (auto* Base=Profile->Material.LoadSynchronous()) Material=UMaterialInstanceDynamic::Create(Base,this);
	UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 I=0; I<7; ++I)
	{
		auto* Mesh=NewObject<UStaticMeshComponent>(GetOwner(),NAME_None,RF_Transient);
		Mesh->SetupAttachment(GetOwner()->GetRootComponent()); Mesh->SetAbsolute(true,true,true);
		Mesh->SetStaticMesh(Cube); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false); Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false);
		if (Material) Mesh->SetMaterial(0,Material);
		Mesh->RegisterComponent(); Pieces.Add(Mesh);
	}
	RefreshPresentation();
}
void UMothDeviceFeedbackComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	for (const auto& Mesh : Pieces) if (IsValid(Mesh)) Mesh->DestroyComponent();
	Pieces.Reset(); Material=nullptr; Super::EndPlay(Reason);
}
void UMothDeviceFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime,TickType,ThisTickFunction); RefreshPresentation();
}
void UMothDeviceFeedbackComponent::RefreshPresentation()
{
	const auto* Device=Cast<ADeviceBase>(GetOwner());
	const auto* Feedback=GetWorld()?GetWorld()->GetSubsystem<UMothFeedbackSubsystem>():nullptr;
	const auto* Profile=Feedback?Feedback->GetProfile():nullptr;
	const auto* Session=GetWorld()?GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>():nullptr;
	const bool bShow=bEnabled&&Device&&Profile&&Profile->bEnabled&&Profile->bWorldVisuals&&Session&&Session->IsGameplayEnabled()
		&&Device->GetDeviceState()!=EDeviceState::Held&&Device->GetDeviceState()!=EDeviceState::Destroyed&&!Device->IsActorBeingDestroyed();
	for (const auto& Mesh : Pieces) Mesh->SetVisibility(false);
	if (!bShow||Pieces.Num()!=7) return;
	const EDeviceState State=Device->GetDeviceState(); const EDeviceKind Kind=Device->GetDeviceKind();
	const EMothFeedbackCue Cue=Kind==EDeviceKind::Launcher?EMothFeedbackCue::Launcher:Kind==EDeviceKind::Bomb?EMothFeedbackCue::Bomb:EMothFeedbackCue::Emitter;
	const auto* Style=Profile->Styles.Find(Cue); if (!Style) return;
	const bool bSpent=State==EDeviceState::Spent, bWarning=Device->IsExpiryWarningActive();
	const float Now=GetWorld()->GetTimeSeconds();
	const float Pulse=bWarning?.65f+.35f*FMath::Sin(Now*16):State==EDeviceState::Active?.8f+.2f*FMath::Sin(Now*7):.65f;
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"),bSpent?Profile->SpentColor:bWarning?Profile->WarningColor:Style->Color);
		Material->SetScalarParameterValue(TEXT("Intensity"),FMath::Clamp(Profile->EmissiveStrength,.1f,8.f)*Pulse);
	}
	const FVector Origin=Device->GetActorLocation()+FVector::UpVector*FMath::Clamp(Profile->MarkerHeightCm,1.f,200.f);
	FRotator Facing=FRotator::ZeroRotator;
	if (const auto* PC=GetWorld()->GetFirstPlayerController())
	{
		FVector View; FRotator ViewRotation; PC->GetPlayerViewPoint(View,ViewRotation);
		Facing.Yaw=(View-Origin).Rotation().Yaw;
	}
	const float Size=FMath::Clamp(Profile->MarkerSizeCm,1.f,200.f)*(bSpent?.65f:bWarning?1+.12f*FMath::Sin(Now*16):1);
	auto Bar=[this](int32 I, FVector Position, FRotator Rotation, FVector Scale)
	{
		Pieces[I]->SetWorldLocationAndRotation(Position,Rotation); Pieces[I]->SetWorldScale3D(Scale); Pieces[I]->SetVisibility(true);
	};
	if (bSpent)
	{
		for (int32 I=0; I<2; ++I) Bar(I,Origin,(Facing.Quaternion()*FRotator(0,0,45+I*90).Quaternion()).Rotator(),FVector(.03f,Size/100,.03f));
	}
	else if (Kind==EDeviceKind::Bomb)
	{
		for (int32 I=0; I<4; ++I)
		{
			const FRotator R(0,0,45+I*90);
			Bar(I,Origin+Facing.RotateVector(R.RotateVector(FVector(0,0,Size*.36f))),
				(Facing.Quaternion()*R.Quaternion()).Rotator(),FVector(.03f,Size*.7f/100,.03f));
		}
	}
	else
	{
		// Up arrow for D01; side arrow for D03. Geometry remains distinct without color.
		const FRotator GlyphRotation=(Facing.Quaternion()*FRotator(0,0,Kind==EDeviceKind::Launcher?90:0).Quaternion()).Rotator();
		Bar(0,Origin,GlyphRotation,FVector(.03f,Size/100,.03f));
		for (int32 I=1; I<3; ++I)
		{
			const FRotator Head(0,0,I==1?45:-45);
			Bar(I,Origin+GlyphRotation.RotateVector(FVector(0,Size*.4f,(I==1?1:-1)*Size*.16f)),
				(GlyphRotation.Quaternion()*Head.Quaternion()).Rotator(),FVector(.03f,Size*.45f/100,.03f));
		}
	}
	if (Kind==EDeviceKind::Emitter&&State==EDeviceState::Active)
	{
		const FVector Direction=Device->GetActivationDirection(); const FRotator Rotation=Direction.Rotation();
		const float Length=FMath::Clamp(Profile->DirectionLengthCm,10.f,250.f);
		const FVector Start=Device->GetActorLocation()+Direction*32;
		Bar(4,Start+Direction*(Length*.5f),Rotation,FVector(Length/100,.035f,.035f));
		for (int32 I=5; I<7; ++I)
			Bar(I,Start+Rotation.RotateVector(FVector(Length-8,(I==5?1:-1)*6,0)),
				(Rotation.Quaternion()*FRotator(0,I==5?35:-35,0).Quaternion()).Rotator(),FVector(.22f,.04f,.04f));
	}
}
