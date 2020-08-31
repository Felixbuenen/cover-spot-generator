// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoverGenerationTrigger.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class COVERSPOTGENERATOR_API ACoverGenerationTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACoverGenerationTrigger();

	UPROPERTY(EditAnywhere)
	UBoxComponent* triggerBox;
	UPROPERTY(EditAnywhere)
	UBoxComponent* generationBox;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnGenerateCoverPoints(class UPrimitiveComponent* OverlapComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
