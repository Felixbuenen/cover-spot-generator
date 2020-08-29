// Fill out your copyright notice in the Description page of Project Settings.

#include "CoverSpotGeneratorAsync.h"

#include "CoverPointGenerator.h"

void CoverSpotGeneratorAsync::DoWork()
{
	_cpg->_Initialize(_generationBBox);
}