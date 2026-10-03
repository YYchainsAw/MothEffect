#include "Weapons/Rifle.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Interfaces/BallisticReactive.h"

ARifle::ARifle()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(SceneRoot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(SceneRoot);
	Muzzle->SetRelativeLocation(FVector(60.0f, 0.0f, 0.0f));
}

void ARifle::BeginPlay()
{
	Super::BeginPlay();
	MagazineSize = FMath::Max(1, MagazineSize);
	FireIntervalSeconds = FMath::Max(0.01f, FireIntervalSeconds);
	ReloadSeconds = FMath::Max(0.01f, ReloadSeconds);
	RangeCm = FMath::Max(1.0f, RangeCm);
	AmmoInMagazine = MagazineSize;
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARifle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	StopFire();
	CancelReload();
	Super::EndPlay(EndPlayReason);
}

AMothEffectCharacter* ARifle::GetPlayerOwner() const
{
	return Cast<AMothEffectCharacter>(GetOwner());
}

bool ARifle::TryStartFire()
{
	AMothEffectCharacter* Player = GetPlayerOwner();
	if (bEndingPlay || !Player || !Player->CanFireRifle() || bReloading || !GetWorld())
	{
		return false;
	}
	if (AmmoInMagazine <= 0)
	{
		OnEmptyMagazine.Broadcast(this);
		return false;
	}
	if (bFireRequested)
	{
		return true;
	}

	bFireRequested = true;
	TryFireOneShot();
	ScheduleNextShot();
	return true;
}

void ARifle::StopFire()
{
	bFireRequested = false;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireTimer);
	}
	// Preserve NextShotTime so repeated mouse presses cannot bypass the fire interval.
}

void ARifle::HandleFireTimer()
{
	TryFireOneShot();
	ScheduleNextShot();
}

void ARifle::ScheduleNextShot()
{
	AMothEffectCharacter* Player = GetPlayerOwner();
	if (bEndingPlay || !bFireRequested || bReloading || AmmoInMagazine <= 0
		|| !Player || !Player->CanFireRifle() || !GetWorld())
	{
		StopFire();
		return;
	}
	const float Delay = FMath::Max(UE_KINDA_SMALL_NUMBER,
		static_cast<float>(NextShotTime - GetWorld()->GetTimeSeconds()));
	GetWorld()->GetTimerManager().SetTimer(FireTimer, this, &ARifle::HandleFireTimer, Delay, false);
}

void ARifle::TryFireOneShot()
{
	AMothEffectCharacter* Player = GetPlayerOwner();
	UWorld* World = GetWorld();
	if (bEndingPlay || !bFireRequested || bReloading || !World || !Player || !Player->CanFireRifle())
	{
		StopFire();
		return;
	}
	if (World->GetTimeSeconds() < NextShotTime)
	{
		return;
	}
	if (AmmoInMagazine <= 0)
	{
		StopFire();
		return;
	}

	FVector ViewLocation;
	FVector ViewDirection;
	if (!Player->GetRifleView(ViewLocation, ViewDirection))
	{
		StopFire();
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothRifleTrace), true, Player);
	Params.AddIgnoredActor(this);
	TArray<AActor*> AttachedActors;
	Player->GetAttachedActors(AttachedActors, true, true);
	Params.AddIgnoredActors(AttachedActors);
	FHitResult CameraHit;
	const FVector ViewEnd = ViewLocation + ViewDirection * RangeCm;
	const bool bCameraHit = World->LineTraceSingleByChannel(CameraHit, ViewLocation, ViewEnd, WeaponTraceChannel, Params);
	const FVector AimPoint = bCameraHit ? CameraHit.ImpactPoint : ViewEnd;
	const FVector MuzzleLocation = Muzzle->GetComponentLocation();
	const FVector ShotDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	FHitResult SafetyHit;
	FCollisionQueryParams SafetyParams = Params;
	SafetyParams.bTraceComplex = false;
	const bool bSafetyHit = World->LineTraceSingleByChannel(SafetyHit,
		Player->GetRifleSafetyOrigin(), MuzzleLocation, WeaponTraceChannel, SafetyParams);
	const bool bMuzzleOverlap = World->OverlapBlockingTestByChannel(MuzzleLocation, FQuat::Identity,
		WeaponTraceChannel, FCollisionShape::MakeSphere(FMath::Max(0.1f, MuzzleClearanceRadiusCm)), SafetyParams);
	const bool bMuzzleBlocked = bSafetyHit || bMuzzleOverlap || ShotDirection.IsNearlyZero()
		|| FVector::DotProduct(ShotDirection, ViewDirection) <= 0.0f;

	// A blocked muzzle still consumes a shot, as specified by W01.
	NextShotTime = World->GetTimeSeconds() + FireIntervalSeconds;
	--AmmoInMagazine;
	FHitResult ShotHit;
	const FVector ShotEnd = MuzzleLocation + ShotDirection * FMath::Min(RangeCm,
		FVector::Distance(MuzzleLocation, AimPoint) + MuzzleClearanceRadiusCm);
	const bool bHit = !bMuzzleBlocked && World->LineTraceSingleByChannel(ShotHit,
		MuzzleLocation, ShotEnd, WeaponTraceChannel, Params);
	FHitContext Context;
	Context.HitId = FGuid::NewGuid();
	Context.SourceActor = this;
	Context.InstigatorPawn = Player;
	Context.ImpactPoint = bHit ? ShotHit.ImpactPoint : ShotEnd;
	Context.ShotDirection = ShotDirection;
	Context.Damage = FMath::Max(0.0f, Damage);

#if !UE_BUILD_SHIPPING
	if (bDrawDebugShots)
	{
		DrawDebugLine(World, ViewLocation, AimPoint, FColor::Cyan, false, FireIntervalSeconds);
		DrawDebugLine(World, Player->GetRifleSafetyOrigin(), MuzzleLocation, FColor::Yellow, false, FireIntervalSeconds);
		DrawDebugLine(World, MuzzleLocation, Context.ImpactPoint,
			bMuzzleBlocked ? FColor::Orange : FColor::Red, false, FireIntervalSeconds);
	}
#endif

	Player->CancelSprintUntilRelease();
	AActor* HitActor = ShotHit.GetActor();
	if (bHit && IsValid(HitActor) && HitActor->GetClass()->ImplementsInterface(UBallisticReactive::StaticClass()))
	{
		IBallisticReactive::Execute_ReceiveBallisticHit(HitActor, Context);
	}
	if (bEndingPlay)
	{
		return;
	}
	OnAmmoChanged.Broadcast(AmmoInMagazine, MagazineSize);
	if (!bEndingPlay)
	{
		OnShotFired.Broadcast(this);
	}
	if (bMuzzleBlocked && !bEndingPlay)
	{
		OnMuzzleBlocked.Broadcast(this);
	}
}

bool ARifle::TryBeginReload()
{
	AMothEffectCharacter* Player = GetPlayerOwner();
	if (bEndingPlay || bReloading || !GetWorld() || !Player || !Player->CanReloadRifle()
		|| AmmoInMagazine >= MagazineSize)
	{
		return false;
	}
	StopFire();
	bReloading = true;
	ActiveReloadSeconds = FMath::Max(0.01f, ReloadSeconds);
	const uint32 TaskId = ++ReloadTaskId;
	FTimerDelegate Completion = FTimerDelegate::CreateWeakLambda(this, [this, TaskId]()
	{
		FinishReload(TaskId);
	});
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, Completion, ActiveReloadSeconds, false);
	OnReloadStarted.Broadcast(this);
	return true;
}

void ARifle::FinishReload(uint32 TaskId)
{
	if (bEndingPlay || !bReloading || TaskId != ReloadTaskId)
	{
		return;
	}
	AMothEffectCharacter* Player = GetPlayerOwner();
	if (!Player || !Player->IsGameplayEnabled() || Player->GetActionState() != EPlayerActionState::Reloading)
	{
		CancelReload();
		return;
	}
	bReloading = false;
	++ReloadTaskId;
	AmmoInMagazine = MagazineSize;
	OnAmmoChanged.Broadcast(AmmoInMagazine, MagazineSize);
	if (!bEndingPlay)
	{
		OnReloadFinished.Broadcast(true);
	}
}

void ARifle::CancelReload()
{
	if (!bReloading)
	{
		return;
	}
	bReloading = false;
	++ReloadTaskId;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
	}
	if (!bEndingPlay)
	{
		OnReloadFinished.Broadcast(false);
	}
}

float ARifle::GetReloadProgress() const
{
	if (!bReloading || !GetWorld() || ActiveReloadSeconds <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(GetWorld()->GetTimerManager().GetTimerElapsed(ReloadTimer) / ActiveReloadSeconds, 0.0f, 1.0f);
}
