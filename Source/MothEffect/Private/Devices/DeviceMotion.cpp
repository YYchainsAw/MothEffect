#include "Devices/DeviceBase.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"

void ADeviceBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateGrounding();
}

void ADeviceBase::RefreshDeviceSize()
{
	const float Radius = FMath::IsFinite(BodyRadiusCm) ? FMath::Max(1.0f, BodyRadiusCm) : 20.0f;
	PhysicsBody->SetSphereRadius(Radius);
	ShotCollider->SetSphereRadius(FMath::IsFinite(ShotRadiusCm) ? FMath::Max(Radius, ShotRadiusCm) : Radius);
	RefreshPrototypeAppearance();
	if (HasActorBegunPlay()) UpdateGrounding();
}

bool ADeviceBase::FindGroundSupport(FHitResult& Hit) const
{
	if (!GetWorld() || !PhysicsBody) return false;
	const FVector Position = PhysicsBody->GetComponentLocation();
	const float Radius = PhysicsBody->GetScaledSphereRadius();
	const float Probe = FMath::IsFinite(GroundSnapDistanceCm) ? FMath::Max(0.1f, GroundSnapDistanceCm) : 5.0f;
	if (Position.ContainsNaN() || !FMath::IsFinite(Radius) || Radius <= 0.0f) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MothDeviceGround), false, this);
	FCollisionResponseParams Responses(ECR_Ignore);
	Responses.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
	Responses.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);
	if (!GetWorld()->SweepSingleByChannel(Hit, Position + FVector::UpVector * Probe,
		Position - FVector::UpVector * Probe, FQuat::Identity, ECC_PhysicsBody,
		FCollisionShape::MakeSphere(Radius), Params, Responses)) return false;
	const UPrimitiveComponent* Support = Hit.GetComponent();
	// Query glyphs, characters, other devices and overlap triggers are not support.
	return !Hit.bStartPenetrating && Support && !Cast<ADeviceBase>(Hit.GetActor())
		&& Support->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Block
		&& Hit.ImpactNormal.Z >= FMath::Clamp(GroundUpNormalMin, 0.01f, 1.0f);
}

void ADeviceBase::SetGrounded(bool bNewGrounded)
{
	if (bGrounded == bNewGrounded) return;
	if (bNewGrounded && PhysicsBody->IsSimulatingPhysics())
	{
		PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
		PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
	bGrounded = bNewGrounded;
	ApplyStateCollision();
	if (bGrounded) SetActorRotation(FRotator(0.0f, GetActorRotation().Yaw, 0.0f), ETeleportType::TeleportPhysics);
}

void ADeviceBase::UpdateGrounding()
{
	const bool bMovableState = DeviceState == EDeviceState::Dormant
		|| (DeviceState == EDeviceState::Active && DeviceKind != EDeviceKind::Launcher);
	if (!HasActorBegunPlay() || bEndingPlay || !bGameplayEnabled || !GetWorld()
		|| GetWorld()->IsPaused() || !bMovableState || GetWorld()->GetTimeSeconds() < GroundingAllowedAt) return;
	if (!bGrounded && PhysicsBody->GetPhysicsLinearVelocity().Z > 1.0f) return;
	FHitResult Support;
	if (FindGroundSupport(Support))
	{
		SetGrounded(true);
		SetActorLocation(Support.Location + FVector::UpVector * 0.25f, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else SetGrounded(false);
}

bool ADeviceBase::WakeForExternalImpulse()
{
	if (!CanReceiveExternalImpulse()) return false;
	// Grounding precedes frame-end impulse settlement; let the accepted impulse take effect first.
	GroundingAllowedAt = GetWorld()->GetTimeSeconds() + 0.05;
	SetGrounded(false);
	return PhysicsBody->IsSimulatingPhysics();
}
