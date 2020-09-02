// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoverDataStructures.h"

#include "GameFramework/Actor.h"
#include "CoverSpotGeneratorAsync.h"
#include "NavMesh/RecastNavMesh.h"
#include "CoverPointGenerator.generated.h"

UCLASS(Blueprintable)
class COVERSPOTGENERATOR_API ACoverPointGenerator : public AActor
{
	GENERATED_BODY()

	friend class CoverSpotGeneratorAsync;

public:

#pragma region GENERATION_PROPERTIES

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _coverPointMinDistanceOnEdge = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _coverPointMinDistance = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	int _maxNumPointsPerEdge = 8;

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _coverPointOffset = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _minCrouchCoverHeight = 120.0f; // minimum height of an obstacle for it to be considered to be a crouch-cover position 

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _minStandCoverHeight = 180.0f; // minimum height of an obstacle for it to be considered to be a standing-cover position

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _maxAttackOverEdgeHeight = 140.0f; // if higher than this, the wall is too high to perform a standing attack

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _standAttackHeight = 150.0f; // physical height at which the agent shoots while standing

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _crouchAttackHeight = 100.0f; // physical height at which the agent shoots while crouched

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation|Side points")
	float _sideLeanOffset = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	float _obstacleCheckDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation|Side points")
	float _obstacleSideCheckInterval = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation|Side points")
	int _numObstacleSideChecks = 10;

	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	bool _asyncGeneration = true;
	
	UPROPERTY(EditAnywhere, Category = "Parameters|Generation")
	bool _complexCanLeanOverObstacleTest = false;

#pragma endregion GENERATION_PROPERTIES

#pragma region DEBUG_PROPERTIES
	UPROPERTY(EditAnywhere, Category = "Parameters|Debug")
	bool _drawCoverPoints = false;

	UPROPERTY(EditAnywhere, Category = "Parameters|Debug")
	bool _drawCoverPointsNormal = false;

	UPROPERTY(EditAnywhere, Category = "Parameters|Debug")
	bool _drawCoverPointsLeanDirection = false;
#pragma endregion DEBUG_PROPERTIES

protected:

	// Sets default values for this actor's properties
	ACoverPointGenerator();
	virtual void Tick(float dt) override;

	// Management
	void _Initialize(const FBox& bbox);
	void _UpdateCoverPointData(const FBox& bbox);
	void ResetCoverPointData();

	// Generation
	void GenerateSidePoints(UWorld* world, const FVector& leftEndPoint, const FVector& rightEndPoint, const FVector& edgeDir, const FVector& obstNormal, const FBox& bbox, FVector& outLeftSide, FVector& outRightSide);
	void GenerateInternalPoints(UWorld* world, const FVector& leftPoint, const FVector& rightPoint, const FVector& obstNormal, const FBox& bbox, bool hasLeftSidePoint, bool hasRightSidePoint);
	bool GetSideCoverPoint(UWorld* world, const FVector& navVert, const FVector& leanDirection, const FVector& obstNormal, const FVector& edgeDir, FVector& outSideCoverPoint) const;
	
	// Tests
	FORCEINLINE bool GetObstacleFaceNormal(UWorld* world, const FVector& edgeStart, const FVector& edgeDir, float edgeLength, FHitResult& outHit) const; // returns false if no obstacle was found
	FORCEINLINE bool AreaAlreadyHasCoverPoint(const FVector& position) const;
	FORCEINLINE bool CanStand(UWorld* world, FVector coverLocation, FVector coverFaceNormal) const;
	FORCEINLINE bool ProvidesCover(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const;
	FORCEINLINE bool CanLeanOver(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const;
	FORCEINLINE bool CanLeanOver(const UCoverPoint*) const;
	FORCEINLINE bool CanLeanSide(const UCoverPoint*) const;

	// Helper methods
	void ProjectNavPointsToGround(UWorld* world, FVector& p1, FVector& p2) const;
	void StoreNewCoverPoint(const FVector& location, const FVector& dirToCover, const FVector& leanDir, const bool& canStand);
	const void DrawDebugData() const;
	FORCEINLINE void PerformLineTrace(UWorld* world, FVector& start, FVector& end, FHitResult& outHit) const;
	FORCEINLINE bool InsideGenerationVolume(const FVector& point, const FBox& box) const;

	// Member variables
	UPROPERTY()
	TArray<UCoverPoint*> _coverPointBuffer; // workaround: store points in TArray so they are properly garbage collected
	TUniquePtr<TCoverPointOctree> _coverPoints;
	mutable bool _isInitialized;
	mutable bool _needsRedrawing;

	// nav mesh data
	FRecastDebugGeometry _navGeo;

public:
	// INTERFACE
	
	UFUNCTION(BlueprintCallable)
	void UpdateCoverpointData(const FBox& bbox);

	UFUNCTION(BlueprintCallable)
	void ClearCoverpointData();

	UFUNCTION(BlueprintCallable)
	TArray<UCoverPoint*> GetCoverPointsWithinExtent(const FVector& position, float extent) const;

	static ACoverPointGenerator* Get(UWorld* world);
	int GetNumberOfIntersectionsFromCover(const UCoverPoint* cp, const FVector& targetLocation) const;
};