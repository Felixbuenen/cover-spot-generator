// Fill out your copyright notice in the Description page of Project Settings.


#include "CoverGenerationTrigger.h"

#include "CoverPointGenerator.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

// Sets default values
ACoverGenerationTrigger::ACoverGenerationTrigger()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	triggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger Box"));
	generationBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Generation Box"));

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

// Called when the game starts or when spawned
void ACoverGenerationTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	triggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACoverGenerationTrigger::OnGenerateCoverPoints);
	triggerBox->SetHiddenInGame(false);
}


void ACoverGenerationTrigger::OnGenerateCoverPoints(UPrimitiveComponent* OverlapComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UWorld* world = GetWorld();
	if (!IsValid(world)) return;

	AActor* player = UGameplayStatics::GetPlayerPawn(world, 0);
	if (OtherActor != player) return;

	FVector extent = generationBox->GetScaledBoxExtent();
	FVector center = generationBox->GetComponentLocation();
	FBox bbox(center - extent, center + extent);

	ACoverPointGenerator* cpg = ACoverPointGenerator::Get(world);
	if (IsValid(cpg))
	{
		cpg->UpdateCoverpointData(bbox);
	}
}


