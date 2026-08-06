#include "UI/OnlineRacingRaceCountdownWidget.h"

#include "Components/TextBlock.h"

void UOnlineRacingRaceCountdownWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideCountdown();
}

void UOnlineRacingRaceCountdownWidget::HideCountdown()
{
	SetVisibility(ESlateVisibility::Collapsed);
	LastDisplayedValue = INDEX_NONE;
}

void UOnlineRacingRaceCountdownWidget::ShowCountdown(const double TimeRemaining)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);

	const int32 CountdownValue = FMath::Max(1, FMath::CeilToInt(TimeRemaining));
	if (CountdownValue == LastDisplayedValue)
	{
		return;
	}

	LastDisplayedValue = CountdownValue;
	if (IsValid(CountdownText))
	{
		CountdownText->SetText(FText::AsNumber(CountdownValue));
	}

	BP_CountdownValueChanged(CountdownValue);
}

void UOnlineRacingRaceCountdownWidget::ShowRaceStarted()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	LastDisplayedValue = 0;

	if (IsValid(CountdownText))
	{
		CountdownText->SetText(FText::FromString(TEXT("GO")));
	}

	BP_RaceStarted();
}
