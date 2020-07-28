// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationSystem.h"
#include "GenericOctree.h"
#include "CoverDataStructures.h"
#include "CoverPointGenerator.generated.h"

UCLASS(Blueprintable)
class COVERSPOTGENERATOR_API ACoverPointGenerator : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACoverPointGenerator();

	UPROPERTY()
	float _coverPointMinDistance = 150.0f;

	UPROPERTY()
	int _maxNumPointsPerEdge = 8;

	UPROPERTY()
	float _agentRadius = 30.0f;

	UPROPERTY()
	float _minCrouchCoverHeight = 100.0f; // minimum height of an obstacle for it to be considered to be a crouch-cover position 

	UPROPERTY()
	float _minStandCoverHeight = 180.0f; // minimum height of an obstacle for it to be considered to be a standing-cover position

	UPROPERTY()
	float _maxAttackOverEdgeHeight = 120.0f; // if higher than this, an agent cannot lean over the edge of the obstacle and needs to lean out to the left or right of the obstacle

	UPROPERTY()
	float _sideLeanOffset = 50.0f;

	UPROPERTY()
	float _obstacleCheckDistance = 100.0f;

	UPROPERTY()
	float _obstacleSideCheckInterval = 10.0f;

	UPROPERTY()
	int _numObstacleSideChecks = 10;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void UpdateCoverPointData();

	bool AreaAlreadyHasCoverPoint(const FVector& position) const;
	bool GetObstacleFaceNormal(UWorld* world, const FVector& edgeStart, const FVector& edgeDir, float edgeLength, FHitResult& outHit) const; // returns false if no obstacle was found
	bool CanStand(UWorld* world, FVector coverLocation, FVector coverFaceNormal) const;
	void ProjectNavPointsToGround(UWorld* world, FVector& p1, FVector& p2) const;
	void TestAndAddSidePoints(UWorld* world, const FVector& leftEndPoint, const FVector& rightEndPoint, const FVector& edgeDir, const FVector& obstNormal, FVector& outLeftSide, FVector& outRightSide, bool canStand) const;
	void TestAndAddInternalPoints(UWorld* world, const FVector& leftPoint, const FVector& rightPoint, const FVector& obstNormal, bool hasLeftSidePoint, bool hasRightSidePoint) const;
	bool GetSideCoverPoint(UWorld* world, const FVector& navVert, const FVector& leanDirection, const FVector& obstNormal, const FVector& edgeDir, FVector& outSideCoverPoint) const;
	bool ProvidesCover(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const;
	bool CanLeanOver(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const;

	// debug drawing
	const void DrawNavEdges() const;
	const void DrawCoverPointLocations() const;

	// nav mesh data
	FRecastDebugGeometry _navGeo;
	TUniquePtr<TCoverPointOctree> _coverPoints;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// DEBUG
	UFUNCTION(BlueprintCallable)
	TArray<UCoverPoint*> GetCoverPointsWithinExtent(const FVector& position, float extent) const;
};
