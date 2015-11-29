#include "Realm.h"
#include "SpectatorCharacter.h"
#include "RealmPlayerController.h"
#include "PlayerCharacter.h"
#include "GameCharacter.h"

ASpectatorCharacter::ASpectatorCharacter(const FObjectInitializer& objectInitializer)
:Super(objectInitializer)
{
	springArm = objectInitializer.CreateDefaultSubobject<USpringArmComponent>(this, TEXT("CameraBoom"));
	rtsCamera = objectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("RTS Camera"));

	springArm->AttachParent = RootComponent;

	springArm->TargetArmLength = 0.f;
	springArm->TargetOffset.X = -1000 * FMath::Cos(45.f);
	springArm->TargetOffset.Y = 1000 * FMath::Cos(45.f);
	springArm->TargetOffset.Z = 1420 * FMath::Cos(45.f);
	springArm->bInheritPitch = false;
	springArm->bInheritRoll = false;
	springArm->bInheritYaw = false;

	rtsCamera->AttachTo(springArm, TEXT(""), EAttachLocation::SnapToTarget);
	rtsCamera->SetWorldRotation(FRotator(-45.f, -45.f, 0.f));

	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	camSpeed = 475.f;
}

void ASpectatorCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxFlySpeed = 1420.f;
	GetCharacterMovement()->BrakingDecelerationFlying = 1710.f;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void ASpectatorCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	check(InputComponent);

	InputComponent->BindAction("MoveCommand", IE_Pressed, this, &ASpectatorCharacter::OnDirectedMoveStart);
	InputComponent->BindAction("MoveCommand", IE_Released, this, &ASpectatorCharacter::OnDirectedMoveStop);
	InputComponent->BindAction("Skill1", IE_Pressed, this, &ASpectatorCharacter::OnUseSkill<0>);
	InputComponent->BindAction("Skill2", IE_Pressed, this, &ASpectatorCharacter::OnUseSkill<1>);
	InputComponent->BindAction("Skill3", IE_Pressed, this, &ASpectatorCharacter::OnUseSkill<2>);
	InputComponent->BindAction("Skill4", IE_Pressed, this, &ASpectatorCharacter::OnUseSkill<3>);
	InputComponent->BindAction("SelfCameraLock", IE_Pressed, this, &ASpectatorCharacter::OnSelfCameraLock);
	InputComponent->BindAction("SelfCameraLock", IE_Released, this, &ASpectatorCharacter::OnUnlockCamera);
}

void ASpectatorCharacter::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (bMoveCommand && IsValid(this))
	{
		CalculateDirectedMove();
	}

	FVector2D mousePosition;
	FVector2D viewportSize;

	UGameViewportClient* gameViewport = GEngine->GameViewport;

	if (!gameViewport)
		return;

	gameViewport->GetViewportSize(viewportSize);

	FVector camDirection = FVector::ZeroVector;

	if (gameViewport->IsFocused(gameViewport->Viewport) && gameViewport->GetMousePosition(mousePosition))
	{
		//Check if the mouse is at the left or right edge of the screen and move accordingly
		if (mousePosition.X / viewportSize.X <= 0.15f)
		{
			//MoveCameraRight(-1.0f * deltaSeconds);
			camDirection += RightCameraMovement(-1.0f * deltaSeconds);
		}
		else if (mousePosition.X / viewportSize.X >= 0.85f)
		{
			camDirection += RightCameraMovement(1.0f * deltaSeconds);
		}

		//Check if the mouse is at the top or bottom edge of the screen and move accordingly
		if (mousePosition.Y / viewportSize.Y <= 0.15f)
		{
			camDirection += ForwardCameraMovement(1.0f * deltaSeconds);
		}
		else if (mousePosition.Y / viewportSize.Y >= 0.85f)
		{
			camDirection += ForwardCameraMovement(-1.0f * deltaSeconds);
		}
	}

	if (Role < ROLE_Authority)
	{
		if (camDirection != FVector::ZeroVector)
			MoveCamera(camDirection * camSpeed);
		//else
			//MoveCamera(GetCharacterMovement()->Velocity.IsNearlyZero(40.f) ? FVector::ZeroVector : GetCharacterMovement()->Velocity * -1);
	}
}

void ASpectatorCharacter::OnDirectedMoveStart()
{
	bMoveCommand = true;
}

void ASpectatorCharacter::OnDirectedMoveStop()
{
	bMoveCommand = false;
}

void ASpectatorCharacter::CalculateDirectedMove()
{
	ARealmPlayerController* pc = Cast<ARealmPlayerController>(GetWorld()->GetFirstPlayerController());
	FHitResult hit;

	if (pc && pc->GetHitResultUnderCursor(ECC_Visibility, true, hit))
	{
		if (hit.GetActor()->IsA(AGameCharacter::StaticClass()))
		{
			AGameCharacter* gc = Cast<AGameCharacter>(hit.GetActor());
			int32 team1 = gc->GetTeamIndex();
			int32 team2 = playerController->GetPlayerCharacter()->GetTeamIndex();

			if (gc && team1 != team2)// && !playerController->IsCharacterOnTeam(mc->GetTeam()))
				pc->ServerStartAutoAttack(gc);
			else
				pc->ServerMoveCommand(hit.Location);
		}
		else
			pc->ServerMoveCommand(hit.Location);
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("Unable to get mouse coordiantes."));
}

void ASpectatorCharacter::OnUseSkill(int32 index)
{
	FHitResult hit;

	if (playerController)
	{
		playerController->GetHitResultUnderCursor(ECC_Visibility, true, hit);
		playerController->ServerUseSkill(index, hit.Location);
	}
}

void ASpectatorCharacter::ToggleMovementSystem(bool bEnabled)
{
	if (bEnabled)
	{
		CalculateDirectedMove();

		if (!GetWorldTimerManager().IsTimerActive(movementTimer))
			GetWorldTimerManager().SetTimer(movementTimer, this, &ASpectatorCharacter::CalculateDirectedMove, 0.1f, true);
	}
	else
	{
		if (GetWorldTimerManager().IsTimerActive(movementTimer))
			GetWorldTimerManager().ClearTimer(movementTimer);
	}
}

FRotator ASpectatorCharacter::GetIsolatedCameraYaw()
{
	return FRotator(0.0f, rtsCamera->ComponentToWorld.Rotator().Yaw, 0.0f);
}

void ASpectatorCharacter::MoveCamera(FVector worldDirection)
{
	AddMovementInput(worldDirection, 50.f);
}

FVector ASpectatorCharacter::RightCameraMovement(float direction)
{
	float movementValue = direction * camSpeed;

	FVector deltaMovement = movementValue * (FRotator(0.0f, 90.0f, 0.0f) + GetIsolatedCameraYaw()).Vector();
	return deltaMovement;
}

FVector ASpectatorCharacter::ForwardCameraMovement(float direction)
{
	float movementValue = direction * camSpeed;

	FVector deltaMovement = movementValue * GetIsolatedCameraYaw().Vector();
	return deltaMovement;
}

void ASpectatorCharacter::OnSelfCameraLock()
{
	if (!IsValid(playerController) || !IsValid(playerController->GetPlayerCharacter()))
		return;

	LockCamera(playerController->GetPlayerCharacter());
}

void ASpectatorCharacter::LockCamera(AActor* focusedActor)
{
	springArm->AttachTo(focusedActor->GetRootComponent());
	AttachRootComponentTo(focusedActor->GetRootComponent());
	cameraLockTarget = focusedActor;
}

void ASpectatorCharacter::OnUnlockCamera()
{
	springArm->DetachFromParent(false);
	springArm->AttachTo(GetRootComponent());

	DetachRootComponentFromParent(false);

	if (cameraLockTarget)
		ServerSetLocation(cameraLockTarget->GetActorLocation());

	UnlockCamera();
}

void ASpectatorCharacter::UnlockCamera()
{
	cameraLockTarget = nullptr;
	bLockedOnCharacter = false;
}

bool ASpectatorCharacter::ServerSetLocation_Validate(FVector newLocation, AActor* attachActor = nullptr)
{
	return true;
}

void ASpectatorCharacter::ServerSetLocation_Implementation(FVector newLocation, AActor* attachActor = nullptr)
{
	SetActorLocation(newLocation);

	if (IsValid(attachActor))
		AttachRootComponentTo(attachActor->GetRootComponent());
	else
		DetachRootComponentFromParent(true);
}