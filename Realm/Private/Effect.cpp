#include "Realm.h"
#include "Effect.h"
#include "UnrealNetwork.h"

AEffect::AEffect(const FObjectInitializer& objectInitializer)
: Super(objectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;

	NetUpdateFrequency = 15.f;
}

void AEffect::OnRepDuration()
{
	if (duration > 0.f)
		GetWorldTimerManager().SetTimer(effectTimer, duration, false);
}

void AEffect::OnRepTimerReset()
{
	if (duration > 0.f)
		GetWorldTimerManager().SetTimer(effectTimer, duration, false);
}

void AEffect::ResetEffectTimer(float newTime /* = 0.f */)
{
	if (newTime != 0.f)
		duration = newTime;

	bTimerReset = !bTimerReset;

	if (IsValid(statsManager))
		GetWorldTimerManager().SetTimer(effectTimer, FTimerDelegate::CreateUObject(statsManager, &UStatsManager::EffectFinished, keyName), duration, false);
}

void AEffect::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AEffect, uiName, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AEffect, description, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AEffect, duration, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AEffect, bStacking, COND_InitialOnly);
	DOREPLIFETIME(AEffect, stackAmount);
	DOREPLIFETIME(AEffect, bTimerReset);
	DOREPLIFETIME(AEffect, effectParticle);
}