// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

/**
 * 
 */
class COVERSPOTGENERATOR_API CoverSpotGeneratorAsync : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<CoverSpotGeneratorAsync>;

private:
	class ACoverPointGenerator* _cpg;
	FBox _generationBBox;

public:
	CoverSpotGeneratorAsync(class ACoverPointGenerator* cpg, FBox genBBox) : _cpg(cpg), _generationBBox(genBBox) { }
	~CoverSpotGeneratorAsync() {}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(CoverSpotGeneratorAsync, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();

};
