#include "Feedback/MothFeedbackProfile.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

UMothFeedbackProfile::UMothFeedbackProfile()
{
	Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MothEffect/Feedback/M_DefaultFeedback.M_DefaultFeedback")));
	Attenuation = TSoftObjectPtr<USoundAttenuation>(FSoftObjectPath(TEXT("/Game/MothEffect/Feedback/SA_DefaultFeedback.SA_DefaultFeedback")));
	const FLinearColor Cyan(.18f,.82f,.9f), Orange(1,.42f,.12f), Purple(.64f,.36f,1);
	auto Add = [this](EMothFeedbackCue Kind, FLinearColor Color, float Duration, float Size, const TCHAR* Sound, float Volume=.5f, float Interval=.06f)
	{
		FMothFeedbackStyle Style; Style.Color=Color; Style.Duration=Duration; Style.SizeCm=Size;
		Style.Volume=Volume; Style.AudioInterval=Interval;
		if (Sound) Style.Sound=TSoftObjectPtr<USoundBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/MothEffect/Feedback/Audio/%s.%s"),Sound,Sound)));
		Styles.Add(Kind,Style);
	};
	Add(EMothFeedbackCue::Shot,Cyan,.06f,9,TEXT("S_Rifle"));
	Add(EMothFeedbackCue::Impact,FLinearColor(.8f,.9f,1),.12f,9,nullptr);
	Add(EMothFeedbackCue::Hit,FLinearColor::White,.14f,12,TEXT("S_Hit"),.4f);
	Add(EMothFeedbackCue::Damage,FLinearColor(1,.12f,.08f),.22f,16,TEXT("S_Damage"));
	Add(EMothFeedbackCue::Launcher,Cyan,.4f,45,TEXT("S_Launcher"));
	Add(EMothFeedbackCue::Bomb,Orange,.35f,80,TEXT("S_Bomb"),.7f);
	Add(EMothFeedbackCue::Emitter,Purple,.3f,30,TEXT("S_Emitter"));
	Add(EMothFeedbackCue::EmitterShot,Purple,.08f,8,TEXT("S_Pulse"),.25f);
	Add(EMothFeedbackCue::EnemyShot,FLinearColor(1,.16f,.08f),.08f,8,TEXT("S_Pulse"),.35f);
	Add(EMothFeedbackCue::Expiry,WarningColor,.3f,24,TEXT("S_Warning"),.4f,.35f);
	Add(EMothFeedbackCue::Spent,SpentColor,.2f,20,nullptr);
	Add(EMothFeedbackCue::Failure,FLinearColor(1,.18f,.08f),.35f,24,TEXT("S_Warning"),.4f,.35f);
	Add(EMothFeedbackCue::Pickup,Cyan,.2f,18,nullptr);
}
