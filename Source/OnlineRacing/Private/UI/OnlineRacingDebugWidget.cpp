#include "UI/OnlineRacingDebugWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"

#include "OnlineRacingPawn.h"
#include "Race/OnlineRacingRaceGameState.h"
#include "Race/OnlineRacingRacePlayerState.h"
#include "Vehicle/OnlineRacingVehicleTelemetryComponent.h"

namespace
{
const TCHAR* GetBooleanText(const bool bValue)
{
	if (bValue)
	{
		return TEXT("Yes");
	}

	return TEXT("No");
}

const TCHAR* GetNetModeText(const ENetMode NetMode)
{
	switch (NetMode)
	{
	case NM_Standalone:
		return TEXT("Standalone");
	case NM_DedicatedServer:
		return TEXT("Dedicated Server");
	case NM_ListenServer:
		return TEXT("Listen Server");
	case NM_Client:
		return TEXT("Client");
	default:
		return TEXT("Unknown");
	}
}
}

void UOnlineRacingDebugWidget::UpdateDebugData(const AOnlineRacingPawn& VehiclePawn)
{
	if (!IsValid(DebugText))
	{
		return;
	}

	const UOnlineRacingVehicleTelemetryComponent* const Telemetry = VehiclePawn.GetVehicleTelemetry();
	if (!IsValid(Telemetry))
	{
		DebugText->SetText(FText::FromString(TEXT("Vehicle telemetry unavailable")));
		return;
	}

	FString PingText = TEXT("N/A");
	const APlayerState* const PlayerState = VehiclePawn.GetPlayerState();
	if (IsValid(PlayerState))
	{
		PingText = FString::Printf(TEXT("%.0f ms"), PlayerState->GetPingInMilliseconds());
	}

	FString RaceText = TEXT("Race state: unavailable");
	const AOnlineRacingRaceGameState* const RaceGameState = VehiclePawn.GetWorld()->GetGameState<AOnlineRacingRaceGameState>();
	const AOnlineRacingRacePlayerState* const RacePlayerState = Cast<AOnlineRacingRacePlayerState>(PlayerState);
	if (IsValid(RaceGameState) && IsValid(RacePlayerState))
	{
		RaceText = FString::Printf(
			TEXT("Race phase: %s\n")
			TEXT("Countdown remaining: %.2f s\n")
			TEXT("Lap: %d/%d  Next checkpoint: %d/%d\n")
			TEXT("Last checkpoint: %d  Finished: %s\n")
			TEXT("Finish position: %d  Time: %.3f s"),
			*UEnum::GetValueAsString(RaceGameState->GetRacePhase()),
			RaceGameState->GetCountdownTimeRemaining(),
			RacePlayerState->GetCurrentLap(),
			RaceGameState->GetTotalLaps(),
			RacePlayerState->GetNextCheckpointIndex(),
			RaceGameState->GetCheckpointCount() - 1,
			RacePlayerState->GetLastCheckpointIndex(),
			GetBooleanText(RacePlayerState->HasFinishedRace()),
			RacePlayerState->GetFinishPosition(),
			RacePlayerState->GetFinishTimeSeconds());
	}

	const FString DebugString = FString::Printf(
		TEXT("Speed: %.1f km/h\n")
		TEXT("RPM: %.0f (%.0f%%)\n")
		TEXT("Gear: %d\n")
		TEXT("Throttle: %.2f  Brake: %.2f  Steering: %.2f\n")
		TEXT("Handbrake: %s  Wheels grounded: %d\n")
		TEXT("Slip: %s  Max: %.0f cm/s\n")
		TEXT("Skid: %s  Max: %.0f cm/s\n")
		TEXT("%s\n")
		TEXT("Net mode: %s\n")
		TEXT("Local role: %s\n")
		TEXT("Remote role: %s\n")
		TEXT("Ping: %s"),
		Telemetry->GetSpeedKmh(),
		Telemetry->GetEngineRpm(),
		Telemetry->GetNormalizedRpm() * 100.f,
		Telemetry->GetCurrentGear(),
		Telemetry->GetThrottleInput(),
		Telemetry->GetBrakeInput(),
		Telemetry->GetSteeringInput(),
		GetBooleanText(Telemetry->IsHandbrakeActive()),
		Telemetry->GetWheelsInContact(),
		GetBooleanText(Telemetry->IsAnyWheelSlipping()),
		Telemetry->GetMaxSlipMagnitude(),
		GetBooleanText(Telemetry->IsAnyWheelSkidding()),
		Telemetry->GetMaxSkidMagnitude(),
		*RaceText,
		GetNetModeText(VehiclePawn.GetNetMode()),
		*UEnum::GetValueAsString(VehiclePawn.GetLocalRole()),
		*UEnum::GetValueAsString(VehiclePawn.GetRemoteRole()),
		*PingText);

	DebugText->SetText(FText::FromString(DebugString));
}
