// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GenericOctree.h"

#include "CoreMinimal.h"
#include "CoverDataStructures.generated.h"

USTRUCT(BlueprintType)
struct FCoverPoint
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cover Point")
	FVector _location;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cover Point")
	FVector _dirToCover;

	// lean direction (vec2; up-left-right)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cover Point")
	FVector2D _leanDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cover Point")
	bool _canStand;

	FCoverPoint(FVector location, FVector dirToCover, FVector2D leanDir, bool canStand) :
		_location(location), _dirToCover(dirToCover), _leanDirection(leanDir), _canStand(canStand) {}

	FCoverPoint() {}
};

struct FCoverPointOctreeElement
{
	FCoverPointOctreeElement(TSharedPtr<FCoverPoint> coverPoint, float extent) : 
		_coverPoint(coverPoint), _bbox(coverPoint->_location, FVector(extent)) {}
	
	TSharedPtr<FCoverPoint> _coverPoint;
	FBoxCenterAndExtent _bbox;
};

struct FCoverPointOctreeSemantics
{
	enum { MaxElementsPerLeaf = 4 };
	enum { MinInclusiveElementsPerNode = 1 };
	enum { MaxNodeDepth = 16 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static FBoxCenterAndExtent GetBoundingBox(const FCoverPointOctreeElement& Element)
	{
		return Element._bbox;
	}

	FORCEINLINE static bool AreElementsEqual(const FCoverPointOctreeElement& a, const FCoverPointOctreeElement& b)
	{
		return a._coverPoint == b._coverPoint;
	}

	/** Ignored for this implementation */
	FORCEINLINE static void SetElementId(const FCoverPointOctreeElement& Element, FOctreeElementId Id)
	{
	}
};

typedef TOctree<FCoverPointOctreeElement, FCoverPointOctreeSemantics> TCoverPointOctree;