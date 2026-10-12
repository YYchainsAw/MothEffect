#include "Tutorial/MothTutorialTarget.h"
#include "Components/HealthComponent.h"
#include "Components/ExternalImpulseComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AMothTutorialTarget::AMothTutorialTarget()
{
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Body->SetStaticMesh(Cube.Object);
	Body->SetRelativeScale3D(FVector(0.6));
	Body->SetCollisionProfileName(TEXT("PhysicsActor"));
	Body->SetCollisionResponseToAllChannels(ECR_Block);
	Body->SetCanEverAffectNavigation(false);
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->ConfigureMaxHealth(60);
	Impulse = CreateDefaultSubobject<UExternalImpulseComponent>(TEXT("Impulse"));
}

void AMothTutorialTarget::BeginPlay()
{
	Super::BeginPlay();
	Health->OnDied.AddUniqueDynamic(this, &AMothTutorialTarget::HandleDied);
}

void AMothTutorialTarget::EnableLiftPhysics()
{
	Body->SetSimulatePhysics(true);
	Body->SetMassOverrideInKg(NAME_None, 4.0f);
}

bool AMothTutorialTarget::ReceiveBallisticHit_Implementation(const FHitContext& Context)
{
	return Health->ApplyHit(Context);
}

void AMothTutorialTarget::ReceiveImpulse_Implementation(FVector Velocity, bool bOverrideZ, FGuid EventId)
{
	if (Health->IsAlive() && Body->IsSimulatingPhysics()) Impulse->QueueImpulse(Velocity, bOverrideZ, EventId);
}

void AMothTutorialTarget::HandleDied(AActor* Victim, AActor* Source)
{
	Body->SetSimulatePhysics(false);
	Impulse->SetGameplayEnabled(false);
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
}

AMothTutorialBlastTarget::AMothTutorialBlastTarget()
{
	GetCapsuleComponent()->InitCapsuleSize(30,30);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
	Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body")); Body->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Body->SetStaticMesh(Cube.Object); Body->SetRelativeScale3D(FVector(.6)); Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Health=CreateDefaultSubobject<UHealthComponent>(TEXT("Health")); Health->ConfigureMaxHealth(60);
}
void AMothTutorialBlastTarget::BeginPlay()
{
	Super::BeginPlay(); GetCharacterMovement()->DisableMovement();
	Health->OnDied.AddUniqueDynamic(this,&AMothTutorialBlastTarget::HandleDied);
}
bool AMothTutorialBlastTarget::ReceiveBallisticHit_Implementation(const FHitContext& Context) { return Health->ApplyHit(Context); }
void AMothTutorialBlastTarget::HandleDied(AActor*,AActor*) { SetActorEnableCollision(false); SetActorHiddenInGame(true); }
