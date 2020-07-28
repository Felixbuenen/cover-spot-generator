// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"

#include "DataProviders/AIDataProvider.h"

#include "EnvQueryGenerator_CoverPoints.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Cover Points"))
class COVERSPOTGENERATOR_API UEnvQueryGenerator_CoverPoints : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Cover Point Parameters")
	FAIDataProviderFloatValue BboxExtent;

	UPROPERTY(EditDefaultsOnly, Category = "Cover Point Parameters")
	FAIDataProviderIntValue MaxNumCoverSpots;

	UPROPERTY(EditDefaultsOnly, Category = Generator)
	TSubclassOf<UEnvQueryContext> GenerateAround;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
