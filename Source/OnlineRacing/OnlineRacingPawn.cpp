// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRacingPawn.h"

#include "Camera/CameraComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "TimerManager.h"

#include "OnlineRacing.h"
#include "OnlineRacingPlayerController.h"
#include "Race/OnlineRacingMatchGameMode.h"
#include "Vehicle/OnlineRacingVehicleTelemetryComponent.h"

AOnlineRacingPawn::AOnlineRacingPawn()
{
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(TEXT("Vehicle"));

	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	VehicleTelemetry = CreateDefaultSubobject<UOnlineRacingVehicleTelemetryComponent>(TEXT("Vehicle Telemetry"));
}

void AOnlineRacingPawn::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(FlipCheckTimer, this, &AOnlineRacingPawn::FlippedCheck, FlipCheckTime, true);
	}
}

void AOnlineRacingPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FlipCheckTimer);
	Super::EndPlay(EndPlayReason);
}

void AOnlineRacingPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!IsValid(EnhancedInputComponent))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[VehiclePawn] Enhanced Input component is required for %s."), *GetNameSafe(this));
		return;
	}

	EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AOnlineRacingPawn::Steering);
	EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AOnlineRacingPawn::Steering);
	EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AOnlineRacingPawn::Throttle);
	EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AOnlineRacingPawn::Throttle);
	EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AOnlineRacingPawn::Brake);
	EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &AOnlineRacingPawn::BrakeStarted);
	EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AOnlineRacingPawn::BrakeStopped);
	EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AOnlineRacingPawn::HandbrakeStarted);
	EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AOnlineRacingPawn::HandbrakeStopped);
	EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &AOnlineRacingPawn::LookAround);
	EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Started, this, &AOnlineRacingPawn::ToggleCamera);
	EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Started, this, &AOnlineRacingPawn::ResetVehicle);
}

void AOnlineRacingPawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	if (bMovingOnGround)
	{
		GetMesh()->SetAngularDamping(0.0f);
	}
	else
	{
		GetMesh()->SetAngularDamping(3.0f);
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	const float CameraYaw = FMath::FInterpTo(BackSpringArm->GetRelativeRotation().Yaw, 0.0f, DeltaSeconds, 1.0f);
	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

void AOnlineRacingPawn::Steering(const FInputActionValue& Value)
{
	DoSteering(Value.Get<float>());
}

void AOnlineRacingPawn::Throttle(const FInputActionValue& Value)
{
	DoThrottle(Value.Get<float>());
}

void AOnlineRacingPawn::Brake(const FInputActionValue& Value)
{
	DoBrake(Value.Get<float>());
}

void AOnlineRacingPawn::BrakeStarted(const FInputActionValue& Value)
{
	DoBrakeStart();
}

void AOnlineRacingPawn::BrakeStopped(const FInputActionValue& Value)
{
	DoBrakeStop();
}

void AOnlineRacingPawn::HandbrakeStarted(const FInputActionValue& Value)
{
	DoHandbrakeStart();
}

void AOnlineRacingPawn::HandbrakeStopped(const FInputActionValue& Value)
{
	DoHandbrakeStop();
}

void AOnlineRacingPawn::LookAround(const FInputActionValue& Value)
{
	DoLookAround(Value.Get<float>());
}

void AOnlineRacingPawn::ToggleCamera(const FInputActionValue& Value)
{
	DoToggleCamera();
}

void AOnlineRacingPawn::ResetVehicle(const FInputActionValue& Value)
{
	DoResetVehicle();
}

void AOnlineRacingPawn::DoSteering(const float SteeringValue)
{
	if (!bRaceInputEnabled)
	{
		return;
	}

	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void AOnlineRacingPawn::DoThrottle(const float ThrottleValue)
{
	if (!bRaceInputEnabled)
	{
		return;
	}

	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AOnlineRacingPawn::DoBrake(const float BrakeValue)
{
	if (!bRaceInputEnabled)
	{
		return;
	}

	ChaosVehicleMovement->SetBrakeInput(BrakeValue);
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void AOnlineRacingPawn::DoBrakeStart()
{
	if (!bRaceInputEnabled)
	{
		return;
	}

	BrakeLights(true);
}

void AOnlineRacingPawn::DoBrakeStop()
{
	if (!bRaceInputEnabled)
	{
		return;
	}

	BrakeLights(false);
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AOnlineRacingPawn::DoHandbrakeStart()
{
	if (!bRaceInputEnabled)
	{
		return;
	}

	ChaosVehicleMovement->SetHandbrakeInput(true);
	BrakeLights(true);
}

void AOnlineRacingPawn::DoHandbrakeStop()
{
	if (!bRaceInputEnabled)
	{
		return;
	}

	ChaosVehicleMovement->SetHandbrakeInput(false);
	BrakeLights(false);
}

void AOnlineRacingPawn::SetRaceInputEnabled(const bool bEnabled)
{
	bRaceInputEnabled = bEnabled;

	ChaosVehicleMovement->SetSteeringInput(0.f);
	ChaosVehicleMovement->SetThrottleInput(0.f);
	ChaosVehicleMovement->SetBrakeInput(0.f);
	ChaosVehicleMovement->SetHandbrakeInput(false);

	if (!bRaceInputEnabled)
	{
		ChaosVehicleMovement->SetBrakeInput(1.f);
		ChaosVehicleMovement->SetHandbrakeInput(true);
	}
}

void AOnlineRacingPawn::DoLookAround(const float YawDelta)
{
	BackSpringArm->AddLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void AOnlineRacingPawn::DoToggleCamera()
{
	bFrontCameraActive = !bFrontCameraActive;
	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void AOnlineRacingPawn::DoResetVehicle()
{
	AOnlineRacingPlayerController* const OnlineRacingPlayerController = Cast<AOnlineRacingPlayerController>(GetController());
	if (IsValid(OnlineRacingPlayerController))
	{
		OnlineRacingPlayerController->RequestVehicleRespawn();
		return;
	}

	if (!HasAuthority())
	{
		Server_ResetVehicle();
		return;
	}

	ResetVehicleAtCurrentLocation();
}

void AOnlineRacingPawn::Server_ResetVehicle_Implementation()
{
	ResetVehicleAtCurrentLocation();
}

void AOnlineRacingPawn::ResetVehicleAtCurrentLocation()
{
	if (!HasAuthority())
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Client][VehiclePawn] Rejected non-authoritative reset for %s."), *GetNameSafe(this));
		return;
	}

	const FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	RespawnVehicleAtTransform(FTransform(ResetRotation, ResetLocation));
}

void AOnlineRacingPawn::RespawnVehicleAtTransform(const FTransform& RespawnTransform)
{
	if (!HasAuthority())
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Client][VehiclePawn] Rejected non-authoritative respawn transform for %s."), *GetNameSafe(this));
		return;
	}

	if (RespawnTransform.ContainsNaN())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][VehiclePawn] Rejected invalid respawn transform for %s."), *GetNameSafe(this));
		return;
	}

	ChaosVehicleMovement->ResetVehicleState();
	SetActorLocationAndRotation(
		RespawnTransform.GetLocation(),
		RespawnTransform.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	GetMesh()->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
	GetMesh()->WakeAllRigidBodies();
	SetRaceInputEnabled(bRaceInputEnabled);
	ForceNetUpdate();
}

void AOnlineRacingPawn::FlippedCheck()
{
	if (!HasAuthority())
	{
		return;
	}

	const UWorld* CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return;
	}

	const float UpDot = FVector::DotProduct(FVector::UpVector, GetMesh()->GetUpVector());
	if (UpDot >= FlipCheckMinDot)
	{
		bPreviousFlipCheck = false;
		return;
	}

	if (bPreviousFlipCheck)
	{
		AController* const VehicleController = GetController();
		
		AOnlineRacingMatchGameMode* const RaceGameMode = CurrentWorld->GetAuthGameMode<AOnlineRacingMatchGameMode>();
		
		if (IsValid(VehicleController) && IsValid(RaceGameMode))
		{
			RaceGameMode->HandleRespawnRequest(*VehicleController);
		}
		else
		{
			ResetVehicleAtCurrentLocation();
		}

		bPreviousFlipCheck = false;
		return;
	}

	bPreviousFlipCheck = true;
}
