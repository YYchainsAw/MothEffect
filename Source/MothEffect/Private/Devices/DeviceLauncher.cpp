#include "Devices/DeviceBase.h"
#include "Devices/LaunchColumn.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "MothEffect.h"
#include "Types/MothCollisionChannels.h"
#include "Feedback/MothFeedbackSubsystem.h"

void ADeviceBase::StartLauncher()
{
	if (!LauncherColumnClass || LauncherColumnClass->HasAnyClassFlags(CLASS_Abstract))
	{
		FailLauncher(ELauncherFailureReason::SpawnFailed);
		return;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothLauncherGround), false, this);
	FCollisionResponseParams Responses(ECR_Ignore);
	Responses.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
	FHitResult Ground;
	if (!GetWorld()->LineTraceSingleByChannel(Ground, LauncherOrigin,
		LauncherOrigin - FVector::UpVector * FMath::Max(1.0f, LauncherGroundProbeDistanceCm),
		MothCollision::GroundTrace, Params, Responses))
	{
		FailLauncher(ELauncherFailureReason::NoGround);
		return;
	}
	const UPrimitiveComponent* Surface = Ground.GetComponent();
	const AActor* GroundActor = Ground.GetActor();
	const FVector Normal = Ground.ImpactNormal.GetSafeNormal();
	const bool bTagged = (IsValid(GroundActor) && GroundActor->ActorHasTag(LauncherSurfaceTag))
		|| (IsValid(Surface) && Surface->ComponentHasTag(LauncherSurfaceTag));
	if (!IsValid(Surface) || Surface->GetCollisionObjectType() != ECC_WorldStatic || Normal.ContainsNaN()
		|| Normal.Z < FMath::Max(0.01f, FMath::Clamp(LauncherGroundUpNormalMin, 0.0f, 1.0f))
		|| (bLauncherRequiresSurfaceTag && !bTagged))
	{
		FailLauncher(ELauncherFailureReason::UnsupportedSurface);
		return;
	}
	const ALaunchColumn* Defaults = LauncherColumnClass.GetDefaultObject();
	// Raise a world-up square base to its highest contact with the local support plane.
	// Do not ignore the ground component: it may also contain the obstructing ceiling.
	const FVector Base = Ground.ImpactPoint + FVector::UpVector * Defaults->GetColumnRadiusCm()
		* (FMath::Abs(Normal.X) + FMath::Abs(Normal.Y)) / Normal.Z;
	if (!Defaults->HasStaticClearance(GetWorld(), Base, this))
	{
		FailLauncher(ELauncherFailureReason::StaticObstruction);
		return;
	}
	const FTransform Transform(FRotator::ZeroRotator, Base);
	ALaunchColumn* Column = GetWorld()->SpawnActorDeferred<ALaunchColumn>(LauncherColumnClass, Transform,
		this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Column || !Column->InitializeColumn(this))
	{
		if (Column) Column->Destroy();
		FailLauncher(ELauncherFailureReason::SpawnFailed);
		return;
	}
	LaunchColumn = Column;
	Column->FinishSpawning(Transform);
	if (!IsValid(Column) || Column->IsActorBeingDestroyed() || !Column->HasActorBegunPlay())
	{
		if (DeviceState == EDeviceState::Active) FailLauncher(ELauncherFailureReason::SpawnFailed);
		return;
	}
#if !UE_BUILD_SHIPPING
	if (bLogDeviceEvents) UE_LOG(LogMothEffect, Log, TEXT("Launcher %s hit=%s origin=%s base=%s column=%s"),
		*GetName(), *ActivationHitId.ToString(), *LauncherOrigin.ToString(), *Base.ToString(), *Column->GetName());
#endif
	if (!bEndingPlay && bGameplayEnabled && DeviceState == EDeviceState::Active)
		GetWorld()->GetSubsystem<UMothFeedbackSubsystem>()->Emit(EMothFeedbackCue::Launcher, Base, FVector::UpVector);
}

void ADeviceBase::FailLauncher(ELauncherFailureReason Reason)
{
	LauncherFailureReason = Reason;
	FinishActivation();
	if (bEndingPlay || IsActorBeingDestroyed() || DeviceState != EDeviceState::Spent) return;
	const TCHAR* Message = Reason == ELauncherFailureReason::NoGround ? TEXT("NO GROUND")
		: Reason == ELauncherFailureReason::UnsupportedSurface ? TEXT("INVALID SURFACE")
		: Reason == ELauncherFailureReason::StaticObstruction ? TEXT("BLOCKED SPACE") : TEXT("COLUMN FAILED");
	LauncherFeedback->SetText(FText::FromString(Message));
	LauncherFeedback->SetVisibility(true);
	GetWorld()->GetSubsystem<UMothFeedbackSubsystem>()->Emit(EMothFeedbackCue::Failure, LauncherOrigin);
#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), LauncherOrigin, 35.0f, 12, FColor::Red, false, LauncherSpentVisualSeconds, 0, 3.0f);
	if (bLogDeviceEvents) UE_LOG(LogMothEffect, Log, TEXT("Launcher %s failed=%s; seed consumed"),
		*GetName(), *UEnum::GetValueAsString(Reason));
#endif
	OnLauncherFailed(Reason);
}

void ADeviceBase::OnLaunchColumnEnded(ALaunchColumn* Column)
{
	if (LaunchColumn != Column) return;
	LaunchColumn = nullptr;
	if (!bEndingPlay && !IsActorBeingDestroyed() && DeviceState == EDeviceState::Active) FinishActivation();
}
