// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationSystem.h"
#include "GenericOctree.h"
#include "CoverDataStructures.h"
#include "CoverPointGenerator.generated.h"

UCLASS()
class COVERSPOTGENERATOR_API ACoverPointGenerator : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACoverPointGenerator();

	UPROPERTY()
	float _coverPointMinDistance = 35.0f;

	UPROPERTY()
	int _maxNumPointsPerEdge = -1;

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
	bool IsStandingCover(UWorld* world, FVector coverLocation, FVector coverFaceNormal) const;
	void ProjectNavPointsToGround(UWorld* world, FVector& p1, FVector& p2) const;
	void TestAndAddSidePoints(UWorld* world, const FVector& leftEndPoint, const FVector& rightEndPoint, const FVector& edgeDir, const FVector& obstNormal) const;
	bool GetSideCoverPoint(UWorld* world, const FVector& navVert, const FVector& leanDirection, const FVector& obstNormal, const FVector& edgeDir, FVector& outSideCoverPoint) const;

	// debug drawing
	const void DrawNavEdges() const;
	const void DrawCoverPointLocations() const;

	// nav mesh data
	FRecastDebugGeometry _navGeo;
	TUniquePtr<TCoverPointOctree> _coverPoints;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
