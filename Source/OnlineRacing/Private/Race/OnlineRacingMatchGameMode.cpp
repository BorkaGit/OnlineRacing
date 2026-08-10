#include "Race/OnlineRacingMatchGameMode.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "OnlineRacing.h"
#include "OnlineRacingPawn.h"
#include "AI/OnlineRacingAIController.h"
#include "Race/OnlineRacingCheckpoint.h"
#include "Race/OnlineRacingGridStart.h"
#include "Race/OnlineRacingMatchGameState.h"
#include "Race/OnlineRacingMatchPlayerState.h"

AOnlineRacingMatchGameMode::AOnlineRacingMatchGameMode()
{
	GameStateClass = AOnlineRacingMatchGameState::StaticClass();
	PlayerStateClass = AOnlineRacingMatchPlayerState::StaticClass();
	AIControllerClass = AOnlineRacingAIController::StaticClass();
}

void AOnlineRacingMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!DiscoverRacePlayerStarts())
	{
		return;
	}

	bCheckpointConfigurationValid = DiscoverRaceCheckpoints();
	if (!bCheckpointConfigurationValid)
	{
		return;
	}

	AOnlineRacingMatchGameState* const RaceGameState = GetGameState<AOnlineRacingMatchGameState>();
	if (!IsValid(RaceGameState))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] RaceGameState is unavailable."));
		return;
	}

	RaceGameState->InitializeRace(TotalLaps, RaceCheckpoints.Num());
	NextFinishPosition = 1;

	if (!SpawnBots())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Race initialization stopped because bots could not be spawned."));
		return;
	}

	for (APlayerState* const PlayerState : RaceGameState->PlayerArray)
	{
		AOnlineRacingMatchPlayerState* const RacePlayerState = Cast<AOnlineRacingMatchPlayerState>(PlayerState);
		if (IsValid(RacePlayerState))
		{
			InitializePlayerState(*RacePlayerState);
		}
	}

	BeginCountdown();
}

void AOnlineRacingMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!bCheckpointConfigurationValid || !IsValid(NewPlayer))
	{
		return;
	}

	AOnlineRacingMatchPlayerState* const RacePlayerState = NewPlayer->GetPlayerState<AOnlineRacingMatchPlayerState>();
	if (IsValid(RacePlayerState))
	{
		InitializePlayerState(*RacePlayerState);
	}
}

void AOnlineRacingMatchGameMode::HandleCheckpointReached(
	const AOnlineRacingCheckpoint& Checkpoint,
	const AOnlineRacingPawn& VehiclePawn)
{
	if (!bCheckpointConfigurationValid)
	{
		return;
	}

	AOnlineRacingMatchGameState* const RaceGameState = GetGameState<AOnlineRacingMatchGameState>();
	if (!IsValid(RaceGameState) || RaceGameState->GetRacePhase() != EOnlineRacingMatchPhase::Racing)
	{
		return;
	}

	AOnlineRacingMatchPlayerState* const RacePlayerState = VehiclePawn.GetPlayerState<AOnlineRacingMatchPlayerState>();
	if (!IsValid(RacePlayerState) || RacePlayerState->HasFinishedRace())
	{
		return;
	}

	const int32 CheckpointIndex = Checkpoint.GetCheckpointIndex();
	if (CheckpointIndex != RacePlayerState->GetNextCheckpointIndex())
	{
		return;
	}

	int32 CurrentLap = RacePlayerState->GetCurrentLap();
	int32 NextCheckpointIndex = CheckpointIndex + 1;
	bool bFinished = false;

	if (NextCheckpointIndex >= RaceCheckpoints.Num())
	{
		NextCheckpointIndex = 0;
	}

	if (CheckpointIndex == 0)
	{
		if (CurrentLap >= TotalLaps)
		{
			bFinished = true;
			NextCheckpointIndex = INDEX_NONE;
			UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] %s finished the race on lap %d."), *GetNameSafe(RacePlayerState), CurrentLap);
		}
		else
		{
			++CurrentLap;
			NextCheckpointIndex = 1;
			UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] %s started lap %d."), *GetNameSafe(RacePlayerState), CurrentLap);
		}
	}

	RacePlayerState->SetRaceProgress(CurrentLap, CheckpointIndex, NextCheckpointIndex);
	if (bFinished)
	{
		RecordPlayerFinish(*RacePlayerState);
	}

	if (!bFinished || !AreAllPlayersFinished())
	{
		return;
	}

	if (IsValid(RaceGameState))
	{
		RaceGameState->SetRacePhase(EOnlineRacingMatchPhase::Finished);
	}
}

bool AOnlineRacingMatchGameMode::HandleRespawnRequest(AController& Controller)
{
	if (!HasAuthority() || !bCheckpointConfigurationValid)
	{
		return false;
	}

	const AOnlineRacingMatchPlayerState* const RacePlayerState = Controller.GetPlayerState<AOnlineRacingMatchPlayerState>();
	AOnlineRacingPawn* const VehiclePawn = Cast<AOnlineRacingPawn>(Controller.GetPawn());
	if (!IsValid(RacePlayerState) || !IsValid(VehiclePawn))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][Race] Rejected respawn for %s because its race state or vehicle is unavailable."), *GetNameSafe(&Controller));
		return false;
	}

	const int32 CheckpointIndex = RacePlayerState->GetLastCheckpointIndex();
	if (!RaceCheckpoints.IsValidIndex(CheckpointIndex) || !IsValid(RaceCheckpoints[CheckpointIndex]))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][Race] Rejected respawn for %s because checkpoint index %d is invalid."), *GetNameSafe(&Controller), CheckpointIndex);
		return false;
	}

	const AOnlineRacingCheckpoint* const Checkpoint = RaceCheckpoints[CheckpointIndex];
	VehiclePawn->RespawnVehicleAtTransform(Checkpoint->GetRespawnTransform());

	UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] Respawned %s at checkpoint %d."), *GetNameSafe(VehiclePawn), CheckpointIndex);

	return true;
}

AActor* AOnlineRacingMatchGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const UWorld* CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return nullptr;
	}

	if (!IsValid(Player))
	{
		return nullptr;
	}

	if (RacePlayerStarts.IsEmpty() && !DiscoverRacePlayerStarts())
	{
		return nullptr;
	}

	AOnlineRacingGridStart* const ExistingStart = Cast<AOnlineRacingGridStart>(Player->StartSpot.Get());

	if (IsValid(ExistingStart) && RacePlayerStarts.Contains(ExistingStart))
	{
		return ExistingStart;
	}

	for (AOnlineRacingGridStart* const Candidate : RacePlayerStarts)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		bool bStartClaimed = false;

		for (FConstControllerIterator ControllerIterator = CurrentWorld->GetControllerIterator(); ControllerIterator; ++ControllerIterator)
		{
			const AController* const OtherController = ControllerIterator->Get();

			if (!IsValid(OtherController) || OtherController == Player)
			{
				continue;
			}

			if (OtherController->StartSpot.Get() == Candidate)
			{
				bStartClaimed = true;
				break;
			}
		}

		if (bStartClaimed)
		{
			continue;
		}

		Player->StartSpot = Candidate;
		return Candidate;
	}

	UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][Race] No free grid start is available for %s."), *GetNameSafe(Player));

	return nullptr;
}

void AOnlineRacingMatchGameMode::BeginCountdown()
{
	if (!HasAuthority())
	{
		return;
	}

	AOnlineRacingMatchGameState* const RaceGameState = GetGameState<AOnlineRacingMatchGameState>();
	if (!IsValid(RaceGameState))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Cannot begin countdown because RaceGameState is unavailable."));
		return;
	}

	const double CountdownEndServerTime = RaceGameState->GetServerWorldTimeSeconds() + CountdownDuration;
	RaceGameState->BeginCountdown(CountdownEndServerTime);
	GetWorldTimerManager().SetTimer(CountdownTimer, this, &AOnlineRacingMatchGameMode::StartRace, CountdownDuration, false);

	UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] Countdown started and will end at server time %.3f."), CountdownEndServerTime);
}

void AOnlineRacingMatchGameMode::StartRace()
{
	if (!HasAuthority())
	{
		return;
	}

	AOnlineRacingMatchGameState* const RaceGameState = GetGameState<AOnlineRacingMatchGameState>();
	if (!IsValid(RaceGameState) || RaceGameState->GetRacePhase() != EOnlineRacingMatchPhase::Countdown)
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][Race] Rejected countdown completion because the race is not in Countdown phase."));
		return;
	}

	const double RaceStartServerTime = RaceGameState->GetServerWorldTimeSeconds();
	RaceGameState->StartRace(RaceStartServerTime);
	UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] Countdown completed; race started."));
}

bool AOnlineRacingMatchGameMode::DiscoverRaceCheckpoints()
{
	TArray<AActor*> FoundCheckpointActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOnlineRacingCheckpoint::StaticClass(), FoundCheckpointActors);

	RaceCheckpoints.Reset(FoundCheckpointActors.Num());
	for (AActor* const FoundActor : FoundCheckpointActors)
	{
		AOnlineRacingCheckpoint* const Checkpoint = Cast<AOnlineRacingCheckpoint>(FoundActor);
		if (IsValid(Checkpoint))
		{
			RaceCheckpoints.Add(Checkpoint);
		}
	}

	if (RaceCheckpoints.Num() < 2)
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] At least two checkpoints are required; found %d."), RaceCheckpoints.Num());
		return false;
	}

	RaceCheckpoints.Sort([](
		const AOnlineRacingCheckpoint& Left,
		const AOnlineRacingCheckpoint& Right)
	{
		return Left.GetCheckpointIndex() < Right.GetCheckpointIndex();
	});

	for (int32 ExpectedIndex = 0; ExpectedIndex < RaceCheckpoints.Num(); ++ExpectedIndex)
	{
		const AOnlineRacingCheckpoint* const Checkpoint = RaceCheckpoints[ExpectedIndex];
		if (!IsValid(Checkpoint) || Checkpoint->GetCheckpointIndex() != ExpectedIndex)
		{
			int32 FoundIndex = INDEX_NONE;
			if (IsValid(Checkpoint))
			{
				FoundIndex = Checkpoint->GetCheckpointIndex();
			}

			UE_LOG(
				LogOnlineRacing,
				Error,
				TEXT("[Server][Race] Checkpoint indices must be unique and contiguous from 0. Expected index %d, found %d on %s."),
				ExpectedIndex,
				FoundIndex,
				*GetNameSafe(Checkpoint));
			return false;
		}
	}

	return true;
}

bool AOnlineRacingMatchGameMode::DiscoverRacePlayerStarts()
{
	TArray<AActor*> FoundStartActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOnlineRacingGridStart::StaticClass(), FoundStartActors);

	RacePlayerStarts.Reset(FoundStartActors.Num());

	for (AActor* const FoundActor : FoundStartActors)
	{
		AOnlineRacingGridStart* const PlayerStart = Cast<AOnlineRacingGridStart>(FoundActor);

		if (IsValid(PlayerStart))
		{
			RacePlayerStarts.Add(PlayerStart);
		}
	}

	if (RacePlayerStarts.IsEmpty())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] No race player starts were found."));

		return false;
	}

	RacePlayerStarts.Sort([](
		const AOnlineRacingGridStart& Left,
		const AOnlineRacingGridStart& Right)
	{
		return Left.GetGridIndex() < Right.GetGridIndex();
	});

	for (int32 ExpectedIndex = 0; ExpectedIndex < RacePlayerStarts.Num(); ++ExpectedIndex)
	{
		const AOnlineRacingGridStart* const PlayerStart = RacePlayerStarts[ExpectedIndex];

		if (!IsValid(PlayerStart) || PlayerStart->GetGridIndex() != ExpectedIndex)
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Grid indices must be unique and contiguous from 0."));

			RacePlayerStarts.Reset();
			return false;
		}
	}

	return true;
}

void AOnlineRacingMatchGameMode::InitializePlayerState(AOnlineRacingMatchPlayerState& RacePlayerState) const
{
	RacePlayerState.InitializeRaceProgress(1);
}

void AOnlineRacingMatchGameMode::RecordPlayerFinish(AOnlineRacingMatchPlayerState& RacePlayerState)
{
	AOnlineRacingMatchGameState* const RaceGameState = GetGameState<AOnlineRacingMatchGameState>();
	if (!IsValid(RaceGameState))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Cannot record finish because RaceGameState is unavailable."));
		return;
	}

	const double FinishTimeSeconds = FMath::Max(
		RaceGameState->GetServerWorldTimeSeconds() - RaceGameState->GetRaceStartServerTime(),
		0.0);
	const int32 FinishPosition = NextFinishPosition;
	++NextFinishPosition;

	RacePlayerState.FinishRace(FinishPosition, FinishTimeSeconds);

	FOnlineRacingMatchResult RaceResult;
	RaceResult.Position = FinishPosition;
	RaceResult.PlayerName = RacePlayerState.GetPlayerName();
	if (RaceResult.PlayerName.IsEmpty())
	{
		RaceResult.PlayerName = FString::Printf(TEXT("Racer %d"), FinishPosition);
	}
	RaceResult.FinishTimeSeconds = FinishTimeSeconds;
	RaceGameState->AddRaceResult(RaceResult);

	UE_LOG(
		LogOnlineRacing,
		Display,
		TEXT("[Server][Race] %s finished in position %d with time %.3f s."),
		*GetNameSafe(&RacePlayerState),
		FinishPosition,
		FinishTimeSeconds);
}

bool AOnlineRacingMatchGameMode::SpawnBots()
{
	if (!HasAuthority())
	{
		return false;
	}

	if (BotCount <= 0)
	{
		return true;
	}

	UWorld* const CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld) || !IsValid(AIControllerClass.Get()))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Cannot spawn bots because the world or AI controller class is unavailable."));
		return false;
	}

	for (int32 BotIndex = 0; BotIndex < BotCount; ++BotIndex)
	{
		AOnlineRacingAIController* const BotController = CurrentWorld->SpawnActor<AOnlineRacingAIController>(AIControllerClass.Get());

		if (!IsValid(BotController))
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Failed to spawn bot controller %d of %d."), BotIndex + 1, BotCount);
			return false;
		}

		AOnlineRacingGridStart* const GridStart = Cast<AOnlineRacingGridStart>(FindPlayerStart(BotController));
		if (!IsValid(GridStart))
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Failed to spawn bot controller %d of %d because it could not find a grid start."), BotIndex + 1, BotCount);

			BotController->Destroy();
			return false;
		}

		// RestartPlayerAtPlayerStart does not save this for AI controllers.
		BotController->StartSpot = GridStart;
		RestartPlayerAtPlayerStart(BotController, GridStart);

		if (!IsValid(BotController->GetPawn()))
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Failed to spawn bot controller %d of %d because it could not spawn a vehicle."), BotIndex + 1, BotCount);

			BotController->Destroy();
			return false;
		}

		UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] Spawned bot %d at grid index %d."), BotIndex + 1, GridStart->GetGridIndex());
	}

	return true;
}

bool AOnlineRacingMatchGameMode::AreAllPlayersFinished() const
{
	const AOnlineRacingMatchGameState* const RaceGameState = GetGameState<AOnlineRacingMatchGameState>();
	if (!IsValid(RaceGameState))
	{
		return false;
	}

	bool bFoundRacingPlayer = false;
	for (const APlayerState* const PlayerState : RaceGameState->PlayerArray)
	{
		const AOnlineRacingMatchPlayerState* const RacePlayerState = Cast<AOnlineRacingMatchPlayerState>(PlayerState);
		if (!IsValid(RacePlayerState))
		{
			continue;
		}

		bFoundRacingPlayer = true;
		if (!RacePlayerState->HasFinishedRace())
		{
			return false;
		}
	}

	return bFoundRacingPlayer;
}
