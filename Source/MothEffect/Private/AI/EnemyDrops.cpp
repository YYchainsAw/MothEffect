#include "AI/EnemyBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Devices/DeviceBase.h"
#include "Devices/DeviceReleaseSafety.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "MothEffect.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"

void AEnemyBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshDropAppearance();
}

void AEnemyBase::RefreshDropAppearance()
{
	const ADeviceBase* Defaults = DropDeviceClass ? DropDeviceClass.GetDefaultObject() : nullptr;
	const FMothPrototypeAppearance Visual = Defaults ? Defaults->GetVisualAppearance() : FMothPrototypeAppearance();
	UStaticMesh* AppearanceMesh = Visual.Mesh.LoadSynchronous();
	const bool bConfigured = AppearanceMesh && Defaults && Defaults->GetDeviceKind() == DropKind;
	BackpackMesh->SetVisibility(bConfigured);
	DropHintText->SetVisibility(bConfigured);
	if (!bDropAttempted) DropResult = DropDeviceClass ? EEnemyDropResult::Ready : EEnemyDropResult::Unconfigured;
	if (!bConfigured) return;
	BackpackMesh->SetStaticMesh(AppearanceMesh);
	BackpackMesh->SetRelativeScale3D(Visual.RelativeTransform.GetScale3D() * 1.4f);
	BackpackMesh->SetRelativeRotation(Visual.RelativeTransform.Rotator());
	UMothPrototypeArtProfile::ApplyMaterials(BackpackMesh,Visual);
	const TCHAR* Hint = DropKind == EDeviceKind::Launcher ? TEXT("D01 / UP")
		: DropKind == EDeviceKind::Bomb ? TEXT("D02 / BLAST") : TEXT("D03 / FIRE");
	const FColor Color = DropKind == EDeviceKind::Launcher ? FColor::Cyan
		: DropKind == EDeviceKind::Bomb ? FColor::Orange : FColor(180, 75, 255);
	DropHintText->SetText(FText::FromString(Hint));
	DropHintText->SetTextRenderColor(Color);
}

bool AEnemyBase::FindDropTransform(const ADeviceBase* Defaults, const FVector& DeathLocation, FTransform& OutTransform) const
{
	const USphereComponent* Body = Defaults->GetPhysicsBody();
	if (!Body || DeathLocation.ContainsNaN() || !FMath::IsFinite(DropSearchRadiusCm)
		|| !FMath::IsFinite(DropGroundProbeDistanceCm) || !FMath::IsFinite(DropClearanceCm)
		|| !FMath::IsFinite(DropGroundUpNormalMin)) return false;
	FCollisionQueryParams Birth(SCENE_QUERY_STAT(MothEnemyDropBirth), false, this);
	FCollisionQueryParams Ground(SCENE_QUERY_STAT(MothEnemyDropGround), false, this);
	// Unstable round devices are obstacles, not supporting ground. Columns remain eligible.
	for (TActorIterator<ADeviceBase> It(GetWorld()); It; ++It) Ground.AddIgnoredActor(*It);
	FCollisionObjectQueryParams Surfaces;
	Surfaces.AddObjectTypesToQuery(ECC_WorldStatic); Surfaces.AddObjectTypesToQuery(ECC_WorldDynamic);
	const float Clearance = FMath::Max(0.01f, DropClearanceCm);
	const float Radius = Defaults->GetConfiguredBodyRadiusCm() * Body->GetShapeScale();
	const float Ring = FMath::Max(0.0f, DropSearchRadiusCm - Radius - Clearance);
	TArray<FVector> Offsets { FVector::ZeroVector };
	for (float Factor : { 0.5f, 1.0f })
		for (int32 I = 0; I < 8; ++I)
		{
			const float Angle = I * PI / 4;
			Offsets.Add(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Ring * Factor);
		}
	for (const FVector& Offset : Offsets)
	{
		const FVector Probe = DeathLocation + Offset;
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByObjectType(Hit, Probe + FVector::UpVector * 50,
			Probe - FVector::UpVector * FMath::Max(1.0f, DropGroundProbeDistanceCm), Surfaces, Ground)
			|| Hit.ImpactNormal.Z < FMath::Clamp(DropGroundUpNormalMin, 0.0f, 1.0f)) continue;
		const FVector Position = Hit.ImpactPoint + Hit.ImpactNormal * (Radius + Clearance);
		if (MothDeviceRelease::IsPathClear(GetWorld(), Body, DeathLocation, Position, Birth, Radius))
		{ OutTransform = FTransform(FRotator::ZeroRotator, Position); return true; }
	}
	// A high airborne death may have no nearby support. A free physical birth falls normally;
	// it must never be attached/frozen or relocated across an obstacle to find ground.
	if (MothDeviceRelease::IsPathClear(GetWorld(), Body, DeathLocation, DeathLocation, Birth, Radius))
	{ OutTransform = FTransform(FRotator::ZeroRotator, DeathLocation); return true; }
	return false;
}

void AEnemyBase::TryDropDevice(TSubclassOf<ADeviceBase> Class, EDeviceKind Kind, const FVector& DeathLocation, uint64 Generation, uint64 LifeGeneration)
{
	if (!Class) return;
	UDeviceLifecycleSubsystem* Lifecycle = GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>();
	if (bPoolInactive || LifeGeneration != PoolLifeGeneration) return;
	if (bEndingPlay || IsActorBeingDestroyed() || !Lifecycle || !Lifecycle->IsGenerationCurrent(Generation))
	{ DropResult = EEnemyDropResult::Cancelled; return; }
	const ADeviceBase* Defaults = Class.GetDefaultObject();
	if (Class->HasAnyClassFlags(CLASS_Abstract) || !Defaults || Defaults->GetDeviceKind() != Kind)
	{ DropResult = EEnemyDropResult::InvalidClass; return; }
	FTransform Transform;
	if (!FindDropTransform(Defaults, DeathLocation, Transform))
	{ DropResult = EEnemyDropResult::NoSafeLocation; return; }
	ADeviceBase* Device = Lifecycle->SpawnDeviceForGeneration(Class, Transform, Generation, [this, Defaults, DeathLocation, Transform, LifeGeneration]
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MothEnemyDropCommit), false, this);
		return !bPoolInactive && LifeGeneration == PoolLifeGeneration && !bEndingPlay && !IsActorBeingDestroyed()
			&& MothDeviceRelease::IsPathClear(GetWorld(), Defaults->GetPhysicsBody(), DeathLocation, Transform.GetLocation(), Params,
				Defaults->GetConfiguredBodyRadiusCm() * Defaults->GetPhysicsBody()->GetShapeScale());
	});
	if (bPoolInactive || LifeGeneration != PoolLifeGeneration)
	{
		if (IsValid(Device)) Device->DestroyDevice();
		return;
	}
	if (Device && (Device->GetDeviceKind() != Kind || Device->GetDeviceState() != EDeviceState::Dormant
		|| (!Device->IsGrounded() && !Device->GetPhysicsBody()->IsSimulatingPhysics()))) { Device->DestroyDevice(); Device = nullptr; }
	DroppedDevice = Device;
	DropResult = Device ? EEnemyDropResult::Spawned
		: (!Lifecycle->IsGenerationCurrent(Generation) || bEndingPlay || IsActorBeingDestroyed()) ? EEnemyDropResult::Cancelled : EEnemyDropResult::Rejected;
#if !UE_BUILD_SHIPPING
	if (bLogEnemyEvents) UE_LOG(LogMothEffect, Log, TEXT("Enemy %s drop=%s result=%s device=%s death=%s position=%s attempts=%d total=%d"),
		*GetName(), *UEnum::GetValueAsString(Kind), *UEnum::GetValueAsString(DropResult), *GetNameSafe(Device),
		*DeathLocation.ToString(), Device ? *Device->GetActorLocation().ToString() : TEXT("none"), GetDropAttempts(), Lifecycle->GetDeviceCount());
#endif
}
