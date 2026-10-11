#include "Devices/DeviceBase.h"
#include "Characters/MothEffectCharacter.h"
#include "Components/HealthComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "MothEffect.h"

void ADeviceBase::ExplodeBomb(const FHitContext& Context)
{
	struct FTarget
	{
		TWeakObjectPtr<AActor> Actor;
		FVector Point;
		FVector Velocity;
	};
	const float Radius = FMath::Max(1.0f, BombRadiusCm);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothBomb), false, this);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, ExplosionCenter, FQuat::Identity, Objects,
		FCollisionShape::MakeSphere(Radius), Params);
	TSet<AActor*> Visited;
	TArray<FTarget> Targets;
	FVector Fallback = FVector(Context.ShotDirection.X, Context.ShotDirection.Y, 0.0).GetSafeNormal();
	if (Fallback.IsNearlyZero()) Fallback = FVector::ForwardVector;

	// Resolve all representatives and cover before damage delegates can move/spawn objects.
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!IsValid(Target) || Target == this || Visited.Contains(Target)) continue;
		Visited.Add(Target);
		const ACharacter* Character = Cast<ACharacter>(Target);
		const ADeviceBase* Device = Cast<ADeviceBase>(Target);
		if (!Character && (!Device || !Device->CanReceiveExternalImpulse())) continue;
		const UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Target->GetRootComponent());
		if (!Root) continue;
		const FVector Point = Root->GetComponentLocation();
		const FVector Radial = Point - ExplosionCenter;
		if (Radial.SizeSquared() > FMath::Square(Radius)) continue;
		// Only WorldStatic entities that block the target's body channel count as cover.
		// Other characters, device shot spheres and overlap-only triggers cannot shield it.
		FCollisionResponseParams CoverResponses(ECR_Ignore);
		CoverResponses.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
		if (GetWorld()->LineTraceTestByChannel(ExplosionCenter, Point, Root->GetCollisionObjectType(),
			Params, CoverResponses)) continue;
		FVector Velocity;
		if (Character)
		{
			const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
			const AMothEffectCharacter* Player = Cast<AMothEffectCharacter>(Target);
			if (!Health || !Health->IsAlive() || (Player && !Player->IsGameplayEnabled())) continue;
			FVector Horizontal = FVector(Radial.X, Radial.Y, 0.0).GetSafeNormal();
			if (Horizontal.IsNearlyZero()) Horizontal = Fallback;
			Velocity = Horizontal * FMath::Max(0.0f, BombCharacterHorizontalVelocityAdd)
				+ FVector::UpVector * FMath::Max(0.0f, BombCharacterUpVelocityAdd);
		}
		else
		{
			FVector Direction = Radial.GetSafeNormal();
			if (Direction.IsNearlyZero()) Direction = Fallback;
			Velocity = Direction * FMath::Max(0.0f, BombDeviceRadialVelocityAdd);
		}
		Targets.Add({Target, Point, Velocity});
	}

	for (const FTarget& Resolved : Targets)
	{
		if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Active) break;
		AActor* Target = Resolved.Actor.Get();
		if (!IsValid(Target) || Target->IsActorBeingDestroyed()) continue;
		if (Cast<ACharacter>(Target))
		{
			if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
			{
				FHitContext Damage;
				Damage.HitId = ActivationHitId;
				Damage.SourceActor = this;
				Damage.InstigatorPawn = Context.InstigatorPawn;
				Damage.ImpactPoint = Resolved.Point;
				Damage.ShotDirection = Resolved.Velocity.GetSafeNormal();
				Damage.Damage = FMath::Max(0.0f, BombDamage);
				Health->ApplyHit(Damage);
			}
		}
		// Never call BallisticReactive here: damage and motion cannot activate a device.
		if (IsValid(Target) && !Target->IsActorBeingDestroyed()
			&& Target->GetClass()->ImplementsInterface(UImpulseReceiver::StaticClass()))
		{
			IImpulseReceiver::Execute_ReceiveImpulse(Target, Resolved.Velocity, false, ActivationHitId);
		}
	}
	if (bEndingPlay || !bGameplayEnabled || DeviceState != EDeviceState::Active) return;
	FinishActivation();
#if !UE_BUILD_SHIPPING
	if (bLogDeviceEvents)
		UE_LOG(LogMothEffect, Log, TEXT("Bomb %s hit=%s center=%s radius=%.1f resolvedTargets=%d"),
			*GetName(), *ActivationHitId.ToString(), *ExplosionCenter.ToString(), Radius, Targets.Num());
#endif
	if (!bEndingPlay && !IsActorBeingDestroyed()) OnBombExploded(ExplosionCenter, Radius);
}
