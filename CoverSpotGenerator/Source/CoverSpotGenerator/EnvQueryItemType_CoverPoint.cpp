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
	FWeakObjectPtr WeakObjPtr = GetValueFromMemory<FWeakObjectPtr>(RawData);
	return (UCoverPoint*)(WeakObjPtr.Get());
}

void UEnvQueryItemType_CoverPoint::SetValue(uint8* RawData, const FWeakObjectPtr& Value)
{
	FWeakObjectPtr WeakObjPtr(Value);
	SetValueInMemory<FWeakObjectPtr>(RawData, WeakObjPtr);
}

FVector UEnvQueryItemType_CoverPoint::GetItemLocation(const uint8* RawData) const
{
	UCoverPoint* coverPoint = UEnvQueryItemType_CoverPoint::GetValue(RawData);
	return coverPoint ? coverPoint->_location : FVector::ZeroVector;
}

FVector UEnvQueryItemType_CoverPoint::GetItemDirection(const uint8* RawData) const
{
	UCoverPoint* coverPoint = UEnvQueryItemType_CoverPoint::GetValue(RawData);
	return coverPoint ? coverPoint->_dirToCover : FVector::ZeroVector;
}

FVector UEnvQueryItemType_CoverPoint::GetItemLeanDirection(const uint8* RawData) const
{
	UCoverPoint* coverPoint = UEnvQueryItemType_CoverPoint::GetValue(RawData);
	return coverPoint ? coverPoint->_leanDirection : FVector::ZeroVector;
}

UCoverPoint* UEnvQueryItemType_CoverPoint::GetCoverPoint(const uint8* RawData) const
{
	UCoverPoint* coverPoint = UEnvQueryItemType_CoverPoint::GetValue(RawData);
	return coverPoint;
}

FString UEnvQueryItemType_CoverPoint::GetDescription(const uint8* RawData) const
{
	return FString("Cover Point");
}

bool UEnvQueryItemType_CoverPoint::StoreInBlackboard(FBlackboardKeySelector& KeySelector, UBlackboardComponent* Blackboard, const uint8* RawData) const
{
	bool bStored = Super::StoreInBlackboard(KeySelector, Blackboard, RawData);
	if (KeySelector.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UObject* MyObject = GetCoverPoint(RawData);
		Blackboard->SetValue<UBlackboardKeyType_Object>(KeySelector.GetSelectedKeyID(), MyObject);

		bStored = true;
	}

	return bStored;
}