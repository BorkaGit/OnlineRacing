// Copyright Epic Games, Inc. All Rights Reserved.


#include "TimeTrialUI.h"

#include "OnlineRacing.h"
#include "TimeTrialStartUI.h"

void UTimeTrialUI::NativeConstruct()
{
	Super::NativeConstruct();

	UTimeTrialStartUI* const StartUI = CreateWidget<UTimeTrialStartUI>(GetOwningPlayer(), StartUIClass);
	if (!IsValid(StartUI))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Client][TimeTrialUI] Failed to create the countdown widget."));
		return;
	}

	StartUI->AddToViewport(0);
	StartUI->OnCountdownFinished.AddDynamic(this, &UTimeTrialUI::StartRace);
	StartUI->StartCountdown();
}

void UTimeTrialUI::UpdateLapCount(int32 Lap, float NewLapStartTime)
{
	LapStartTime = NewLapStartTime;
	const float LapTime = NewLapStartTime - LastLapTime;

	if (Lap > 1)
	{
		if (BestLapTime < 0.0f)
		{
			BestLapTime = LapTime;
		}
		else if (LapTime < BestLapTime)
		{
			BestLapTime = LapTime;
		}
	}
	else
	{
		BestLapTime = -1.0f;
	}

	CurrentLap = Lap;
	LastLapTime = NewLapStartTime;
	BP_UpdateLaps();
}

void UTimeTrialUI::StartRace()
{
	OnRaceStart.Broadcast();
}
