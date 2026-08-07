#pragma once

#include "Blueprint/UserWidget.h"
#include "Race/OnlineRacingRaceGameState.h"
#include "OnlineRacingRaceResultsWidget.generated.h"

class UTextBlock;

UCLASS(Abstract)
class ONLINERACING_API UOnlineRacingRaceResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void HideResults();
	void ShowResults(const TArray<FOnlineRacingRaceResult>& RaceResults);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Race|Results")
	TObjectPtr<UTextBlock> ResultsText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Race|Results", meta = (DisplayName = "Race Results Updated"))
	void BP_RaceResultsUpdated(const TArray<FOnlineRacingRaceResult>& RaceResults);
};
