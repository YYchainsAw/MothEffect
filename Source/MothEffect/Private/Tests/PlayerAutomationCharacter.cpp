#include "Tests/PlayerAutomationCharacter.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/Rifle.h"

AMothPlayerAutomationCharacter::AMothPlayerAutomationCharacter()
{
#if WITH_DEV_AUTOMATION_TESTS
	RifleClass = ARifle::StaticClass();
	RifleAttachSocket = TEXT("AutomationHold");
	DeviceAttachSocket = TEXT("AutomationHold");
	MinGunRecoverySeconds = 0.4f;
	// Use only an engine fixture mesh, with no project BP, ABP, skeleton edits or Montages.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> TestMesh(TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube"));
	if (TestMesh.Succeeded() && TestMesh.Object->GetRefSkeleton().GetNum() > 0)
	{
		RifleAttachSocket = TestMesh.Object->GetRefSkeleton().GetBoneName(0);
		DeviceAttachSocket = RifleAttachSocket;
		GetMesh()->SetSkeletalMeshAsset(TestMesh.Object);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetVisibility(false);
	}
#endif
}

void AMothPlayerAutomationCharacter::BeginPlay()
{
	Super::BeginPlay();
#if WITH_DEV_AUTOMATION_TESTS
	GetCharacterMovement()->DisableMovement();
	// Non-null presentation requests deliberately cannot play without an AnimInstance.
	FireMontage = NewObject<UAnimMontage>(this);
	ReloadMontage = NewObject<UAnimMontage>(this);
	ThrowMontage = NewObject<UAnimMontage>(this);
#endif
}
