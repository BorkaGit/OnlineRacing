#include "UI/OnlineRacingCountdownWidget.h"

#include "Components/TextBlock.h"

void UOnlineRacingCountdownWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HideCountdown();
}

void UOnlineRacingCountdownWidget::HideCountdown()
{
	SetVisibility(ESlateVisibility::Collapsed);
	LastDisplayedValue = INDEX_NONE;
}

void UOnlineRacingCountdownWidget::ShowCountdown(const double TimeRemaining)
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

void UOnlineRacingCountdownWidget::ShowRaceStarted()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	LastDisplayedValue = 0;

	if (IsValid(CountdownText))
	{
		CountdownText->SetText(FText::FromString(TEXT("GO")));
	}

	BP_RaceStarted();
}
