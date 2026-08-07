// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AIController.h"
#include "OnlineRacingAIController.generated.h"

class AOnlineRacingPawn;
class AOnlineRacingRacingLine;

UCLASS()
class ONLINERACING_API AOnlineRacingAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AOnlineRacingAIController();
	
	virtual void Tick(float DeltaSeconds) override;
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Identity")
	FString BotDisplayName = TEXT("AI Racer");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Driving", meta = (ClampMin = "100.0", ForceUnits = "cm"))
	float LookAheadDistance = 1200.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Debug")
	bool bDrawDrivingDebug = true;
	
private:
	
	void FindRacingLine();
	
	void DrawDrivingTarget() const;
	
	TWeakObjectPtr<AOnlineRacingPawn> VehiclePawn;
	TWeakObjectPtr<AOnlineRacingRacingLine> RacingLine;
};
