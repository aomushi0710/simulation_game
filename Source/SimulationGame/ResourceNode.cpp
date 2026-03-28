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

void AResourceNode::Interact_Implementation(AActor* InstigatorActor)
{
	Durability--;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("資源を叩いた！ 残り耐久値: %d"), Durability));
	}

	if (Durability <= 0)
	{
		// 叩いた相手（InstigatorActor）をキャラクターとして認識させる
		ASimulationGameCharacter* PlayerCharacter = Cast<ASimulationGameCharacter>(InstigatorActor);
		if (PlayerCharacter)
		{
			PlayerCharacter->AddItem(FName(TEXT("木材")), 1);
		}

		Destroy();
	}
} 
