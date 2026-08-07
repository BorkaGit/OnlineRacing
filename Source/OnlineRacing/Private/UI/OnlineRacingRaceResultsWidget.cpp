#include "UI/OnlineRacingRaceResultsWidget.h"

#include "Components/TextBlock.h"

void UOnlineRacingRaceResultsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideResults();
}

void UOnlineRacingRaceResultsWidget::HideResults()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UOnlineRacingRaceResultsWidget::ShowResults(const TArray<FOnlineRacingRaceResult>& RaceResults)
{
	if (IsValid(ResultsText))
	{
		FString ResultsString = TEXT("RESULTS");
		for (const FOnlineRacingRaceResult& RaceResult : RaceResults)
		{
			const int32 Minutes = FMath::FloorToInt(RaceResult.FinishTimeSeconds / 60.0);
			const double Seconds = FMath::Fmod(RaceResult.FinishTimeSeconds, 60.0);
			ResultsString += FString::Printf(
				TEXT("\n%d. %s  %02d:%06.3f"),
				RaceResult.Position,
				*RaceResult.PlayerName,
				Minutes,
				Seconds);
		}

		ResultsText->SetText(FText::FromString(ResultsString));
	}

	SetVisibility(ESlateVisibility::Visible);
	BP_RaceResultsUpdated(RaceResults);
}
