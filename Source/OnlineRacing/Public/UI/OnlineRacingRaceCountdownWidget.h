#pragma once

#include "Blueprint/UserWidget.h"
#include "OnlineRacingRaceCountdownWidget.generated.h"

class UTextBlock;

UCLASS(Abstract)
class ONLINERACING_API UOnlineRacingRaceCountdownWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void HideCountdown();
	void ShowCountdown(double TimeRemaining);
	void ShowRaceStarted();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Race|Countdown")
	TObjectPtr<UTextBlock> CountdownText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Race|Countdown", meta = (DisplayName = "Countdown Value Changed"))
	void BP_CountdownValueChanged(int32 CountdownValue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Race|Countdown", meta = (DisplayName = "Race Started"))
	void BP_RaceStarted();

private:
	int32 LastDisplayedValue = INDEX_NONE;
};
