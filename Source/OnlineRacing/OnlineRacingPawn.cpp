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
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void AOnlineRacingPawn::DoThrottle(const float ThrottleValue)
{
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AOnlineRacingPawn::DoBrake(const float BrakeValue)
{
	ChaosVehicleMovement->SetBrakeInput(BrakeValue);
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void AOnlineRacingPawn::DoBrakeStart()
{
	BrakeLights(true);
}

void AOnlineRacingPawn::DoBrakeStop()
{
	BrakeLights(false);
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AOnlineRacingPawn::DoHandbrakeStart()
{
	ChaosVehicleMovement->SetHandbrakeInput(true);
	BrakeLights(true);
}

void AOnlineRacingPawn::DoHandbrakeStop()
{
	ChaosVehicleMovement->SetHandbrakeInput(false);
	BrakeLights(false);
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
	if (!HasAuthority())
	{
		Server_ResetVehicle();
		return;
	}

	ResetVehicleOnServer();
}

void AOnlineRacingPawn::Server_ResetVehicle_Implementation()
{
	ResetVehicleOnServer();
}

void AOnlineRacingPawn::ResetVehicleOnServer()
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

	SetActorTransform(FTransform(ResetRotation, ResetLocation), false, nullptr, ETeleportType::TeleportPhysics);
	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
	ChaosVehicleMovement->ResetVehicleState();
}

void AOnlineRacingPawn::FlippedCheck()
{
	if (!HasAuthority())
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
		ResetVehicleOnServer();
		bPreviousFlipCheck = false;
		return;
	}

	bPreviousFlipCheck = true;
}
