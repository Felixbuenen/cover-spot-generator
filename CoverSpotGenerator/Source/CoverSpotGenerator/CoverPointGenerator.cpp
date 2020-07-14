// Fill out your copyright notice in the Description page of Project Settings.


#include "CoverPointGenerator.h"

#include "DrawDebugHelpers.h"
#include "Engine/LevelBounds.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ACoverPointGenerator::ACoverPointGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ACoverPointGenerator::BeginPlay()
{
	Super::BeginPlay();

	ULevel* currLevel = GetLevel();
	if (currLevel)
	{
		FBox levelBbox = ALevelBounds::CalculateLevelBounds(currLevel);
		
		// init octree
		_coverPoints = MakeUnique<TCoverPointOctree>(levelBbox.GetCenter(), levelBbox.GetExtent().GetMax());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("level ptr couldn't be loaded"));
		return;
	}

	UWorld* world = GetWorld();
	if (world)
	{
		UNavigationSystemBase* navSystem = GetWorld()->GetNavigationSystem();
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
}


// Called every frame
void ACoverPointGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}

// stores candidate cover-locations in the cover point octree
void ACoverPointGenerator::UpdateCoverPointData()
{
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

		FVector outLeftSide, outRightSide;
		TestAndAddSidePoints(world, v2, v1, edgeDir, obstacleCheckHit.Normal, outLeftSide, outRightSide);

		bool hasLeftSidePoint = !outLeftSide.Equals(v2);
		bool hasRightSidePoint = !outRightSide.Equals(v1);

		bool isStandingCover = IsStandingCover(world, v1 + edgeDir * edgeLength * 0.5f, obstacleCheckHit.Normal);
		if (isStandingCover) continue;

		// edge between the two side cover points
		FVector internalEdge = (outLeftSide - outRightSide);
		float internalEdgeLength = internalEdge.Size();
		internalEdge /= internalEdgeLength;

		int numInternalPoints = (int)(internalEdgeLength / _coverPointMinDistance) + 1;
		if (numInternalPoints <= 1) continue;

		// optionally clamp to max. number of cover points for nav mesh edge
		if (_maxNumPointsPerEdge > -1) numInternalPoints = FMath::Clamp<int>(numInternalPoints, 0, _maxNumPointsPerEdge);
		float coverPointInterval = internalEdgeLength / (float)(numInternalPoints - 1);

		FVector startLoc = outRightSide;
		if (hasRightSidePoint)
		{
			startLoc += internalEdge * coverPointInterval;
			numInternalPoints--;
		}
		if (hasLeftSidePoint)
		{
			numInternalPoints--;
		}

		for (int idx = 0; idx < numInternalPoints; idx++)
		{
			FVector pointLocation = startLoc + idx * internalEdge * coverPointInterval;

			if (ProvidesCover(world, pointLocation, obstacleCheckHit.Normal) && CanLeanOver(world, pointLocation, obstacleCheckHit.Normal))
			{
				_coverPoints->AddElement(FCoverPointOctreeElement(MakeShared<FCoverPoint>(pointLocation, FVector(0.0f)), _coverPointMinDistance));
			}
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
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints.Get(), bbox); it.HasPendingElements(); it.Advance())
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

bool ACoverPointGenerator::IsStandingCover(UWorld* world, FVector coverLocation, FVector coverFaceNormal) const
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

void ACoverPointGenerator::TestAndAddSidePoints(UWorld* world, const FVector& leftVertex, const FVector& rightVertex, const FVector& edgeDir, const FVector& obstNormal, FVector& outLeftSide, FVector& outRightSide) const
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

	if (leftCheck.bBlockingHit && rightCheck.bBlockingHit && FVector::DotProduct(leftCheck.Normal, rightCheck.Normal) < 0.8f)
	{
		return;
	}

	// check if there is side-cover to the endpoints of this nav edge. If so, generate cover points at these locations.
	FVector leftSideCoverPoint, rightSideCoverPoint;

	if (GetSideCoverPoint(world, leftVertex, rightToNormal, obstNormal, edgeDir, leftSideCoverPoint))
	{
		if (!AreaAlreadyHasCoverPoint(leftSideCoverPoint))
		{
			_coverPoints->AddElement(FCoverPointOctreeElement(MakeShared<FCoverPoint>(leftSideCoverPoint, FVector(0.0f)), _coverPointMinDistance));
			outLeftSide = leftSideCoverPoint;
		}
	}
	if (GetSideCoverPoint(world, rightVertex, -rightToNormal, obstNormal, -edgeDir, rightSideCoverPoint))
	{
		if (!AreaAlreadyHasCoverPoint(rightSideCoverPoint))
		{
			_coverPoints->AddElement(FCoverPointOctreeElement(MakeShared<FCoverPoint>(rightSideCoverPoint, FVector(0.0f)), _coverPointMinDistance));
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
	int blub = 0;
	// loop through octree
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints.Get(), _coverPoints->GetRootBounds()); it.HasPendingElements(); it.Advance())
	{
		blub++;
		FCoverPointOctreeElement el = it.GetCurrentElement();
		TSharedPtr<FCoverPoint> cp = el._coverPoint;
		
		DrawDebugSphere(world, cp->_location, debugSphereExtent, 7, FColor::Cyan, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("num points: %d"), blub);
}
