// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "ResourceNode.generated.h"

UCLASS()
class SIMULATIONGAME_API AResourceNode : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	AResourceNode();

protected:
	virtual void BeginPlay() override;

	// 木や石の見た目（3Dモデル）を設定するためのコンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComponent;

	// 資源の耐久値（何回叩けるか）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 Durability = 3;

	// このリソースノードがドロップするアイテムの種類
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	FString DroppedItemName = TEXT("Wood");

	// ドロップするアイテムの数量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 ItemDropQuantity = 1;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void Interact_Implementation(AActor* Interactor);
}; 
