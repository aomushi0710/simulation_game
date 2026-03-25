// Fill out your copyright notice in the Description page of Project Settings.

#include "ResourceNode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "SimulationGameCharacter.h"

// Sets default values
AResourceNode::AResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

// Called when the game starts or when spawned
void AResourceNode::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AResourceNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AResourceNode::Interact_Implementation(AActor* Interactor)
{
	// ダメージ値を1ずつ下す
	Health--;

	// 画面の右上の青色でメッセージを出す(2秒表示)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("Tree hit! Health: %d"), Health));
	}

	// ダメージ値が0以下になった場合
	if (Health <= 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Tree destroyed!"));
		}

		// Interactorをキャスト
		ASimulationGameCharacter* Character = Cast<ASimulationGameCharacter>(Interactor);
		if (Character)
		{
			// ドロップアイテムをインベントリに追加
			FItemData ItemToAdd;
			ItemToAdd.ItemType = EItemType::Wood;
			ItemToAdd.ItemName = DroppedItemName;
			ItemToAdd.Quantity = ItemDropQuantity;
			Character->AddItemToInventory(ItemToAdd);
		}

		// このオブジェクトを削除
		Destroy();
	}
}
