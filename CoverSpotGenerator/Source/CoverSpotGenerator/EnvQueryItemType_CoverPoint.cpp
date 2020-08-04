// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryItemType_CoverPoint.h"
#include "EnvironmentQuery/Items/EnvQueryItemType.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UEnvQueryItemType_CoverPoint::UEnvQueryItemType_CoverPoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ValueSize = sizeof(FWeakObjectPtr);
}

UCoverPoint* UEnvQueryItemType_CoverPoint::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<UCoverPoint*>(RawData);
}

void UEnvQueryItemType_CoverPoint::SetValue(uint8* RawData, UCoverPoint* Value)
{
	SetValueInMemory<UCoverPoint*>(RawData, Value);
}

FVector UEnvQueryItemType_CoverPoint::GetItemLocation(const uint8* RawData) const
{
	UCoverPoint* coverPoint = GetValue(RawData);
	return coverPoint ? coverPoint->_location : FVector::ZeroVector;
}

FString UEnvQueryItemType_CoverPoint::GetDescription(const uint8* RawData) const
{
	return FString("Cover Point");
}

void UEnvQueryItemType_CoverPoint::AddBlackboardFilters(FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const
{
	Super::AddBlackboardFilters(KeySelector, FilterOwner);
}

bool UEnvQueryItemType_CoverPoint::StoreInBlackboard(FBlackboardKeySelector& KeySelector, UBlackboardComponent* Blackboard, const uint8* RawData) const
{
	// if vector blackboard-key is passed: only get location
	bool bStored = Super::StoreInBlackboard(KeySelector, Blackboard, RawData);

	// if object blackboard-key is passed: get complete cover point object
	if (!bStored && KeySelector.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UObject* CoverObject = GetValue(RawData);
		Blackboard->SetValue<UBlackboardKeyType_Object>(KeySelector.GetSelectedKeyID(), CoverObject);

		bStored = true;
	}

	return bStored;
}