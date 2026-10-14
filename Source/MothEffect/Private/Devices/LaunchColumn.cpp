#include "Devices/LaunchColumn.h"
#include "Devices/DeviceBase.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/ImpulseReceiver.h"
#include "Types/MothCollisionChannels.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool DeviceBodyIntersectsBox(const ADeviceBase* Device, const UBoxComponent* Box)
	{
		const USphereComponent* Body = Device ? Device->GetPhysicsBody() : nullptr;
		if (!IsValid(Body) || !IsValid(Box)) return false;
		const FVector Offset = Body->GetComponentLocation() - Box->GetComponentLocation();
		const FVector Extent = Box->GetUnscaledBoxExtent();
		const FVector Outside(
			FMath::Max(0.0, FMath::Abs(Offset.X) - Extent.X),
			FMath::Max(0.0, FMath::Abs(Offset.Y) - Extent.Y),
			FMath::Max(0.0, FMath::Abs(Offset.Z) - Extent.Z));
		return Outside.SizeSquared() <= FMath::Square(Body->GetScaledSphereRadius());
	}
}

ALaunchColumn::ALaunchColumn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	ColumnRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ColumnRoot"));
	SetRootComponent(ColumnRoot);
	ColumnVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ColumnVisual"));
	ColumnVisual->SetupAttachment(ColumnRoot);
	ColumnVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ColumnVisual->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	if (Cube.Succeeded()) ColumnVisual->SetStaticMesh(Cube.Object);
	RiseQuery = CreateDefaultSubobject<UBoxComponent>(TEXT("RiseQuery"));
	RiseQuery->SetupAttachment(ColumnRoot);
	RiseQuery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RiseQuery->SetCollisionResponseToAllChannels(ECR_Ignore);
	RiseQuery->SetGenerateOverlapEvents(false);
	RiseQuery->SetCanEverAffectNavigation(false);
	SolidCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("SolidCollider"));
	SolidCollider->SetupAttachment(ColumnRoot);
	SolidCollider->SetCollisionObjectType(ECC_WorldDynamic);
	SolidCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SolidCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	for (ECollisionChannel Channel : {ECC_WorldStatic, ECC_WorldDynamic, ECC_Pawn, ECC_PhysicsBody,
		ECC_Visibility, MothCollision::Projectile}) SolidCollider->SetCollisionResponseToChannel(Channel, ECR_Block);
	SolidCollider->SetGenerateOverlapEvents(false);
	SolidCollider->SetCanEverAffectNavigation(false);
	StageLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StageLabel"));
	StageLabel->SetupAttachment(ColumnRoot);
	StageLabel->SetHorizontalAlignment(EHTA_Center);
	StageLabel->SetWorldSize(24.0f);
	StageLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALaunchColumn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateVisual(bInitialized ? VisibleHeight : GetColumnHeightCm());
}

bool ALaunchColumn::InitializeColumn(ADeviceBase* Source)
{
	if (bInitialized || !IsValid(Source) || Source->GetDeviceState() != EDeviceState::Active
		|| Source->GetDeviceKind() != EDeviceKind::Launcher || Source->GetWorld() != GetWorld()) return false;
	SourceDevice = Source;
	bInitialized = true;
	ImpulseEventId = FGuid::NewGuid();
	SetActorRotation(FRotator::ZeroRotator);
	SetActorScale3D(FVector::OneVector);
	return true;
}

void ALaunchColumn::BeginPlay()
{
	Super::BeginPlay();
	if (!bInitialized || !SourceDevice.IsValid()) { Destroy(); return; }
	SolidCollider->SetBoxExtent(FVector(GetColumnRadiusCm(), GetColumnRadiusCm(), GetColumnHeightCm() * 0.5f));
	SolidCollider->SetRelativeLocation(FVector(0.0, 0.0, GetColumnHeightCm() * 0.5f));
	RiseQuery->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UpdateVisual(0.0f);
	UpdatePresentation();
}

bool ALaunchColumn::IsSolidSupportEnabled() const
{
	return SolidCollider->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics;
}

bool ALaunchColumn::HasStaticClearance(UWorld* World, const FVector& Base, const AActor* IgnoredSource) const
{
	if (!World || Base.ContainsNaN()) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothColumnStatic), false, IgnoredSource);
	Params.AddIgnoredActor(this);
	FCollisionResponseParams Responses(ECR_Ignore);
	Responses.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
	const float Bottom = GetGroundContactToleranceCm();
	const float HalfHeight = (GetColumnHeightCm() - Bottom) * 0.5f;
	return !World->OverlapBlockingTestByChannel(Base + FVector(0.0, 0.0, Bottom + HalfHeight),
		FQuat::Identity, ECC_WorldDynamic,
		FCollisionShape::MakeBox(FVector(GetColumnRadiusCm(), GetColumnRadiusCm(), HalfHeight)), Params, Responses);
}

bool ALaunchColumn::HasDynamicClearance() const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothColumnOccupancy), false, this);
	if (SourceDevice.IsValid()) Params.AddIgnoredActor(SourceDevice.Get());
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, SolidCollider->GetComponentLocation(), FQuat::Identity,
		Objects, FCollisionShape::MakeBox(SolidCollider->GetUnscaledBoxExtent()), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const ADeviceBase* Device = Cast<ADeviceBase>(Overlap.GetActor());
		// A frozen root is absent from collision queries, but must not be enclosed by support.
		if (IsValid(Device) && !Device->IsActorBeingDestroyed() && Device->IsGrounded()
			&& Device->CanReceiveExternalImpulse() && DeviceBodyIntersectsBox(Device, SolidCollider))
			return false;
		const UPrimitiveComponent* Component = Overlap.GetComponent();
		if (IsValid(Component) && Component->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block)
			return false;
	}
	return true;
}

void ALaunchColumn::QueryLaunchTargets(float Height)
{
	if (Height <= 0.0f || Phase != ELaunchColumnPhase::Rising) return;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothColumnRise), false, this);
	if (SourceDevice.IsValid()) Params.AddIgnoredActor(SourceDevice.Get());
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	TArray<FOverlapResult> Overlaps;
	// The full reached volume covers the vertical rise path and late entrants behind the tip.
	GetWorld()->OverlapMultiByObjectType(Overlaps, RiseQuery->GetComponentLocation(), FQuat::Identity,
		Objects, FCollisionShape::MakeBox(RiseQuery->GetUnscaledBoxExtent()), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!IsValid(Target) || Target->IsActorBeingDestroyed() || AffectedActors.Contains(Target)
			|| !Target->Implements<UImpulseReceiver>()) continue;
		const ACharacter* Character = Cast<ACharacter>(Target);
		const ADeviceBase* Device = Cast<ADeviceBase>(Target);
		const UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Target->GetRootComponent());
		// Grounded devices are found by their shot sphere; settle range against the real body.
		if (Device)
		{
			if (!Device->CanReceiveExternalImpulse() || !DeviceBodyIntersectsBox(Device, RiseQuery)) continue;
		}
		else if (Overlap.GetComponent() != Root) continue;
		if (Character)
		{
			const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
			const AMothEffectCharacter* Player = Cast<AMothEffectCharacter>(Target);
			if (Character->GetCharacterMovement()->MovementMode == MOVE_None || (Health && !Health->IsAlive())
				|| (Player && !Player->IsGameplayEnabled())) continue;
		}
		else if (!Device && (!Root || !Root->IsSimulatingPhysics())) continue;
		AffectedActors.Add(Target);
		IImpulseReceiver::Execute_ReceiveImpulse(Target,
			FVector(0.0, 0.0, FMath::Max(0.0f, Character ? CharacterUpVelocityCmPerSec : DeviceUpVelocityCmPerSec)),
			true, ImpulseEventId);
		if (bEndingPlay || IsActorBeingDestroyed() || Phase != ELaunchColumnPhase::Rising) return;
	}
}

void ALaunchColumn::UpdateVisual(float Height)
{
	VisibleHeight = FMath::Clamp(Height, 0.0f, GetColumnHeightCm());
	const float NonzeroHeight = FMath::Max(0.01f, VisibleHeight);
	ColumnVisual->SetRelativeScale3D(FVector(GetColumnRadiusCm() * 0.02f, GetColumnRadiusCm() * 0.02f, NonzeroHeight * 0.01f));
	ColumnVisual->SetRelativeLocation(FVector(0.0, 0.0, VisibleHeight * 0.5f));
	ColumnVisual->SetVisibility(VisibleHeight > 0.0f);
	RiseQuery->SetBoxExtent(FVector(GetColumnRadiusCm(), GetColumnRadiusCm(), NonzeroHeight * 0.5f));
	RiseQuery->SetRelativeLocation(FVector(0.0, 0.0, VisibleHeight * 0.5f));
	StageLabel->SetRelativeLocation(FVector(0.0, 0.0, VisibleHeight + 25.0f));
}

void ALaunchColumn::UpdatePresentation()
{
	const bool bSolid = IsSolidSupportEnabled();
	const TCHAR* Label = Phase == ELaunchColumnPhase::Rising ? TEXT("RISING")
		: Phase == ELaunchColumnPhase::Holding ? (bSolid ? TEXT("SOLID") : TEXT("WAIT"))
		: Phase == ELaunchColumnPhase::Retracting ? TEXT("RETRACT") : TEXT("");
	StageLabel->SetText(FText::FromString(Label));
	StageLabel->SetTextRenderColor(bSolid ? FColor::Green : FColor(160, 160, 160));
	ColumnVisual->SetVectorParameterValueOnMaterials(TEXT("Color"), bSolid ? FVector(0.1, 0.8, 0.2) : FVector(0.65));
	OnColumnPresentationChanged(Phase, bSolid);
}

void ALaunchColumn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInitialized || bEndingPlay || IsActorBeingDestroyed() || GetWorld()->IsPaused()) return;
	if (!SourceDevice.IsValid()) { Destroy(); return; }
	if (Phase == ELaunchColumnPhase::Rising || Phase == ELaunchColumnPhase::Holding)
	{
		Elapsed += FMath::Max(0.0f, DeltaSeconds);
		const double Rise = FMath::Max(0.01f, RiseSeconds);
		if (Phase == ELaunchColumnPhase::Rising)
		{
			UpdateVisual(GetColumnHeightCm() * FMath::Clamp(Elapsed / Rise, 0.0, 1.0));
			QueryLaunchTargets(VisibleHeight);
			if (bEndingPlay || IsActorBeingDestroyed() || Phase != ELaunchColumnPhase::Rising) return;
			if (Elapsed < Rise) return;
			Phase = ELaunchColumnPhase::Holding;
			RiseQuery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UpdatePresentation();
			if (bEndingPlay || IsActorBeingDestroyed() || Phase != ELaunchColumnPhase::Holding) return;
		}
		const double HoldEnd = Rise + FMath::Max(0.01f, HoldSeconds);
		if (Elapsed >= HoldEnd)
		{
			BeginRetraction();
			RetractionElapsed = Elapsed - HoldEnd;
		}
		else if (!IsSolidSupportEnabled() && HasStaticClearance(GetWorld(), GetActorLocation(), SourceDevice.Get())
			&& HasDynamicClearance())
		{
			SolidCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			UpdatePresentation();
		}
	}
	else if (Phase == ELaunchColumnPhase::Retracting) RetractionElapsed += FMath::Max(0.0f, DeltaSeconds);
	if (bEndingPlay || IsActorBeingDestroyed() || Phase != ELaunchColumnPhase::Retracting) return;
	UpdateVisual(RetractionStartHeight * (1.0 - FMath::Clamp(RetractionElapsed / GetRetractSeconds(), 0.0, 1.0)));
	if (RetractionElapsed >= GetRetractSeconds()) FinishColumn();
}

void ALaunchColumn::BeginRetraction()
{
	if (bEndingPlay || Phase == ELaunchColumnPhase::Retracting || Phase == ELaunchColumnPhase::Finished) return;
	// Disable support and queries before presentation callbacks or mesh movement.
	DisableSolidSupport();
	RiseQuery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RetractionStartHeight = VisibleHeight;
	RetractionElapsed = 0.0;
	Phase = ELaunchColumnPhase::Retracting;
	UpdatePresentation();
}

void ALaunchColumn::FinishColumn()
{
	DisableSolidSupport();
	RiseQuery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Phase = ELaunchColumnPhase::Finished;
	SetActorTickEnabled(false);
	if (SourceDevice.IsValid()) SourceDevice->OnLaunchColumnEnded(this);
	SourceDevice.Reset();
	Destroy();
}

void ALaunchColumn::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	DisableSolidSupport();
	RiseQuery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (SourceDevice.IsValid()) SourceDevice->OnLaunchColumnEnded(this);
	SourceDevice.Reset();
	AffectedActors.Reset();
	Super::EndPlay(Reason);
}

void ALaunchColumn::DisableSolidSupport()
{
	const bool bWasSolid = IsSolidSupportEnabled();
	SolidCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (!bWasSolid || !GetWorld()) return;
	// Stationary walking can retain a cached floor until movement resumes. Release the
	// actual support base explicitly so retraction cannot leave a character suspended.
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		ACharacter* Character = *It;
		if (Character->GetMovementBaseObject() != SolidCollider) continue;
		Character->SetBase(static_cast<FMovementBaseInterfaceData*>(nullptr));
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		Movement->bForceNextFloorCheck = true;
		if (Movement->IsMovingOnGround()) Movement->SetMovementMode(MOVE_Falling);
	}
}
