// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimulationGameCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "InteractableInterface.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ASimulationGameCharacter

ASimulationGameCharacter::ASimulationGameCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASimulationGameCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASimulationGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASimulationGameCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASimulationGameCharacter::Look);

		// Interact
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ASimulationGameCharacter::PerformInteraction);

		// Inventory
		if (InventoryAction)
		{
			EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &ASimulationGameCharacter::ToggleInventoryUI);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASimulationGameCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASimulationGameCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASimulationGameCharacter::PerformInteraction()
{
	FVector StartLoc = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	FVector ForwardVector = GetActorForwardVector();
	FVector EndLoc = StartLoc + (ForwardVector * InteractionRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLoc,
		EndLoc,
		ECC_Visibility,
		QueryParams
	);

	if (bHit)
	{
		// 衝突したところに「緑色の点」を描画する
		DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 10.0f, FColor::Green, false, 2.0f);

		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			// 衝突したオブジェクトの名前を「赤色の文字」で出力する
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, FString::Printf(TEXT("Hit: %s"), *HitActor->GetName()));

			if (HitActor->Implements<UInteractableInterface>())
			{
				// インターフェースを持ってた場合「マゼンタ（赤紫）の文字」を出す
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Magenta, TEXT("Interface implemented!"));
				IInteractableInterface::Execute_Interact(HitActor, this);
			}
			else
			{
				// インターフェースを持ってない場合「赤色の文字」を出す
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Interface not implemented!"));
			}
		}
	}
	else
	{
		// 何も当たった線「赤い線」を描く
		DrawDebugLine(GetWorld(), StartLoc, EndLoc, FColor::Red, false, 2.0f, 0, 2.0f);
	}
}

bool ASimulationGameCharacter::AddItemToInventory(const FItemData& Item)
{
	// インベントリが満杯かどうか確認
	if (InventoryItems.Num() >= MaxInventorySlots)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Inventory is full!"));
		}
		return false;
	}

	// 同じアイテムタイプが既に存在するか確認
	for (FItemData& ExistingItem : InventoryItems)
	{
		if (ExistingItem.ItemType == Item.ItemType)
		{
			// 既存のアイテムに数量を追加
			ExistingItem.Quantity += Item.Quantity;
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
					FString::Printf(TEXT("Acquired: %s x %d"), *Item.ItemName, Item.Quantity));
			}
			return true;
		}
	}

	// 新しいアイテムを追加
	InventoryItems.Add(Item);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
			FString::Printf(TEXT("Acquired: %s x %d"), *Item.ItemName, Item.Quantity));
	}
	return true;
}

void ASimulationGameCharacter::ToggleInventoryUI()
{
	bInventoryVisible = !bInventoryVisible;

	if (bInventoryVisible)
	{
		DisplayInventoryUI();
	}
}

void ASimulationGameCharacter::DisplayInventoryUI()
{
	if (!GEngine)
	{
		return;
	}

	// インベントリが空の場合
	if (InventoryItems.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, TEXT("========== Inventory =========="));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, TEXT("Inventory is empty"));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, TEXT("================================"));
		return;
	}

	// インベントリヘッダーを表示
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, TEXT("========== Inventory =========="));

	int32 ItemIndex = 1;
	int32 TotalItems = 0;

	// 各アイテムを表示
	for (const FItemData& Item : InventoryItems)
	{
		FString ItemTypeStr;
		switch (Item.ItemType)
		{
			case EItemType::Wood:
				ItemTypeStr = TEXT("Wood");
				break;
			case EItemType::Stone:
				ItemTypeStr = TEXT("Stone");
				break;
			case EItemType::Iron:
				ItemTypeStr = TEXT("Iron");
				break;
			case EItemType::Gold:
				ItemTypeStr = TEXT("Gold");
				break;
			default:
				ItemTypeStr = TEXT("Unknown");
				break;
		}

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, 
			FString::Printf(TEXT("[%d] %s x %d"), ItemIndex, *ItemTypeStr, Item.Quantity));

		TotalItems += Item.Quantity;
		ItemIndex++;
	}

	// フッター情報を表示
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, 
		FString::Printf(TEXT("Items: %d / %d"), InventoryItems.Num(), MaxInventorySlots));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, 
		FString::Printf(TEXT("Total: %d"), TotalItems));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, TEXT("================================"));
}