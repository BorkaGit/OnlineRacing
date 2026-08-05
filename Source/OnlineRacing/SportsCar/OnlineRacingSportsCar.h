// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineRacingPawn.h"
#include "OnlineRacingSportsCar.generated.h"

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class AOnlineRacingSportsCar : public AOnlineRacingPawn
{
	GENERATED_BODY()
	
public:

	AOnlineRacingSportsCar();
};
