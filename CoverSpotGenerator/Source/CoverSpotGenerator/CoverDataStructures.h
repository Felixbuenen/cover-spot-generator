// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GenericOctree.h"

#include "CoreMinimal.h"
#include "CoverDataStructures.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UCoverPoint : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cover Point")
		FVector _location;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cover Point")
		FVector _dirToCover;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cover Point")
		FVector _leanDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cover Point")
		bool _canStand;

	UCoverPoint() = default;
	~UCoverPoint() 
	{
		//UE_LOG(LogTemp, Warning, TEXT("Deleted"));
	}

	FORCEINLINE void Init(FVector location, FVector dirToCover, FVector leanDir, bool canStand)
	{
		_location = location;
		_dirToCover = dirToCover;
		_leanDirection = leanDir;
		_canStand = canStand;
	}
};

struct FCoverPointOctreeElement
{
	FCoverPointOctreeElement(UCoverPoint* coverPoint, float extent)
	{
		_coverPoint = coverPoint;
		_bbox.Center = _coverPoint->_location;
		_bbox.Extent = FVector(extent);
	}

	~FCoverPointOctreeElement()
	{
		//UE_LOG(LogTemp, Warning, TEXT("TREE ELEMENT GOT DELETED"));
	}
	
	UCoverPoint* _coverPoint;

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