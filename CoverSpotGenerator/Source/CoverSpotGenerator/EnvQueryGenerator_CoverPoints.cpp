// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryGenerator_CoverPoints.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "EnvQueryItemType_CoverPoint.h"

#include "CoverPointGenerator.h"

UEnvQueryGenerator_CoverPoints::UEnvQueryGenerator_CoverPoints(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ItemType = UEnvQueryItemType_CoverPoint::StaticClass();
	GenerateAround = UEnvQueryContext_Querier::StaticClass();

	BboxExtent.DefaultValue = 500.0f;
	MaxNumCoverSpots.DefaultValue = -1;
}

void UEnvQueryGenerator_CoverPoints::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	// bind data
	UObject* BindOwner = QueryInstance.Owner.Get();
	BboxExtent.BindData(BindOwner, QueryInstance.QueryID);

	// bind context data
	TArray<FVector> ContextLocations;
	QueryInstance.PrepareContext(GenerateAround, ContextLocations);

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACoverPointGenerator::StaticClass(), FoundActors);
	if (FoundActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("no generator"));
		return;
	}

	ACoverPointGenerator* gen = (ACoverPointGenerator*)FoundActors[0];
	if (gen == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("cannot cast to coverpointgenerator"));
		return;
	}


	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		TArray<UCoverPoint*> CoverPoints = gen->GetCoverPointsWithinExtent(ContextLocations[ContextIndex], BboxExtent.GetValue());
		QueryInstance.AddItemData<UEnvQueryItemType_CoverPoint>(CoverPoints);

		UE_LOG(LogTemp, Warning, TEXT("coverpoint[0] = %s"), *CoverPoints[0]->_location.ToString());
	}
}

FText UEnvQueryGenerator_CoverPoints::GetDescriptionTitle() const
{
	return FText::FromString("TEST DESCRIPTION TITLE");
}

FText UEnvQueryGenerator_CoverPoints::GetDescriptionDetails() const
{
	return FText::FromString("TEST DESCRIPTION");
}