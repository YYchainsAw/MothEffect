#include "Feedback/MothFeedbackVisual.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AMothFeedbackVisual::AMothFeedbackVisual()
{
	PrimaryActorTick.bCanEverTick=true;
	RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetActorEnableCollision(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 I=0; I<12; ++I)
	{
		auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Piece%d"),I));
		Mesh->SetupAttachment(RootComponent); Mesh->SetStaticMesh(Cube.Object);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false); Mesh->SetVisibility(false);
		Pieces.Add(Mesh);
	}
}

void AMothFeedbackVisual::InitializeCue(EMothFeedbackCue Kind, const FMothFeedbackStyle& Style, UMothFeedbackProfile* Profile, FVector Direction, float RadiusCm)
{
	Cue=Kind; Duration=FMath::Clamp(Style.Duration,.02f,2.f);
	Size=FMath::Clamp(RadiusCm>0?RadiusCm:Style.SizeCm,1.f,1000.f);
	// Bomb ring stays horizontal; directional flashes align to the committed flight direction.
	SetActorRotation(Cue==EMothFeedbackCue::Bomb?FRotator::ZeroRotator:Direction.GetSafeNormal().Rotation());
	if (auto* Material=Profile->Material.LoadSynchronous())
		for (const auto& Mesh : Pieces)
		{
			auto* MID=UMaterialInstanceDynamic::Create(Material,this);
			MID->SetVectorParameterValue(TEXT("Color"),Style.Color);
			MID->SetScalarParameterValue(TEXT("Intensity"),FMath::Clamp(Profile->EmissiveStrength,.1f,8.f));
			Mesh->SetMaterial(0,MID);
		}
	SetLifeSpan(Duration); UpdateShape();
}

void AMothFeedbackVisual::Tick(float DeltaSeconds) { Super::Tick(DeltaSeconds); Age+=DeltaSeconds; UpdateShape(); }

void AMothFeedbackVisual::UpdateShape()
{
	const float T=FMath::Clamp(Age/Duration,0.f,1.f);
	const bool bRing=Cue==EMothFeedbackCue::Bomb;
	const bool bCross=Cue==EMothFeedbackCue::Spent||Cue==EMothFeedbackCue::Failure;
	const bool bLift=Cue==EMothFeedbackCue::Launcher;
	const int32 Count=bRing?12:bCross?2:bLift?3:4;
	for (int32 I=0; I<Pieces.Num(); ++I)
	{
		const auto& Mesh=Pieces[I]; Mesh->SetVisibility(I<Count);
		if (I>=Count) continue;
		FVector Position; FRotator Rotation; FVector Scale;
		if (bRing)
		{
			const float Angle=I*UE_TWO_PI/Count, Radius=Size*(.2f+.8f*T);
			Position=FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,4);
			Rotation=FRotator(0,FMath::RadiansToDegrees(Angle)+90,0);
			Scale=FVector(Size*.28f/100,.025f*(1-T)+.01f,.02f);
		}
		else if (bLift)
		{
			Position=FVector(0,(I-1)*Size*.35f,Size*(.3f+T));
			Scale=FVector(.035f,.035f,Size*(.8f-.5f*T)/100);
		}
		else
		{
			const float Angle=(bCross?45.f:I*90.f)+ (bCross?I*90.f:0.f);
			Rotation=FRotator(0,0,Angle);
			Position=FVector(0,0,0);
			if (!bCross) Position=Rotation.RotateVector(FVector(0,Size*(.5f+.5f*T),0));
			Scale=FVector(.02f,Size*(bCross?1.f:.4f)*(1-.65f*T)/100,.025f);
		}
		Mesh->SetRelativeLocationAndRotation(Position,Rotation); Mesh->SetRelativeScale3D(Scale);
	}
}
