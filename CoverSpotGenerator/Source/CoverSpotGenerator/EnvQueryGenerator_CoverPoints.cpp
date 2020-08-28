// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryGenerator_CoverPoints.h"

#include "CoverPointGenerator.h"
#include "EnvQueryItemType_CoverPoint.h"

#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EngineUtils.h"

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

	// get the cover point generator
	UWorld* world = GetWorld();
	if (!IsValid(world)) return;

	const ACoverPointGenerator* cpg = ACoverPointGenerator::Get(world);
	if (!IsValid(cpg))
	{
		UE_LOG(LogTemp, Error, TEXT("EQS cover generator: no generator found. Make sure there is a CoverSpotGenerator in the scene."));
		return;
	}

	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		TArray<UCoverPoint*> CoverPoints = cpg->GetCoverPointsWithinExtent(ContextLocations[ContextIndex], BboxExtent.GetValue());
		QueryInstance.AddItemData<UEnvQueryItemType_CoverPoint>(CoverPoints);
	}
}

FText UEnvQueryGenerator_CoverPoints::GetDescriptionTitle() const
{
	return FText::FromString("Cover Spot Generator");
}

FText UEnvQueryGenerator_CoverPoints::GetDescriptionDetails() const
{
	return FText::FromString("Cover spots (range defined by bbox) around querier");
}