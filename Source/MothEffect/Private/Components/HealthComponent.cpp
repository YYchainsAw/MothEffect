#include "Components/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	Super::EndPlay(EndPlayReason);
}

bool UHealthComponent::ApplyHit(const FHitContext& Context)
{
	if (bEndingPlay || !HasBegunPlay() || !IsAlive() || !GetWorld() || GetWorld()->IsPaused()
		|| !Context.HitId.IsValid() || !FMath::IsFinite(Context.Damage) || Context.Damage <= 0.0f
		|| AcceptedHits.Contains(Context.HitId))
	{
		return false;
	}

	AcceptedHits.Add(Context.HitId);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Context.Damage);
	const bool bFatal = CurrentHealth <= 0.0f;
	bHasDied = bFatal;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	if (bFatal && !bEndingPlay)
	{
		OnDied.Broadcast(GetOwner(), Context.SourceActor.Get());
	}
	return true;
}
