// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvQueryTest_CoverSpot_IsSafe.generated.h"

UCLASS()
class COVERSPOTGENERATOR_API UEnvQueryTest_CoverSpot_IsSafe : public UEnvQueryTest
{
	GENERATED_UCLASS_BODY()

	/** context */
	UPROPERTY(EditDefaultsOnly, Category = Cover)
	TSubclassOf<UEnvQueryContext> SafeFrom;

	UPROPERTY(EditDefaultsOnly, Category = Cover)
	FAIDataProviderFloatValue MyTraceHeight;

	UPROPERTY(EditDefaultsOnly, Category = Cover)
	FAIDataProviderFloatValue EnemyTraceHeight;

	UPROPERTY(EditDefaultsOnly, Category = Cover)
	FAIDataProviderFloatValue TestRadius;

	UPROPERTY(EditDefaultsOnly, Category = Debug)
	FAIDataProviderBoolValue DrawSafeFromAboveTest;

	UPROPERTY(EditDefaultsOnly, Category = Debug)
	FAIDataProviderBoolValue DrawSafeFromSideTest;

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

protected:
	bool CoverProvidesSafety(UWorld* world, const AActor* context, const class UCoverPoint* coverPoint, const class ACoverPointGenerator* cpg) const;
};
