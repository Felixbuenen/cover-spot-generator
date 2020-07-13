// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericOctree.h"

struct FCoverPoint
{
	FVector _location;
	FVector _dirToCover;

	FCoverPoint(FVector location, FVector dirToCover) :
		_location(location), _dirToCover(dirToCover) {}
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