// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_CoverSpot_LooksAt.generated.h"

class UCoverPoint;

UCLASS()
class COVERSPOTGENERATOR_API UEnvQueryTest_CoverSpot_LooksAt : public UEnvQueryTest
{
	GENERATED_UCLASS_BODY()
	
	/** context */
	UPROPERTY(EditDefaultsOnly, Category = Cover)
	TSubclassOf<UEnvQueryContext> LookAtContext;

	UPROPERTY(EditDefaultsOnly, Category = Cover)
	FAIDataProviderFloatValue MinScoreViewingAngle;

	UPROPERTY(EditDefaultsOnly, Category = Cover)
	FAIDataProviderFloatValue MaxScoreViewingAngle;

	//UPROPERTY(EditDefaultsOnly, Category = Cover)
	//FAIDataProviderFloatValue MinViewingAngle;
	//
	//UPROPERTY(EditDefaultsOnly, Category = Cover)
	//FAIDataProviderFloatValue MaxViewingAngle;

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

protected:
	float ScoreViewingAngle(const UCoverPoint* cp, const FVector& targetLocation) const;
};
