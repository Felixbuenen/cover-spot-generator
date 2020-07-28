// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "CoverDataStructures.h"
#include "EnvQueryItemType_CoverPoint.generated.h"

UCLASS()
class COVERSPOTGENERATOR_API UEnvQueryItemType_CoverPoint : public UEnvQueryItemType_VectorBase
{
	GENERATED_UCLASS_BODY()
	
public:
	typedef UCoverPoint* FValueType; // FValueType is used as an abstract type by the EQS system: here we define it

	static UCoverPoint* GetValue(const uint8* RawData);
	static void SetValue(uint8* RawData, UCoverPoint* Value);
	
	virtual FVector GetItemLocation(const uint8* RawData) const;

	virtual void AddBlackboardFilters(FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const override;
	virtual bool StoreInBlackboard(FBlackboardKeySelector& KeySelector, UBlackboardComponent* Blackboard, const uint8* RawData) const override;
	virtual FString GetDescription(const uint8* RawData) const override;
};
