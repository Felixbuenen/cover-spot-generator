// Fill out your copyright notice in the Description page of Project Settings.

#include "EnvQueryTest_CoverSpot_IsSafe.h"

#include "../Generator/CoverDataStructures.h"
#include "../Generator/CoverPointGenerator.h"
#include "EnvQueryItemType_CoverPoint.h"

#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

UEnvQueryTest_CoverSpot_IsSafe::UEnvQueryTest_CoverSpot_IsSafe(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ValidItemType = UEnvQueryItemType_CoverPoint::StaticClass();
	
	TestPurpose = EEnvTestPurpose::Filter;
	FilterType = EEnvTestFilterType::Match;

	MyTraceHeight.DefaultValue = 40.0f;
	EnemyTraceHeight.DefaultValue = 80.0f;
	TestRadius.DefaultValue = 30.0f;

	DrawSafeFromAboveTest.DefaultValue = false;
	DrawSafeFromSideTest.DefaultValue = false;
}

void UEnvQueryTest_CoverSpot_IsSafe::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (QueryOwner == nullptr)
	{
		return;
	}

	EnemyTraceHeight.BindData(QueryOwner, QueryInstance.QueryID);
	TestRadius.BindData(QueryOwner, QueryInstance.QueryID);
	MyTraceHeight.BindData(QueryOwner, QueryInstance.QueryID);
	DrawSafeFromAboveTest.BindData(QueryOwner, QueryInstance.QueryID);
	DrawSafeFromSideTest.BindData(QueryOwner, QueryInstance.QueryID);

	TArray<AActor*> ContextActors;
	if (!QueryInstance.PrepareContext(SafeFrom, ContextActors))
	{
		return;
	}

	UWorld* world = GetWorld();
	if (!IsValid(world)) return;

	const ACoverPointGenerator* cpg = ACoverPointGenerator::Get(world);
	if (!IsValid(cpg))
	{
		UE_LOG(LogTemp, Warning, TEXT("EQS cover test: no generator"));
		return;
	}

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const UCoverPoint* cp = UEnvQueryItemType_CoverPoint::GetValue(It.GetItemData());

		if (!IsValid(cp))
		{
			UE_LOG(LogTemp, Warning, TEXT("invalid cover point"));
			continue;
		}

		for (int32 ContextIndex = 0; ContextIndex < ContextActors.Num(); ContextIndex++)
		{
			bool isSafe = CoverProvidesSafety(world, ContextActors[ContextIndex], cp, cpg);
			It.SetScore(TestPurpose, FilterType, isSafe, true);
		}
	}
}

bool UEnvQueryTest_CoverSpot_IsSafe::CoverProvidesSafety(UWorld* world, const AActor* context, const UCoverPoint* coverPoint, const ACoverPointGenerator* cpg) const
{	
	// check outer point that may be visible from side
	FVector sideOffset = coverPoint->_leanDirection;
	sideOffset.Z = 0.0f;
	FVector backOffset = -1.0f * coverPoint->_dirToCover;
	backOffset.Z = 0.0f;

	FVector outerBodyPointDir = sideOffset + backOffset;
	outerBodyPointDir.Normalize();

	FVector traceStart = coverPoint->_location + outerBodyPointDir * TestRadius.GetValue();
	traceStart.Z += MyTraceHeight.GetValue();
	FVector traceEnd = context->GetActorLocation();
	traceEnd.Z += EnemyTraceHeight.GetValue();
	FHitResult outHit2;
	UKismetSystemLibrary::LineTraceSingle(world, traceStart, traceEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHit2, true);
	bool isSafeFromSide = outHit2.bBlockingHit ? (outHit2.Actor != context) : false;
	
	if (DrawSafeFromSideTest.GetValue())
	{
		DrawDebugLine(world, traceStart, traceEnd, FColor::Green, false, 1.0f);
		DrawDebugSphere(world, traceStart, 30.0f, 12, FColor::Emerald, false, 1.0f);
	}

	// check if the enemy can attack agent from above at this cover position
	bool isSafeFromAbove = true;
	if (coverPoint->_leanDirection.Z > 0.0f)
	{
		// check from center 
		FVector traceStart = coverPoint->_location + -1.0f * coverPoint->_dirToCover * TestRadius.GetValue();
		traceStart.Z += MyTraceHeight.GetValue();
		FVector traceEnd = context->GetActorLocation();
		traceEnd.Z += EnemyTraceHeight.GetValue();
		
		FHitResult outHit;
		UKismetSystemLibrary::LineTraceSingle(world, traceStart, traceEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHit, true);
		isSafeFromAbove = outHit.bBlockingHit ? (outHit.Actor != context) : false;

		if (DrawSafeFromAboveTest.GetValue())
		{
			DrawDebugLine(world, traceStart, traceEnd, FColor::Green, false, 1.0f);
			DrawDebugSphere(world, traceStart, 10.0f, 12, FColor::Emerald, false, 1.0f);
		}
	}

	return (isSafeFromSide && isSafeFromAbove);
}

FText UEnvQueryTest_CoverSpot_IsSafe::GetDescriptionTitle() const
{
	return FText::FromString("Cover Is Safe");
}

FText UEnvQueryTest_CoverSpot_IsSafe::GetDescriptionDetails() const
{
	return FText::FromString(FString::Printf(TEXT("CoverSpot provides cover from %s."),
		*UEnvQueryTypes::DescribeContext(SafeFrom).ToString()));
}