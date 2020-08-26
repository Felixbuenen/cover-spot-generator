// Fill out your copyright notice in the Description page of Project Settings.

#include "EnvQueryTest_CoverSpot_LooksAt.h"

#include "CoverDataStructures.h"
#include "CoverPointGenerator.h"
#include "EnvQueryItemType_CoverPoint.h"

#include "EngineUtils.h"

UEnvQueryTest_CoverSpot_LooksAt::UEnvQueryTest_CoverSpot_LooksAt(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ValidItemType = UEnvQueryItemType_CoverPoint::StaticClass();
	MinScoreViewingAngle.DefaultValue = 55.0f;
	MaxScoreViewingAngle.DefaultValue = 15.0f;
	FloatValueMax.DefaultValue = 75.0f;
	FilterType = EEnvTestFilterType::Maximum;
	ScoringFactor.DefaultValue = -1.0f;
}

void UEnvQueryTest_CoverSpot_LooksAt::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (QueryOwner == nullptr)
	{
		return;
	}

	MinScoreViewingAngle.BindData(QueryOwner, QueryInstance.QueryID);
	MaxScoreViewingAngle.BindData(QueryOwner, QueryInstance.QueryID);
	FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
	FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);

	float MaxViewingAngle = 0.0f, MinViewingAngle = 0.0f;

	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(LookAtContext, ContextLocations))
	{
		return;
	}

	UWorld* world = GetWorld();
	if (!IsValid(world)) return;

	TActorIterator<ACoverPointGenerator> it(world);
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
				It.SetScore(TestPurpose, FilterType, ScoreViewingAngle(cp, ContextLocations[ContextIndex]), FloatValueMin.GetValue(), FloatValueMax.GetValue());
			}
		}
	}
}

float UEnvQueryTest_CoverSpot_LooksAt::ScoreViewingAngle(const UCoverPoint* cp, const FVector& targetLocation) const
{
	// calculate viewing angle, which is the angle between the cover point view direction and the target location
	FVector dirToTarget = targetLocation - cp->_location;
	dirToTarget.Normalize();
	float angle = FMath::Acos(FVector::DotProduct(cp->_dirToCover, dirToTarget));
	angle = FMath::RadiansToDegrees(angle);
	angle -= MaxScoreViewingAngle.GetValue();
	
	// scale the value such that it fits within the min- and max score viewing angle parameters
	float viewRange = MinScoreViewingAngle.GetValue() - MaxScoreViewingAngle.GetValue();
	angle = FMath::Clamp((angle / viewRange), 0.0f, 1.0f);

	return angle;
}

FText UEnvQueryTest_CoverSpot_LooksAt::GetDescriptionTitle() const
{
	return FText::FromString(FString::Printf(TEXT("CoverSpot look at %s."),
		*UEnvQueryTypes::DescribeContext(LookAtContext).ToString()));
}

FText UEnvQueryTest_CoverSpot_LooksAt::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("CoverSpot looks in direction of %s."),
		*UEnvQueryTypes::DescribeContext(LookAtContext).ToString()));
}