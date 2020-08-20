// Fill out your copyright notice in the Description page of Project Settings.


#include "CoverPointGenerator.h"

#include <vector>
#include <utility>

#include "DrawDebugHelpers.h"
#include "Engine/LevelBounds.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Editor/EditorEngine.h"
#include "Math/NumericLimits.h"

bool ACoverPointGenerator::ShouldTickIfViewportsOnly() const
{
	return false;
}

// Sets default values
ACoverPointGenerator::ACoverPointGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// init octree
	_coverPoints = MakeUnique<TCoverPointOctree>(FVector::ZeroVector, _maxBboxExtent);
}

void ACoverPointGenerator::Initialize()
{
	UWorld* world = GetWorld();
	if (world)
	{
		UNavigationSystemBase* navSystem = world->GetNavigationSystem();
		if (navSystem)
		{
			ARecastNavMesh* navMeshData = static_cast<ARecastNavMesh*>(FNavigationSystem::GetCurrent<UNavigationSystemV1>(world)->GetDefaultNavDataInstance());
			_navGeo.bGatherNavMeshEdges = true;
			navMeshData->BeginBatchQuery();
			navMeshData->GetDebugGeometry(_navGeo);
			navMeshData->FinishBatchQuery();
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

	// init cover point data
	UpdateCoverPointData();

	// apply octree optimization
	_coverPoints->ShrinkElements();

	// draw debug data
	//DrawNavEdges();
	DrawCoverPointLocations();

	// bind nav mesh creation event
	if (UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(world))
		NavSystem->OnNavigationGenerationFinishedDelegate.AddDynamic(this, &ACoverPointGenerator::OnNavigationGenerationFinished);
}


// Called when the game starts or when spawned
void ACoverPointGenerator::BeginPlay()
{
	Super::BeginPlay();

	Initialize();
}


// Called every frame
void ACoverPointGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// stores candidate cover-locations in the cover point octree
void ACoverPointGenerator::UpdateCoverPointData()
{
	// TODO: reset octree

	UWorld* world = GetWorld();
	
	// loop over all nav mesh edges
	int numEdges = _navGeo.NavMeshEdges.Num();
	for (int i = 0; i < numEdges; i += 2)
	{
		FVector v1 = _navGeo.NavMeshEdges[i];
		FVector v2 = _navGeo.NavMeshEdges[i + 1];
		ProjectNavPointsToGround(world, v1, v2);

		// calculate edge
		FVector edgeDir = (v2 - v1);
		float edgeLength = edgeDir.Size();	
		edgeDir /= edgeLength;

		// get the normal of the face of the obstacle that this edge is parallel to
		FHitResult obstacleCheckHit;
		if (!GetObstacleFaceNormal(world, v1, edgeDir, edgeLength, obstacleCheckHit)) continue;
		
		// check if can stand over here or for each internal point?
		bool isStandingCover = CanStand(world, v1 + edgeDir * edgeLength * 0.5f, obstacleCheckHit.Normal);

		FVector outLeftSide, outRightSide;
		TestAndAddSidePoints(world, v2, v1, edgeDir, obstacleCheckHit.ImpactNormal, outLeftSide, outRightSide, isStandingCover);

		if (isStandingCover) continue;

		// check if can lean over here or for each internal point?

		bool hasLeftSidePoint = !outLeftSide.Equals(v2);
		bool hasRightSidePoint = !outRightSide.Equals(v1);

		TestAndAddInternalPoints(world, outLeftSide, outRightSide, obstacleCheckHit.Normal, hasLeftSidePoint, hasRightSidePoint);
	}

}

void ACoverPointGenerator::TestAndAddInternalPoints(UWorld* world, const FVector& leftPoint, const FVector& rightPoint, const FVector& obstNormal, bool hasLeftSidePoint, bool hasRightSidePoint)
{
	// edge between the two side cover points
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
		startLoc += internalEdge * coverPointInterval;
		numInternalPoints--;
	}
	// check if we should place a cover spot on left end point of nav edge
	if (hasLeftSidePoint || AreaAlreadyHasCoverPoint(startLoc + internalEdge * coverPointInterval * (numInternalPoints - 1)))
	{
		numInternalPoints--;
	}
	
	// check and generate internal points
	for (int idx = 0; idx < numInternalPoints; idx++)
	{
		FVector pointLocation = startLoc + idx * internalEdge * coverPointInterval;

		if (ProvidesCover(world, pointLocation, obstNormal) && CanLeanOver(world, pointLocation, obstNormal))
		{
			bool canStand = false; // if agent can lean over, standing isn't safe
			UCoverPoint* cp = NewObject<UCoverPoint>();
			cp->Init(pointLocation, -obstNormal, FVector(0, 0, 1), canStand);
			_coverPoints->AddElement(FCoverPointOctreeElement(cp, _coverPointMinDistance));
			_coverPointBuffer.Emplace(cp);

			//DrawDebugLine(world, FVector(pointLocation), FVector(pointLocation + obstNormal * 150.0f), FColor::Magenta, true);
		}
	}
}

bool ACoverPointGenerator::ProvidesCover(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const
{
	FVector checkStart = coverLocation;
	checkStart.Z += _minCrouchCoverHeight;
	FVector checkStop = checkStart + -coverFaceNormal * _obstacleCheckDistance;
	FHitResult outHit;

	UKismetSystemLibrary::LineTraceSingle(world, checkStart, checkStop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHit, true);

	return outHit.bBlockingHit;
}

bool ACoverPointGenerator::CanLeanOver(UWorld* world, const FVector& coverLocation, const FVector& coverFaceNormal) const
{
	FVector checkStart = coverLocation;
	checkStart.Z += _maxAttackOverEdgeHeight;
	FVector checkStop = checkStart + -coverFaceNormal * _obstacleCheckDistance;
	FHitResult outHit;

	UKismetSystemLibrary::LineTraceSingle(world, checkStart, checkStop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHit, true);

	return !outHit.bBlockingHit;
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

	UKismetSystemLibrary::LineTraceSingle(world, checkStart, checkStop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHit, true);

	//DrawDebugLine(world, outHit.Location, outHit.Location + outHit.Normal * 50.0f, FColor::Yellow, true);

	return outHit.bBlockingHit;
}

bool ACoverPointGenerator::CanStand(UWorld* world, FVector coverLocation, FVector coverFaceNormal) const
{
	FVector checkStart = coverLocation;
	checkStart.Z += _minStandCoverHeight;
	FVector checkStop = checkStart + -coverFaceNormal * _obstacleCheckDistance;

	FHitResult outHit;
	UKismetSystemLibrary::LineTraceSingle(world, checkStart, checkStop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHit, true);

	return outHit.bBlockingHit;
}

void ACoverPointGenerator::ProjectNavPointsToGround(UWorld* world, FVector& p1, FVector& p2) const
{
	// project points to ground
	FHitResult projectHit;

	// first vertex
	FVector projectEnd = p1 + FVector::DownVector * 200.0f;
	UKismetSystemLibrary::LineTraceSingle(world, p1, projectEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, projectHit, true);
	if (projectHit.bBlockingHit) p1 = projectHit.Location;

	// second vertex
	projectEnd = p2 + FVector::DownVector * 200.0f;
	UKismetSystemLibrary::LineTraceSingle(world, p2, projectEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, projectHit, true);
	if (projectHit.bBlockingHit) p2 = projectHit.Location;
}

void ACoverPointGenerator::TestAndAddSidePoints(UWorld* world, const FVector& leftVertex, const FVector& rightVertex, const FVector& edgeDir, const FVector& obstNormal, FVector& outLeftSide, FVector& outRightSide, bool canStand)
{
	// TODO: 
	// (1) extend so that crouch- and stand height are taken into account
	// (2) REFACTOR
	// (3) more consistent variable naming

	outLeftSide = leftVertex;
	outRightSide = rightVertex;

	const float vertOffset = 20.0f;
	const float agentRadius = 30.0f; // actually half-radius
	const float distToObstacle = agentRadius;
	FVector rightVec = FVector::CrossProduct(FVector::UpVector, edgeDir);
	FVector rightToNormal = FVector::CrossProduct(FVector::UpVector, obstNormal);

	FHitResult rightCheck, leftCheck;
	FVector initRightStart = rightVertex;
	initRightStart.Z += 20.0f;
	FVector initRightStop = initRightStart + rightVec * _obstacleCheckDistance;
	UKismetSystemLibrary::LineTraceSingle(world, initRightStart, initRightStop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, rightCheck, false);
	FVector initLeftStart = rightVertex;
	initLeftStart.Z += 20.0f;
	FVector initLeftStop = initLeftStart + rightVec * _obstacleCheckDistance;
	UKismetSystemLibrary::LineTraceSingle(world, initLeftStart, initLeftStop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, leftCheck, false);

	if (leftCheck.bBlockingHit && rightCheck.bBlockingHit && FVector::DotProduct(leftCheck.ImpactNormal, rightCheck.ImpactNormal) < 0.8f)
	{
		return;
	}

	// check if there is side-cover to the endpoints of this nav edge. If so, generate cover points at these locations.
	FVector leftSideCoverPoint, rightSideCoverPoint;

	if (GetSideCoverPoint(world, leftVertex, rightToNormal, obstNormal, edgeDir, leftSideCoverPoint))
	{
		if (!AreaAlreadyHasCoverPoint(leftSideCoverPoint))
		{
			FVector leanDirection = rightToNormal;
			if (!canStand)
			{
				leanDirection.Z = CanLeanOver(world, leftSideCoverPoint, obstNormal) ? 1.0f : 0.0f;
			}

			UCoverPoint* cp = NewObject<UCoverPoint>(UCoverPoint::StaticClass());
			cp->Init(leftSideCoverPoint, -obstNormal, leanDirection, canStand);
			_coverPoints->AddElement(FCoverPointOctreeElement(cp, _coverPointMinDistance));
			_coverPointBuffer.Emplace(cp);

			//DrawDebugLine(world, FVector(leftSideCoverPoint), FVector(leftSideCoverPoint + obstNormal * 150.0f), FColor::Magenta, true);

			outLeftSide = leftSideCoverPoint;		
		}
	}
	if (GetSideCoverPoint(world, rightVertex, -rightToNormal, obstNormal, -edgeDir, rightSideCoverPoint))
	{
		if (!AreaAlreadyHasCoverPoint(rightSideCoverPoint))
		{
			FVector leanDirection = -rightToNormal;
			if (!canStand)
			{
				leanDirection.Z = CanLeanOver(world, rightSideCoverPoint, obstNormal) ? 1.0f : 0.0f;
			}
			UCoverPoint* cp = NewObject<UCoverPoint>(UCoverPoint::StaticClass());
			cp->Init(rightSideCoverPoint, -obstNormal, leanDirection, canStand);
			_coverPoints->AddElement(FCoverPointOctreeElement(cp, _coverPointMinDistance));
			_coverPointBuffer.Emplace(cp);

			//DrawDebugLine(world, FVector(rightSideCoverPoint), FVector(rightSideCoverPoint + obstNormal * 150.0f), FColor::Magenta, true);

			outRightSide = rightSideCoverPoint;
		}
	}
}

bool ACoverPointGenerator::GetSideCoverPoint(UWorld* world, const FVector& navVert, const FVector& leanDirection, const FVector& obstNormal, const FVector& edgeDir, FVector& outSideCoverPoint) const
{
	const float vertOffset = 20.0f;
	const float distToObstacle = _agentRadius;

	FHitResult projectHit;
	FVector startPoint = navVert;
	startPoint.Z += vertOffset;
	FVector projectEnd = startPoint + -obstNormal * _obstacleCheckDistance;

	// shoot ray from start point (nav edge vertex) towards obstacle to decide whether we should search to the left or right to find a cover spot
	UKismetSystemLibrary::LineTraceSingle(world, startPoint, projectEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, projectHit, false);

	FVector sidePoint;
	bool foundEndPoint = false;

	// if vertex perpendicular to obstacle: sweep ray in direction of lean-dir until it doesn't intersect obstacle to find its edge
	if (projectHit.bBlockingHit) // TODO: check obstacle normal instead?
	{
		FHitResult sideHitCheck;
		float lastDistance = projectHit.Distance;
		for (int i = 0; i < _numObstacleSideChecks; i++)
		{
			FVector start = startPoint + (i + 1) * edgeDir * _obstacleSideCheckInterval;
			FVector stop = start + -obstNormal * _obstacleCheckDistance;

			UKismetSystemLibrary::LineTraceSingle(world, start, stop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, sideHitCheck, false);
			if (!sideHitCheck.bBlockingHit)
			{
				// edge found
				float offsetFromObstacle = lastDistance - distToObstacle;
				sidePoint = start - leanDirection * (_agentRadius + _obstacleSideCheckInterval) + -obstNormal * offsetFromObstacle;
				foundEndPoint = true;
				break;
			}

			lastDistance = sideHitCheck.Distance;
		}
	}
	// if vertex not perp. to obstacle: sweep ray in opposite direction of lean-dir. until obstacle is found to find its edge
	else
	{
		FHitResult sideHitCheck;
		for (int i = 0; i < _numObstacleSideChecks; i++)
		{
			FVector start = startPoint + (i + 1) * -edgeDir * _obstacleSideCheckInterval;
			FVector stop = start + -obstNormal * _obstacleCheckDistance;

			UKismetSystemLibrary::LineTraceSingle(world, start, stop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, sideHitCheck, false);
			if (sideHitCheck.bBlockingHit)
			{
				// edge found
				float offsetFromObstacle = sideHitCheck.Distance - distToObstacle;
				sidePoint = start - leanDirection * _agentRadius + -obstNormal * offsetFromObstacle;
				foundEndPoint = true;
				break;
			}
		}
	}

	// found an endpoint: make sure the sides are not blocked by the obstacles (and thus provide cover from which the agent can potentially attack)
	if (foundEndPoint)
	{
		FHitResult visionFromSideCheck;
		FVector visionFromSideCheckStart = sidePoint + leanDirection * (_sideLeanOffset + _agentRadius);
		FVector visionFromSideCheckStop = visionFromSideCheckStart + -obstNormal * _obstacleCheckDistance;

		UKismetSystemLibrary::LineTraceSingle(world, visionFromSideCheckStart, visionFromSideCheckStop, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, visionFromSideCheck, false);

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


const void ACoverPointGenerator::DrawNavEdges() const
{
	int numEdges = _navGeo.NavMeshEdges.Num();

	UWorld* world = GetWorld();

	for (int i = 0; i < numEdges; i += 2)
	{
		FVector v1 = _navGeo.NavMeshEdges[i];
		FVector v2 = _navGeo.NavMeshEdges[i + 1];

		FVector lineStart = v1;
		FVector lineEnd = v2;

		DrawDebugLine(world, lineStart, lineEnd, FColor::Magenta, true);

		// DEBUG draw edge normals
		FVector dir = (v2 - v1);
		dir.Normalize();

		FVector leftVec = FVector::CrossProduct(dir, FVector::UpVector);
		FVector leftLineStart = v1 + dir * (v2 - v1).Size() * 0.5f;
		FVector leftLineEnd = leftLineStart + (leftVec * 100.0f);

		//DrawDebugLine(world, leftLineStart, leftLineEnd, FColor::Yellow, true);
	}

	int numVerts = _navGeo.MeshVerts.Num();
	for (int i = 0; i < numVerts; i++)
	{
		DrawDebugSphere(world, _navGeo.MeshVerts[i], 10.0f, 5, FColor::Silver, true);
	}
}

const void ACoverPointGenerator::DrawCoverPointLocations() const
{
	if (!_coverPoints.IsValid()) return;

	UWorld* world = GetWorld();
	if (!world)
	{
		UE_LOG(LogTemp, Warning, TEXT("No world found..."));
		return;
	}

	float debugSphereExtent = 30.0f;
	// loop through octree
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints, _coverPoints->GetRootBounds()); it.HasPendingElements(); it.Advance())
	{
		FCoverPointOctreeElement el = it.GetCurrentElement();
		UCoverPoint* cp = el._coverPoint;
		
		if (cp != nullptr)
		{
			DrawDebugSphere(world, cp->_location, debugSphereExtent, 7, FColor::Cyan, true);

			// draw face away
			//DrawDebugLine(world, cp->_location * FVector(1,1,0) + FVector(0,0,150), cp->_location + cp->_dirToCover * -150.0f + FVector(0, 0, 150.0f), FColor::Black, true);
			//DrawDebugLine(world, cp->_location * FVector(1, 1, 0) + FVector(0, 0, 150), cp->_location + cp->_leanDirection * 150.0f * FVector(1, 1, 0) + FVector(0, 0, 150.0f), FColor::Black, true);

		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("num points: %d"), blub);
}

TArray<UCoverPoint*> ACoverPointGenerator::GetCoverPointsWithinExtent(const FVector& position, float extent) const
{
	FBox bbox(position - FVector(extent), position + FVector(extent));

	TArray<UCoverPoint*> points;

	// example code how to query octree
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints, bbox); it.HasPendingElements(); it.Advance())
	{
		// PROBLEM: points are generated within mesh (unreachable). either set min area size in project settings or post process the navigation mesh...
		// probably out of scope for this project: just check if point is reachable
		if (!IsValid(it.GetCurrentElement()._coverPoint))
		{
			UE_LOG(LogTemp, Warning, TEXT("nullptr detected in cover points!"));
			return points;
		}
		points.Emplace(it.GetCurrentElement()._coverPoint);
	}

	int numPoints = points.Num();
	if (numPoints == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("no points in proximity. bbox center: %s, bbox extent: %s"), *bbox.GetCenter().ToString(), *bbox.GetExtent().ToString());
	}

	//DrawDebugBox(GetWorld(), point._location, FVector(100.0f), FColor::Black, false, 20.0f);
	//DrawDebugLine(GetWorld(), point._location, point._location - point._dirToCover * 150.0f, FColor::Black, false, 20.0f, (uint8)'\000', 5.0f);

	return points;
}

int ACoverPointGenerator::GetNumberOfIntersectionsFromCover(const UCoverPoint* cp, const FVector& targetLocation) const
{
	const int infinite = TNumericLimits<int>::Max();
	const float epsilon = 0.00001f;
	const float enemyCrouchHeight = 80.0f;
	const FVector& leanDir = cp->_leanDirection;

	int numHitsSide, numHitsOver = infinite; 
	FVector2D leanDir2D = FVector2D(leanDir) * _sideLeanOffset;

	bool canLeanOver = std::abs(leanDir.Z) > epsilon;
	bool canLeanSide = leanDir2D.SizeSquared() > epsilon;

	UWorld* world = GetWorld();
	if (!IsValid(world)) return infinite;

	// check if agent can lean over this cover point
	if (canLeanOver)
	{
		FVector traceStart = cp->_location;
		traceStart.Z += _standAttackHeight;

		FVector traceEnd = targetLocation;
		traceEnd.Z += enemyCrouchHeight;

		TArray<FHitResult> outHits;
		UKismetSystemLibrary::LineTraceMulti(world, traceStart, traceEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHits, true);
		//DrawDebugLine(world, traceStart, traceEnd, FColor::Green, false, 1.0f);
		//DrawDebugLine(world, cp->_location, traceStart, FColor::Green, false, 2.0f);

		numHitsOver = outHits.Num();
	}

	// check if agent can lean to the side of this cover point
	if (canLeanSide)
	{
		FVector traceStart = cp->_location;
		traceStart.Z += _crouchAttackHeight;
		traceStart.X += leanDir2D.X;
		traceStart.Y += leanDir2D.Y;

		FVector traceEnd = targetLocation;
		traceEnd.Z += enemyCrouchHeight;

		TArray<FHitResult> outHits;
		UKismetSystemLibrary::LineTraceMulti(world, traceStart, traceEnd, ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::None, outHits, true);
		
		//DrawDebugLine(world, cp->_location, traceStart, FColor::Blue, false, 2.0f);
		//DrawDebugLine(world, traceStart, traceEnd, FColor::Blue, false, 1.0f);

		numHitsSide = outHits.Num();
	}

	// optionally: check if can hit with lean-over AND lean-side.

	int numHits;
	// prefer some cover over no cover
	//if (numHitsSide == 0 && numHitsOver > 0) numHits = numHitsOver;
	//else if(numHitsOver == 0 && numHitsSide > 0) numHits = numHitsSide;
	//else numHits = FMath::Min(numHitsSide, numHitsOver);
	numHits = FMath::Min(numHitsSide, numHitsOver);
	FColor color;
	if (numHits == 0)
		color = FColor::Green;
	else if (numHits == 1)
		color = FColor::Yellow;
	else if (numHits == 2)
		color = FColor::Orange;
	else if (numHits == 3)
		color = FColor::Red;
	else
		color = FColor::Cyan;
	//DrawDebugSphere(world, cp->_location, 50.0f, 12, color, false, 1.0f);

	return numHits;
}

bool ACoverPointGenerator::CanAttackFromCover(const UCoverPoint* cp, const FVector& targetLocation) const
{
	const float epsilon = 0.0001;

	const FVector& leanDir = cp->_leanDirection;
	FVector2D leanDir2D = FVector2D(leanDir) * _sideLeanOffset;
	bool canLeanOver = std::abs(leanDir.Z) < epsilon;
	bool canLeanSide = leanDir2D.SizeSquared() < epsilon;

	return true;
}


void ACoverPointGenerator::OnNavigationGenerationFinished(class ANavigationData* NavData)
{
	ARecastNavMesh* navMeshData = static_cast<ARecastNavMesh*>(NavData);
	_navGeo.bGatherNavMeshEdges = true;
	navMeshData->BeginBatchQuery();
	navMeshData->GetDebugGeometry(_navGeo);
	navMeshData->FinishBatchQuery();

	FlushPersistentDebugLines(GetWorld());
	UpdateCoverPointData();
	// apply octree optimization
	_coverPoints->ShrinkElements();

	// draw debug data
	//DrawNavEdges();
	DrawCoverPointLocations();

	UE_LOG(LogTemp, Warning, TEXT("Cover Point data updated!"));
}

