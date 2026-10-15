#include "Components/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Subsystems/DeviceLifecycleSubsystem.h"
#include "Feedback/MothFeedbackSubsystem.h"
#include "GameFramework/Pawn.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UHealthComponent::ConfigureMaxHealth(float Value)
{
	if (HasBegunPlay() || bEndingPlay || !FMath::IsFinite(Value) || Value < 1.0f) return false;
	MaxHealth = Value;
	CurrentHealth = Value;
	return true;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	AcceptedHits.Reset();
	bHasDied = false;
	bEndingPlay = false;
}

void UHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	AcceptedHits.Reset();
	OnDeathCommittedNative.Clear();
	Super::EndPlay(EndPlayReason);
}

bool UHealthComponent::ApplyHit(const FHitContext& Context)
{
	const auto* Session = GetWorld() ? GetWorld()->GetSubsystem<UDeviceLifecycleSubsystem>() : nullptr;
	if (Session && !Session->IsGameplayEnabled()) return false;
	if (bEndingPlay || !HasBegunPlay() || !IsAlive() || !GetWorld() || GetWorld()->IsPaused()
		|| !Context.HitId.IsValid() || !FMath::IsFinite(Context.Damage) || Context.Damage <= 0.0f
		|| AcceptedHits.Contains(Context.HitId))
	{
		return false;
	}

	const uint64 LifecycleGeneration = Session ? Session->GetLifecycleGeneration() : 0;
	const float PreviousHealth = CurrentHealth;
	AcceptedHits.Add(Context.HitId);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Context.Damage);
	const float RemainingHealth = CurrentHealth;
	const bool bFatal = CurrentHealth <= 0.0f;
	bHasDied = bFatal;
	if (bFatal) OnDeathCommittedNative.Broadcast(GetOwner(), Context.SourceActor.Get());
	if (bEndingPlay) return true;
	if (UMothFeedbackSubsystem* Feedback = GetWorld()->GetSubsystem<UMothFeedbackSubsystem>())
	{
		Feedback->ReportConfirmedEnemyHit(GetOwner(), Context, PreviousHealth - RemainingHealth,
			RemainingHealth, bFatal, LifecycleGeneration);
	}
	if (bEndingPlay) return true;
	const auto* VictimPawn = Cast<APawn>(GetOwner());
	const auto* InstigatorPawn = Context.InstigatorPawn.Get();
	// Feedback observers may reset the run without destroying this health owner.
	// Finish the committed damage/death below, but never emit old effects in the new run.
	if (Session && Session->IsGenerationCurrent(LifecycleGeneration))
	{
		UMothFeedbackSubsystem* Feedback = GetWorld()->GetSubsystem<UMothFeedbackSubsystem>();
		const bool bLocalConfirmedHit = IsValid(InstigatorPawn) && InstigatorPawn != VictimPawn
			&& InstigatorPawn->IsPlayerControlled() && InstigatorPawn->IsLocallyControlled();
		Feedback->Emit(EMothFeedbackCue::Hit, Context.ImpactPoint, Context.ShotDirection,
			bLocalConfirmedHit, false);
		if (Session->IsGenerationCurrent(LifecycleGeneration)
			&& VictimPawn && VictimPawn->IsPlayerControlled() && VictimPawn->IsLocallyControlled())
		{
			Feedback->Emit(EMothFeedbackCue::Damage, Context.ImpactPoint, Context.ShotDirection, false, true);
		}
	}
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	if (bFatal && !bEndingPlay)
	{
		OnDied.Broadcast(GetOwner(), Context.SourceActor.Get());
	}
	return true;
}
