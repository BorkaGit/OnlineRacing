#pragma once

#include "Blueprint/UserWidget.h"
#include "Race/OnlineRacingMatchGameState.h"
#include "OnlineRacingResultsWidget.generated.h"

class UTextBlock;

UCLASS(Abstract)
class ONLINERACING_API UOnlineRacingResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void HideResults();
	void ShowResults(const TArray<FOnlineRacingMatchResult>& RaceResults);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Race|Results")
	TObjectPtr<UTextBlock> ResultsText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Race|Results", meta = (DisplayName = "Race Results Updated"))
	void BP_RaceResultsUpdated(const TArray<FOnlineRacingMatchResult>& RaceResults);
};
