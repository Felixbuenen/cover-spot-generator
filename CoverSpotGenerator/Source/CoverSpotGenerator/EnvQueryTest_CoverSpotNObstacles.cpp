// Fill out your copyright notice in the Description page of Project Settings.

#include "EnvQueryTest_CoverSpotNObstacles.h"

#include "EnvQueryItemType_CoverPoint.h"
#include "CoverPointGenerator.h"

#include "EngineUtils.h"
#include "AISystem.h"

UEnvQueryTest_CoverSpotNObstacles::UEnvQueryTest_CoverSpotNObstacles(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ValidItemType = UEnvQueryItemType_CoverPoint::StaticClass();
	ScoringFactor.DefaultValue = -1.0; // by default, prefer less obstacles between querier and target
}

void UEnvQueryTest_CoverSpotNObstacles::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (QueryOwner == nullptr)
	{
		return;
	}

	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
	float MinFilterThresholdValue = FloatValueMin.GetValue();

	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	float MaxFilterThresholdValue = FloatValueMax.GetValue();

	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(Context, ContextLocations))
	{
		return;
	}

	TActorIterator<ACoverPointGenerator> it(GetWorld());
	const ACoverPointGenerator* cpg = *it;
	
	if (!IsValid(cpg))
	{
		UE_LOG(LogTemp, Warning, TEXT("EQS cover test: no generator found. Make sure there is a CoverSpotGenerator in the scene."));
		return;
	}
	
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const UCoverPoint* cp = UEnvQueryItemType_CoverPoint::GetValue(It.GetItemData());
		
		if (IsValid(cp))
		{
			for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
			{
				float score = (float)(cpg->GetNumberOfIntersectionsFromCover(cp, ContextLocations[ContextIndex]));
				It.SetScore(TestPurpose, FilterType, score, MinFilterThresholdValue, MaxFilterThresholdValue);
			}
		}
	}
}

FText UEnvQueryTest_CoverSpotNObstacles::GetDescriptionTitle() const
{
	return FText::FromString("Number of obstacles");
}

FText UEnvQueryTest_CoverSpotNObstacles::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("Number of obstacles between querier and %s."), 
		*UEnvQueryTypes::DescribeContext(Context).ToString()));
}