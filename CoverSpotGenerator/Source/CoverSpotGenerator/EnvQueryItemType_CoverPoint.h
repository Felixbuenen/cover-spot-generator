// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "CoverDataStructures.h"
#include "EnvQueryItemType_CoverPoint.generated.h"

UCLASS()
class COVERSPOTGENERATOR_API UEnvQueryItemType_CoverPoint : public UEnvQueryItemType_VectorBase
{
	GENERATED_BODY()
	
public:
	UEnvQueryItemType_CoverPoint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static UCoverPoint* GetValue(const uint8* RawData);
	static void SetValue(uint8* RawData, const FWeakObjectPtr& Value);
	
	FVector GetItemLocation(const uint8* RawData) const;
	FVector GetItemDirection(const uint8* RawData) const;
	FVector GetItemLeanDirection(const uint8* RawData) const;
	UCoverPoint* GetCoverPoint(const uint8* RawData) const;

	virtual bool StoreInBlackboard(FBlackboardKeySelector& KeySelector, UBlackboardComponent* Blackboard, const uint8* RawData) const override;
	virtual FString GetDescription(const uint8* RawData) const override;
};
