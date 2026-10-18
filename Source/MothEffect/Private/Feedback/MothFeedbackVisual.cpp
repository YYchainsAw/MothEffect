#include "Feedback/MothFeedbackVisual.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
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

void AMothFeedbackVisual::InitializeCue(EMothFeedbackCue Kind, const FMothFeedbackStyle& Style, UMothFeedbackProfile* Profile, FVector Direction, float RadiusCm, UNiagaraSystem* System)
{
	Cue=Kind; Duration=FMath::Clamp(Style.Duration,.02f,2.f);
	Size=FMath::Clamp(RadiusCm>0?RadiusCm:Style.SizeCm,1.f,1000.f);
	// Bomb ring stays horizontal; directional flashes align to the committed flight direction.
	SetActorRotation(Cue==EMothFeedbackCue::Bomb||Cue==EMothFeedbackCue::Launcher?FRotator::ZeroRotator:Direction.GetSafeNormal().Rotation());
	bDrawLegacyShape=Style.bLegacyVisual && !IsValid(System);
	if (IsValid(System))
	{
		NiagaraComponent=NewObject<UNiagaraComponent>(this,TEXT("CueNiagara"));
		AddInstanceComponent(NiagaraComponent);
		NiagaraComponent->SetupAttachment(RootComponent);
		NiagaraComponent->SetAutoActivate(false);
		// This bounded actor owns cleanup, even if the selected system loops forever.
		NiagaraComponent->SetAutoDestroy(false);
		NiagaraComponent->SetAsset(System);
		NiagaraComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NiagaraComponent->SetGenerateOverlapEvents(false);
		NiagaraComponent->SetCanEverAffectNavigation(false);
		NiagaraComponent->SetTickableWhenPaused(false);
		NiagaraComponent->RegisterComponent();
		const FQuat Offset=Style.NiagaraRotation.ContainsNaN()?FQuat::Identity:Style.NiagaraRotation.Quaternion();
		NiagaraComponent->SetWorldRotation(Direction.GetSafeNormal().Rotation().Quaternion()*Offset);
		FVector Scale=Style.NiagaraScale.ContainsNaN()?FVector::OneVector:Style.NiagaraScale;
		Scale=Scale.GetAbs().BoundToBox(FVector(.001f),FVector(100.f));
		if (Style.bScaleNiagaraToRadius && FMath::IsFinite(RadiusCm) && RadiusCm>0)
		{
			const float Reference=FMath::IsFinite(Style.NiagaraReferenceRadiusCm)?FMath::Max(1.f,Style.NiagaraReferenceRadiusCm):100.f;
			Scale*=FMath::Clamp(RadiusCm/Reference,.01f,100.f);
		}
		NiagaraComponent->SetRelativeScale3D(Scale);
		// Do not add unused user overrides to third-party systems with fixed authored colors.
		const auto HasColor=[System](FName Parameter)
		{
			return !Parameter.IsNone() && System->GetExposedParameters().IndexOf(
				FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(),Parameter))!=INDEX_NONE;
		};
		FName ColorParameter=Style.NiagaraColorParameter;
		if (ColorParameter.IsNone())
			ColorParameter=HasColor(TEXT("User.MothColor"))?FName(TEXT("User.MothColor")):FName(TEXT("User.Color"));
		if (HasColor(ColorParameter)) NiagaraComponent->SetVariableLinearColor(ColorParameter,Style.Color);
		if (!Style.NiagaraScaleParameter.IsNone() && System->GetExposedParameters().IndexOf(
			FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),Style.NiagaraScaleParameter))!=INDEX_NONE)
			NiagaraComponent->SetVariableFloat(Style.NiagaraScaleParameter,Scale.GetMax());
		if (!Style.NiagaraDirectionParameter.IsNone() && System->GetExposedParameters().IndexOf(
			FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(),Style.NiagaraDirectionParameter))!=INDEX_NONE)
			NiagaraComponent->SetVariableVec3(Style.NiagaraDirectionParameter,Direction.GetSafeNormal());
		NiagaraComponent->Activate(true);
		Duration=FMath::IsFinite(Style.NiagaraLifetimeSeconds)?FMath::Clamp(Style.NiagaraLifetimeSeconds,.05f,10.f):2.f;
	}
	if (bDrawLegacyShape && Profile) if (auto* Material=Profile->Material.LoadSynchronous())
		for (const auto& Mesh : Pieces)
		{
			auto* MID=UMaterialInstanceDynamic::Create(Material,this);
			MID->SetVectorParameterValue(TEXT("Color"),Style.Color);
			MID->SetScalarParameterValue(TEXT("Intensity"),FMath::Clamp(Profile->EmissiveStrength,.1f,8.f));
			Mesh->SetMaterial(0,MID);
		}
	if (!bDrawLegacyShape && !NiagaraComponent) { Destroy(); return; }
	SetLifeSpan(Duration); UpdateShape();
}

void AMothFeedbackVisual::Tick(float DeltaSeconds) { Super::Tick(DeltaSeconds); Age+=DeltaSeconds; UpdateShape(); }

void AMothFeedbackVisual::SetCuePaused(bool bPaused)
{
	if (IsValid(NiagaraComponent)) NiagaraComponent->SetPaused(bPaused);
}

void AMothFeedbackVisual::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(NiagaraComponent))
	{
		NiagaraComponent->DeactivateImmediate();
		NiagaraComponent->DestroyComponent();
		NiagaraComponent=nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AMothFeedbackVisual::UpdateShape()
{
	const float T=FMath::Clamp(Age/Duration,0.f,1.f);
	const bool bRing=Cue==EMothFeedbackCue::Bomb;
	const bool bCross=Cue==EMothFeedbackCue::Spent||Cue==EMothFeedbackCue::Failure;
	const bool bLift=Cue==EMothFeedbackCue::Launcher;
	const int32 Count=bRing?12:bCross?2:bLift?3:4;
	for (int32 I=0; I<Pieces.Num(); ++I)
	{
		const auto& Mesh=Pieces[I]; Mesh->SetVisibility(bDrawLegacyShape&&I<Count);
		if (!bDrawLegacyShape) continue;
		if (I>=Count) continue;
		FVector Position; FRotator Rotation=FRotator::ZeroRotator; FVector Scale;
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
