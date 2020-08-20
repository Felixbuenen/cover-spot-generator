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
	float _maxAttackOverEdgeHeight = 140.0f; // if higher than this, the wall is too high to perform a standing attack

	UPROPERTY()
	float _standAttackHeight = 150.0f; // physical height at which the agent shoots while standing

	UPROPERTY()
	float _crouchAttackHeight = 100.0f; // physical height at which the agent shoots while crouched

	UPROPERTY()
	float _sideLeanOffset = 50.0f;

	UPROPERTY()
	float _obstacleCheckDistance = 100.0f;

	UPROPERTY()
	float _obstacleSideCheckInterval = 10.0f;

	UPROPERTY()
	int _numObstacleSideChecks = 10;

	UPROPERTY()
	float _maxBboxExtent = 32000.f;

protected:

	// Sets default values for this actor's properties
	ACoverPointGenerator();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Initialize();
	void UpdateCoverPointData();

	bool AreaAlreadyHasCoverPoint(const FVector& position) const;
	bool GetObstacleFaceNormal(UWorld* world, const FVector& edgeStart, const FVector& edgeDir, float edgeLength, FHitResult& outHit) const; // returns false if no obstacle was found
	bool CanStand(UWorld* world, FVector coverLocation, FVector coverFaceNormal) const;
	void ProjectNavPointsToGround(UWorld* world, FVector& p1, FVector& p2) const;
	void TestAndAddSidePoints(UWorld* world, const FVector& leftEndPoint, const FVector& rightEndPoint, const FVector& edgeDir, const FVector& obstNormal, FVector& outLeftSide, FVector& outRightSide, bool canStand);
	void TestAndAddInternalPoints(UWorld* world, const FVector& leftPoint, const FVector& rightPoint, const FVector& obstNormal, bool hasLeftSidePoint, bool hasRightSidePoint);
	bool GetSideCoverPoint(UWorld* world, const FVector& navVert, const FVector& leanDirection, const FVector& obstNormal, const FVector& edgeDir, FVector& outSideCoverPoint) const;
	bool ProvidesCover(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const;
	bool CanLeanOver(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const;

	// debug drawing
	const void DrawNavEdges() const;
	const void DrawCoverPointLocations() const;

	// nav mesh data
	FRecastDebugGeometry _navGeo;

	UPROPERTY()
	TArray<UCoverPoint*> _coverPointBuffer; // workaround: store points in TArray so they are properly garbage collected
	TUniquePtr<TCoverPointOctree> _coverPoints;

	bool ShouldTickIfViewportsOnly() const override;

private:
	UFUNCTION()
	void OnNavigationGenerationFinished(class ANavigationData* NavData);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	TArray<UCoverPoint*> GetCoverPointsWithinExtent(const FVector& position, float extent) const;

	int GetNumberOfIntersectionsFromCover(const UCoverPoint* cp, const FVector& targetLocation) const;
	bool CanAttackFromCover(const UCoverPoint* cp, const FVector& targetLocation) const;
};
