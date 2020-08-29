// Fill out your copyright notice in the Description page of Project Settings.


#include "CoverPointGenerator.h"

#include <vector>
#include <utility>

#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/LevelBounds.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"

#define EPSILON 0.00001

// Sets default values
ACoverPointGenerator::ACoverPointGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	_isInitialized = false;
}

/*
---------- Interface ------------
*/

ACoverPointGenerator* ACoverPointGenerator::Get(UWorld* world)
{
	if (!IsValid(world)) return nullptr;

	TActorIterator<ACoverPointGenerator> it(world);
	ACoverPointGenerator* cpg = *it;

	return cpg;
}


void ACoverPointGenerator::UpdateCoverpointData(const FBox& bbox)
{
	_Initialize(bbox);
}

void ACoverPointGenerator::ClearCoverpointData()
{
	ResetCoverPointData();
}

TArray<UCoverPoint*> ACoverPointGenerator::GetCoverPointsWithinExtent(const FVector& position, float extent) const
{
	if (!_isInitialized) return TArray<UCoverPoint*>();

	FBox bbox(position - FVector(extent), position + FVector(extent));
	TArray<UCoverPoint*> points;

	// iterate over the octree to find cover points within the given BBOX
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints, bbox); it.HasPendingElements(); it.Advance())
	{
		if (IsValid(it.GetCurrentElement()._coverPoint))
		{
			points.Emplace(it.GetCurrentElement()._coverPoint);
		}
	}

	return points;
}

// returns how many obstacles are in between the cover point and a given target location
int ACoverPointGenerator::GetNumberOfIntersectionsFromCover(const UCoverPoint* cp, const FVector& targetLocation) const
{
	if (!_isInitialized) return 0;

	const int infinite = 0xffff;
	const float epsilon = 0.00001f;
	const float enemyCrouchHeight = 80.0f;
	const FVector& leanDir = cp->_leanDirection;

	int numHitsSide, numHitsOver = infinite;

	UWorld* world = GetWorld();
	if (!IsValid(world)) return infinite;

	// check number of intersections if agent would lean over this cover point obstacle
	if (CanLeanOver(cp))
	{
		FVector traceStart = cp->_location;
		traceStart.Z += _standAttackHeight;

		FVector traceEnd = targetLocation;
		traceEnd.Z += enemyCrouchHeight;

		TArray<FHitResult> outHits;
		UKismetSystemLibrary::LineTraceMulti(world, traceStart, traceEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHits, true);

		numHitsOver = outHits.Num();
	}

	// check number of intersections if agent would lean aside from this cover point obstacle
	if (CanLeanSide(cp))
	{
		FVector traceStart = cp->_location;
		traceStart.Z += _crouchAttackHeight;
		traceStart.X += leanDir.X;
		traceStart.Y += leanDir.Y;

		FVector traceEnd = targetLocation;
		traceEnd.Z += enemyCrouchHeight;

		TArray<FHitResult> outHits;
		UKismetSystemLibrary::LineTraceMulti(world, traceStart, traceEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHits, true);

		numHitsSide = outHits.Num();
	}

	return FMath::Min(numHitsSide, numHitsOver);
}


/*
---------- Management ------------
*/

void ACoverPointGenerator::_Initialize(const FBox& bbox)
{
	FDateTime totalTimeBefore, totalTimeAfter;
	totalTimeBefore = FDateTime::Now();
	FDateTime timeBefore, timeAfter;
	float timeTaken;

	UWorld* world = GetWorld();
	if (world)
	{
		UNavigationSystemBase* navSystem = world->GetNavigationSystem();
		if (navSystem)
		{
			timeBefore = FDateTime::Now();
			ARecastNavMesh* navMeshData = static_cast<ARecastNavMesh*>(FNavigationSystem::GetCurrent<UNavigationSystemV1>(world)->GetDefaultNavDataInstance());
			FRecastDebugGeometry debugGeo;
			debugGeo.bGatherNavMeshEdges = true;
			navMeshData->BeginBatchQuery();
			navMeshData->GetDebugGeometry(debugGeo);
			navMeshData->FinishBatchQuery();
			_navGeo = debugGeo;

			timeAfter = FDateTime::Now();

			timeTaken = (timeAfter - timeBefore).GetTotalSeconds();
			UE_LOG(LogTemp, Log, TEXT("nav mesh query time: %f"), timeTaken);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No NavSystem found!"));
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("World couldn't be loaded!"));
		return;
	}

	timeBefore = FDateTime::Now();
	// init cover point data
	_UpdateCoverPointData(bbox);
	timeAfter = FDateTime::Now();

	timeTaken = (timeAfter - timeBefore).GetTotalSeconds();
	UE_LOG(LogTemp, Log, TEXT("Coverpoint generation time: %f"), timeTaken);

	// apply octree optimization
	_coverPoints->ShrinkElements();

	timeBefore = FDateTime::Now();
	DrawDebugData();
	timeAfter = FDateTime::Now();
	timeTaken = (timeAfter - timeBefore).GetTotalSeconds();
	UE_LOG(LogTemp, Log, TEXT("draw debug data: %f"), timeTaken);

	totalTimeAfter = FDateTime::Now();
	float totalTimeTaken = (totalTimeAfter - totalTimeBefore).GetTotalSeconds();
	UE_LOG(LogTemp, Log, TEXT("total time taken: %f"), totalTimeTaken);
}

void ACoverPointGenerator::_UpdateCoverPointData(const FBox& bbox)
{
	ResetCoverPointData();

	// re-init octree
	_coverPoints = MakeUnique<TCoverPointOctree>(bbox.GetCenter(), bbox.GetExtent().GetMax());

	UWorld* world = GetWorld();
	// loop over all nav mesh edges
	int numEdges = _navGeo.NavMeshEdges.Num();
	UE_LOG(LogTemp, Log, TEXT("Number of navmesh edges: %d"), numEdges);
	for (int i = 0; i < numEdges; i += 2)
	{
		FVector v1 = _navGeo.NavMeshEdges[i];
		FVector v2 = _navGeo.NavMeshEdges[i + 1];
		ProjectNavPointsToGround(world, v1, v2);

		if (!FMath::LineBoxIntersection(bbox, v1, v2, (v2 - v1))) continue;

		// calculate edge
		FVector edgeDir = (v2 - v1);
		float edgeLength = edgeDir.Size();	
		edgeDir /= edgeLength;

		// get the normal of the face of the obstacle that this edge is parallel to
		FHitResult obstacleCheckHit;
		if (!GetObstacleFaceNormal(world, v1, edgeDir, edgeLength, obstacleCheckHit)) continue;

		FVector outLeftSide, outRightSide;
		GenerateSidePoints(world, v2, v1, edgeDir, obstacleCheckHit.ImpactNormal, bbox, outLeftSide, outRightSide);

		// if not "complex can lean over obstacle test", check if agent can lean over obstacle once at middle of edge (may be too coarse)
		if (!_complexCanLeanOverObstacleTest)
		{
			bool isStandingCover = CanStand(world, v1 + edgeDir * edgeLength * 0.5f, obstacleCheckHit.Normal);
			if (isStandingCover) continue;
		}

		bool hasLeftSidePoint = !outLeftSide.Equals(v2);
		bool hasRightSidePoint = !outRightSide.Equals(v1);
		GenerateInternalPoints(world, outLeftSide, outRightSide, obstacleCheckHit.Normal, bbox, hasLeftSidePoint, hasRightSidePoint);
	}

	int numCoverPoints = _coverPointBuffer.Num();
	UE_LOG(LogTemp, Log, TEXT("Num cover points: %d"), numCoverPoints);

	_isInitialized = true;
}

void ACoverPointGenerator::ResetCoverPointData()
{
	_isInitialized = false;

	_coverPointBuffer.Empty();
	if(_coverPoints)
		_coverPoints->Destroy();
}


/*
---------- Generation ------------
*/

void ACoverPointGenerator::GenerateInternalPoints(UWorld* world, const FVector& leftPoint, const FVector& rightPoint, const FVector& obstNormal, const FBox& bbox, bool hasLeftSidePoint, bool hasRightSidePoint)
{
	// get the edge between the two side cover points
	FVector internalEdge = (leftPoint - rightPoint);
	float internalEdgeLength = internalEdge.Size();
	internalEdge /= internalEdgeLength;

	int numInternalPoints = (int)(internalEdgeLength / _coverPointMinDistance) + 1;
	if (numInternalPoints <= 1) return;

	// optionally clamp to max. number of cover points for nav mesh edge
	if (_maxNumPointsPerEdge > -1) numInternalPoints = FMath::Clamp<int>(numInternalPoints, 0, _maxNumPointsPerEdge);
	float coverPointInterval = internalEdgeLength / (float)(numInternalPoints - 1);

	FVector startLoc = rightPoint;

	// check if we should place a cover spot on right end point of nav edge
	if (hasRightSidePoint || AreaAlreadyHasCoverPoint(startLoc))
	{
		// move the starting position further along the internal edge and decrease the total number of internal points
		startLoc += internalEdge * coverPointInterval;
		numInternalPoints--;
	}
	// check if we should place a cover spot on left end point of nav edge
	if (hasLeftSidePoint || AreaAlreadyHasCoverPoint(startLoc + internalEdge * coverPointInterval * (numInternalPoints - 1)))
	{
		numInternalPoints--;
	}
	
	// generate internal points
	for (int idx = 0; idx < numInternalPoints; idx++)
	{
		FVector pointLocation = startLoc + idx * internalEdge * coverPointInterval;
		if (!InsideGenerationVolume(pointLocation, bbox)) continue;

		// an internal point can only be useful if one can lean over it (internal points are not at the sides of obstacles)
		if (ProvidesCover(world, pointLocation, obstNormal) && CanLeanOver(world, pointLocation, obstNormal))
		{
			bool canStand = false; // if agent can lean over, standing isn't safe
			FVector dirToCover = -obstNormal;
			FVector leanDir = FVector::UpVector;

			StoreNewCoverPoint(pointLocation, dirToCover, leanDir, canStand);
		}
	}
}

void ACoverPointGenerator::GenerateSidePoints(UWorld* world, const FVector& leftVertex, const FVector& rightVertex, const FVector& edgeDir, const FVector& obstNormal, const FBox& bbox, FVector& outLeftSide, FVector& outRightSide)
{
	outLeftSide = leftVertex;
	outRightSide = rightVertex;

	FVector rightToNormal = FVector::CrossProduct(FVector::UpVector, obstNormal);
	auto storeSidePoint = [&](const FVector& location, const FVector& dirToCover, FVector leanDirection, FVector& outSidePoint)
	{
		if (!AreaAlreadyHasCoverPoint(location))
		{
			bool canStand = CanStand(world, location, -dirToCover);
			if (!canStand)
			{
				leanDirection.Z = CanLeanOver(world, location, obstNormal) ? 1.0f : 0.0f;
			}

			StoreNewCoverPoint(location, dirToCover, leanDirection, canStand);
			outSidePoint = location;
		}
	};
	
	// check if there is a cover point at the left side and store it
	FVector leftSideCoverPoint;
	bool hasLeftSidePoint = GetSideCoverPoint(world, leftVertex, rightToNormal, obstNormal, edgeDir, leftSideCoverPoint);
	if (hasLeftSidePoint && InsideGenerationVolume(leftSideCoverPoint, bbox))
		storeSidePoint(leftSideCoverPoint, -obstNormal, rightToNormal, outLeftSide);

	// check if there is a cover point at the right side and store it
	FVector rightSideCoverPoint;
	bool hasRightSidePoint = GetSideCoverPoint(world, rightVertex, -rightToNormal, obstNormal, -edgeDir, rightSideCoverPoint);
	if (hasRightSidePoint && InsideGenerationVolume(rightSideCoverPoint, bbox))
		storeSidePoint(rightSideCoverPoint, -obstNormal, -rightToNormal, outRightSide);
}

// The navigation mesh is not perfect. Therefore, we cannot assume that nav-mesh edges perfectly represent obstacle information. This method performs a sweep test 
//  along the obstacle to find the side of an obstacle.
bool ACoverPointGenerator::GetSideCoverPoint(UWorld* world, const FVector& navVert, const FVector& leanDirection, const FVector& obstNormal, const FVector& edgeDir, FVector& outSideCoverPoint) const
{
	const float vertOffset = 20.0f;

	// decide whether we should search along the left or right to find a cover spot
	FVector startPoint = navVert;
	startPoint.Z += vertOffset;
	FVector projectEnd = startPoint + -obstNormal * _obstacleCheckDistance;
	
	FHitResult projectHit;
	PerformLineTrace(world, startPoint, projectEnd, projectHit);

	// Lambda that performs the sweep test to find the edge of an obstacle (and thus side cover point).
	// If sweeping in lean direction: currently not in front of obstacle, stop with the first line trace that intersects the obstacle.
	// If sweeping in opposite lean direction: currently in front of obstacle, stop wtih the first line trace that does not intersect the obstacle, store last intersection.
	bool foundEndPoint = false;
	auto findEdgePoint = [&](bool sweepInLeanDir) -> FVector
	{
		FVector _sidePoint;
		FHitResult sideHitCheck;

		FVector edgeDirection = sweepInLeanDir ? edgeDir : -edgeDir;
		float lastDistance = projectHit.Distance;

		for (int i = 0; i < _numObstacleSideChecks; i++)
		{
			FVector start = startPoint + (i + 1) * edgeDirection * _obstacleSideCheckInterval;
			FVector stop = start + -obstNormal * _obstacleCheckDistance;

			PerformLineTrace(world, start, stop, sideHitCheck);
			bool foundEdge = sweepInLeanDir != sideHitCheck.bBlockingHit;
			if (foundEdge)
			{
				// edge found: offset the resulting point from the obstacle such that there is a clearance of _agentRadius
				float distToObstacle = sweepInLeanDir ? lastDistance : sideHitCheck.Distance;
				float offsetFromObstacle = distToObstacle - _agentRadius;
				float leanOffset = sweepInLeanDir ? _agentRadius + _obstacleSideCheckInterval : _agentRadius;

				_sidePoint = start - leanDirection * leanOffset + -obstNormal * offsetFromObstacle;
				foundEndPoint = true;
				break;
			}

			lastDistance = sideHitCheck.Distance;
		}

		return _sidePoint;
	};

	// Find side cover point: sweep direction depends on whether we're already in front of the obstacle or not.
	bool sweepInEdgeDirection = projectHit.bBlockingHit;
	FVector sidePoint = findEdgePoint(sweepInEdgeDirection);

	// make sure resulting side cover point is valid (may not be true due to nav-mesh imperfections)
	if (foundEndPoint)
	{
		FVector visionFromSideCheckStart = sidePoint + leanDirection * (_sideLeanOffset + _agentRadius);
		FVector visionFromSideCheckStop = visionFromSideCheckStart + -obstNormal * _obstacleCheckDistance;

		FHitResult visionFromSideCheck;
		PerformLineTrace(world, visionFromSideCheckStart, visionFromSideCheckStop, visionFromSideCheck);

		if (!visionFromSideCheck.bBlockingHit)
		{
			// vision not blocked from side: valid side cover point
			sidePoint.Z -= vertOffset;
			outSideCoverPoint = sidePoint;

			return true;
		}
	}

	return false;
}


/*
---------- Tests ------------
*/

bool ACoverPointGenerator::ProvidesCover(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const
{
	FVector checkStart = coverLocation;
	checkStart.Z += _minCrouchCoverHeight;
	FVector checkStop = checkStart + -coverFaceNormal * _obstacleCheckDistance;
	
	FHitResult outHit;
	PerformLineTrace(world, checkStart, checkStop, outHit);

	return outHit.bBlockingHit;
}

bool ACoverPointGenerator::CanStand(UWorld* world, FVector coverLocation, FVector coverFaceNormal) const
{
	FVector checkStart = coverLocation;
	checkStart.Z += _minStandCoverHeight;
	FVector checkStop = checkStart + -coverFaceNormal * _obstacleCheckDistance;

	FHitResult outHit;
	PerformLineTrace(world, checkStart, checkStop, outHit);

	return outHit.bBlockingHit;
}

bool ACoverPointGenerator::CanLeanOver(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const
{
	FVector checkStart = coverLocation;
	checkStart.Z += _maxAttackOverEdgeHeight;
	FVector checkStop = checkStart + -coverFaceNormal * _obstacleCheckDistance;

	FHitResult outHit;
	PerformLineTrace(world, checkStart, checkStop, outHit);

	return !outHit.bBlockingHit;
}

bool ACoverPointGenerator::CanLeanOver(const UCoverPoint* cp) const
{
	const float epsilon = 0.0001;
	return (cp->_leanDirection.Z > epsilon);
}

bool ACoverPointGenerator::CanLeanSide(const UCoverPoint* cp) const
{
	const float epsilon = 0.0001f;
	FVector2D leanDir2D = FVector2D(cp->_leanDirection);
	return (leanDir2D.SizeSquared() > epsilon);
}

bool ACoverPointGenerator::AreaAlreadyHasCoverPoint(const FVector& position) const
{
	FBox bbox(position, position);

	// example code how to query octree
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints, bbox); it.HasPendingElements(); it.Advance())
	{
		if ((it.GetCurrentElement()._coverPoint->_location - position).Size() < 10.0f)
		{
			return true;
		}
	}

	return false;
}

bool ACoverPointGenerator::GetObstacleFaceNormal(UWorld* world, const FVector& edgeStart, const FVector& edgeDir, float edgeLength, FHitResult& outHit) const
{
	// get normal of obstacle face this edge is parallel to
	FVector rightVec = FVector::CrossProduct(FVector::UpVector, edgeDir);
	FVector checkStart = edgeStart + edgeDir * edgeLength * 0.5f;
	checkStart.Z += 20.0f;
	FVector checkStop = checkStart + rightVec * _obstacleCheckDistance;

	PerformLineTrace(world, checkStart, checkStop, outHit);

	return outHit.bBlockingHit;
}


/*
---------- Helper methods ------------
*/

void ACoverPointGenerator::ProjectNavPointsToGround(UWorld* world, FVector& p1, FVector& p2) const
{
	const float maxProjectionHeight = 500.0f;
	FHitResult projectHit;
	
	// first vertex
	FVector projectEnd = p1 + FVector::DownVector * maxProjectionHeight;
	PerformLineTrace(world, p1, projectEnd, projectHit);
	if (projectHit.bBlockingHit) p1 = projectHit.Location;
	
	// second vertex
	projectEnd = p2 + FVector::DownVector * maxProjectionHeight;
	PerformLineTrace(world, p2, projectEnd, projectHit);
	if (projectHit.bBlockingHit) p2 = projectHit.Location;
}

const void ACoverPointGenerator::DrawDebugData() const
{
	if (!_coverPoints.IsValid()) return;

	UWorld* world = GetWorld();
	if (!world)
	{
		UE_LOG(LogTemp, Warning, TEXT("No world found..."));
		return;
	}

	FlushPersistentDebugLines(world);

	const float debugSphereExtent = 30.0f;
	const float debugLineLength = 80.0f;
	const float debugLineVertOffset = 10.0f;
	// loop through octree
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints, _coverPoints->GetRootBounds()); it.HasPendingElements(); it.Advance())
	{
		FCoverPointOctreeElement el = it.GetCurrentElement();
		UCoverPoint* cp = el._coverPoint;
		
		if (cp != nullptr)
		{
			if (_drawCoverPoints)
			{
				DrawDebugSphere(world, cp->_location, debugSphereExtent, 7, FColor::Cyan, true);
			}

			if (_drawCoverPointsNormal)
			{
				FVector startPoint = cp->_location + FVector::UpVector * debugLineVertOffset;
				FVector stopPoint = startPoint + cp->_dirToCover * debugLineLength * -1.0f;
				DrawDebugLine(world, startPoint, stopPoint, FColor::Magenta, true);
			}

			if (_drawCoverPointsLeanDirection)
			{
				FVector startPoint = cp->_location + FVector::UpVector * debugLineVertOffset;
				FVector stopPoint = startPoint + cp->_leanDirection * debugLineLength;
				DrawDebugLine(world, startPoint, stopPoint, FColor::Green, true);
			}
		}
	}
}

void ACoverPointGenerator::StoreNewCoverPoint(const FVector& location, const FVector& dirToCover, const FVector& leanDir, const bool& canStand)
{
	UCoverPoint* cp = NewObject<UCoverPoint>();
	cp->Init(location, dirToCover, leanDir, canStand);

	_coverPoints->AddElement(FCoverPointOctreeElement(cp, _coverPointMinDistance));
	_coverPointBuffer.Emplace(cp);
}

void ACoverPointGenerator::PerformLineTrace(UWorld* world, FVector& start, FVector& end, FHitResult& outHit) const
{
	UKismetSystemLibrary::LineTraceSingle(world, start, end, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHit, true);
}

bool ACoverPointGenerator::InsideGenerationVolume(const FVector& point, const FBox& box) const
{
	return FMath::PointBoxIntersection(point, box);
}
