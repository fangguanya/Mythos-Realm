#include "Realm.h"
#include "RealmPlayerController.h"
#include "RealmMoveController.h"
#include "PlayerCharacter.h"
#include "SpectatorCharacter.h"
#include "UnrealNetwork.h"
#include "PlayerHUD.h"
#include "Mod.h"
#include "RealmPlayerState.h"
#include "RealmGameMode.h"

ARealmPlayerController::ARealmPlayerController(const FObjectInitializer& objectInitializer)
:Super(objectInitializer)
{
	bShowMouseCursor = true;

	//debug code
	static ConstructorHelpers::FClassFinder<APlayerCharacter> PlayerPawnBPClass(TEXT("/Game/Realm/Characters/PCs/Leighton/Leighton"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		defaultCharacterClass = PlayerPawnBPClass.Class;
	}
}

void ARealmPlayerController::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ACameraActor> camitr(GetWorld()); camitr; ++camitr)
	{
		ACameraActor* ca = (*camitr);
		if (ca->Tags.Contains("startCam"))
			SetViewTarget((*camitr));
	}
}

void ARealmPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("IngameStore", IE_Pressed, this, &ARealmPlayerController::OnToggleIngameStore);
	InputComponent->BindAction("TargetClick", IE_Pressed, this, &ARealmPlayerController::OnTargetSelect);
}

void ARealmPlayerController::Possess(APawn* aPawn)
{
	Super::Possess(aPawn);

	//@todo: get the character class the player chose to play with

	if (!IsValid(moveController) && Role == ROLE_Authority)
	{
		moveController = GetWorld()->SpawnActor<ARealmMoveController>(FVector::ZeroVector, FRotator::ZeroRotator);
		moveController->SetOwner(this);

		ASpectatorCharacter* sc = Cast<ASpectatorCharacter>(aPawn);
		if (sc)
		{
			sc->SetPlayerController(this);
			sc->GetCharacterMovement()->SetMovementMode(MOVE_Flying);;
			//sc->GetCharacterMovement()->MaxFlySpeed = 600.f;
			SetViewTarget(sc);
		}

		if (!playerCharacter)
		{
			APlayerCharacter* pc = GetWorld()->SpawnActor<APlayerCharacter>(GetDefaultCharacterClass(), aPawn->GetActorLocation(), aPawn->GetActorRotation());
			ARealmPlayerState* ps = Cast<ARealmPlayerState>(PlayerState);

			if (pc)
			{
				moveController->Possess(pc);
				playerCharacter = pc;
				playerCharacter->SetPlayerController(this);
				playerCharacter->SetOwner(this);

				if (ps)
					playerCharacter->SetTeamIndex(ps->GetTeamIndex());
			}
		}
	}
}

bool ARealmPlayerController::ServerMoveCommand_Validate(FVector targetLocation)
{
	return true;
}

void ARealmPlayerController::ServerMoveCommand_Implementation(FVector targetLocation)
{
	if (!IsValid(this))
		return;
	
	ServerClearAttackCommands();

	if (IsValid(moveController))
		moveController->MoveToLocation(targetLocation);
}

bool ARealmPlayerController::ServerStartAutoAttack_Validate(AGameCharacter* target)
{
	return IsValid(target);
}

void ARealmPlayerController::ServerStartAutoAttack_Implementation(AGameCharacter* target)
{
	if (!IsValid(playerCharacter))
		return;

	if (playerCharacter->GetCurrentTarget() == target)
		return;

	playerCharacter->SetCurrentTarget(target);
	GetPlayerInAutoAttackRange();
}

bool ARealmPlayerController::ServerClearMoveCommands_Validate()
{
	return true;
}

void ARealmPlayerController::ServerClearMoveCommands_Implementation()
{
	if (!IsValid(moveController))
		return;

	moveController->StopMovement();
}

bool ARealmPlayerController::ServerClearAttackCommands_Validate()
{
	return true;
}

void ARealmPlayerController::ServerClearAttackCommands_Implementation()
{
	if (!IsValid(playerCharacter))
		return;

	playerCharacter->SetCurrentTarget(nullptr);
	GetWorldTimerManager().ClearTimer(aaRangeTimer);
	playerCharacter->StopAutoAttack();
}

bool ARealmPlayerController::ServerUseSkill_Validate(int32 index, FVector mouseHitLoc)
{
	return true;
}

void ARealmPlayerController::ServerUseSkill_Implementation(int32 index, FVector mouseHitLoc)
{
	if (!IsValid(playerCharacter))
		return;

	playerCharacter->UseSkill(index, mouseHitLoc, playerCharacter->currentTarget);
}

bool ARealmPlayerController::ServerChooseCharacter_Validate(TSubclassOf<APlayerCharacter> chosenCharacter)
{
	return chosenCharacter != nullptr;
}

void ARealmPlayerController::ServerChooseCharacter_Implementation(TSubclassOf<APlayerCharacter> chosenCharacter)
{
	GetWorld()->GetAuthGameMode<ARealmGameMode>()->PlayerSelectedCharacter(this, chosenCharacter);
}

void ARealmPlayerController::GetPlayerInAutoAttackRange()
{
	if (!IsValid(playerCharacter) || !IsValid(playerCharacter->GetCurrentTarget()) || !IsValid(moveController))
		return;

	if (!playerCharacter->GetCurrentTarget()->IsAlive())
	{
		moveController->NeedsNewCommand();
		return;
	}

	float distanceSq = (playerCharacter->GetActorLocation() - playerCharacter->GetCurrentTarget()->GetActorLocation()).SizeSquared2D();
	float aaRange = playerCharacter->GetAutoAttackManager()->GetCurrentAutoAttackRangeSquared();
	if (distanceSq <= aaRange)
		playerCharacter->StartAutoAttack();
	else
	{
		moveController->MoveToActor(playerCharacter->GetCurrentTarget());
		GetWorldTimerManager().SetTimer(aaRangeTimer, this, &ARealmPlayerController::GetPlayerInAutoAttackRange, 0.05f);
	}
}

TSubclassOf<APlayerCharacter> ARealmPlayerController::GetDefaultCharacterClass() const
{
	ARealmPlayerState* ps = Cast<ARealmPlayerState>(PlayerState);
	if (IsValid(ps) && IsValid(ps->GetChosenCharacterClass()))
		return ps->GetChosenCharacterClass();
	
	return nullptr;
}

APlayerCharacter* ARealmPlayerController::GetPlayerCharacter() const
{
	return playerCharacter;
}

void ARealmPlayerController::ClientInitIngameStore_Implementation(const TArray<TSubclassOf<AMod> >& modStore)
{
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
		hud->InitIngameStore(modStore);
}

void ARealmPlayerController::ClientOpenPregameScreen_Implementation()
{
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
		hud->OpenPregameScreen();
}

void ARealmPlayerController::ClientOpenCharacterSelectScreen_Implementation(const TArray<TSubclassOf<APlayerCharacter> >& availableCharacters)
{
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
		hud->OpenCharacterSelectScreen(availableCharacters);
}

void ARealmPlayerController::ClientOpenPlayerHUD_Implementation()
{
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
		hud->OpenPlayerHUD();
}

void ARealmPlayerController::OnOpenIngameStore()
{
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
		hud->OpenInGameStore();
}

void ARealmPlayerController::OnCloseIngameStore()
{
	APlayerHUD* hud = Cast<APlayerHUD>(GetHUD());
	if (IsValid(hud))
		hud->CloseInGameStore();
}

void ARealmPlayerController::OnToggleIngameStore()
{
	bIngameStoreOpen = !bIngameStoreOpen;
	if (bIngameStoreOpen)
		OnOpenIngameStore();
	else
		OnCloseIngameStore();
}

void ARealmPlayerController::OnTargetSelect()
{
	FHitResult hit;

	if (GetHitResultUnderCursor(ECC_Visibility, true, hit))
	{

	}
}

bool ARealmPlayerController::ServerBuyPlayerMod_Validate(TSubclassOf<AMod> wantedMod)
{
	return wantedMod != nullptr;
}

void ARealmPlayerController::ServerBuyPlayerMod_Implementation(TSubclassOf<AMod> wantedMod)
{
	if (GetPlayerCharacter()->GetModCount() + 1 > 6)
		return;

	AMod* modToBuy = Cast<AMod>(wantedMod->GetDefaultObject());
	if (modToBuy)
	{
		AMod* modToAdd;
		if (GetPlayerCharacter()->GetCredits() >= modToBuy->GetCost())
		{
			modToAdd = GetWorld()->SpawnActor<AMod>(wantedMod, GetCharacter()->GetActorLocation(), GetCharacter()->GetActorRotation());
			if (IsValid(modToAdd))
			{
				GetPlayerCharacter()->AddMod(modToAdd);
				GetPlayerCharacter()->ChangeCredits(-1 * modToAdd->GetCost());
			}
		}
	}
}

bool ARealmPlayerController::ServerSellPlayerMod_Validate(int32 index)
{
	return (index >= 0 && index < 7);
}

void ARealmPlayerController::ServerSellPlayerMod_Implementation(int32 index)
{
	
}

ARealmMoveController* ARealmPlayerController::GetMoveController() const
{
	return moveController;
}

void ARealmPlayerController::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARealmPlayerController, playerCharacter);
}