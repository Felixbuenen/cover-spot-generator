// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryGenerator_CoverPoints.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

#include "CoverPointGenerator.h"

UEnvQueryGenerator_CoverPoints::UEnvQueryGenerator_CoverPoints(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	GenerateAround = UEnvQueryContext_Querier::StaticClass();

	BboxExtent.DefaultValue = 500.0f;
	MaxNumCoverSpots.DefaultValue = -1;
}

void UEnvQueryGenerator_CoverPoints::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	UE_LOG(LogTemp, Warning, TEXT("generating items..."));


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

	TArray<FNavLocation> CoverPointLocations;
	//CoverPoints.Reserve( * ContextLocations.Num());

	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		TArray<UCoverPoint*> coverPoints = gen->GetCoverPointsWithinExtent(ContextLocations[ContextIndex], BboxExtent.GetValue());
		for (auto cp : coverPoints)
		{
			CoverPointLocations.Emplace(cp->_location);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("num points: %d"), CoverPointLocations.Num());

	ProjectAndFilterNavPoints(CoverPointLocations, QueryInstance);
	StoreNavPoints(CoverPointLocations, QueryInstance);
}

FText UEnvQueryGenerator_CoverPoints::GetDescriptionTitle() const
{
	return FText::FromString("TEST DESCRIPTION TITLE");
}

FText UEnvQueryGenerator_CoverPoints::GetDescriptionDetails() const
{
	return FText::FromString("TEST DESCRIPTION");
}