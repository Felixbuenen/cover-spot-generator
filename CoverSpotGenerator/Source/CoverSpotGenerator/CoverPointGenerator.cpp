// Fill out your copyright notice in the Description page of Project Settings.


#include "CoverPointGenerator.h"

#include "DrawDebugHelpers.h"
#include "Engine/LevelBounds.h"

// Sets default values
ACoverPointGenerator::ACoverPointGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACoverPointGenerator::BeginPlay()
{
	Super::BeginPlay();

	// ---- OCTREE SAMPLE CODE -------

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
	}
	
	// example code how to query octree
	//for (TCoverPointOctree::TConstElementBoxIterator<> *it; it->HasPendingElements(); it->Advance())
	//{
	//	const FCoverPointOctreeElement currEl = it->GetCurrentElement();
	//}
	
	// ---- OCTREE SAMPLE CODE -------

	UWorld* world = GetWorld();
	if (world)
	{
		UNavigationSystemBase* navSystem = GetWorld()->GetNavigationSystem();
		if (navSystem)
		{
			ARecastNavMesh* navMeshData = static_cast<ARecastNavMesh*>(FNavigationSystem::GetCurrent<UNavigationSystemV1>(world)->GetDefaultNavDataInstance());
			_navGeo.bGatherNavMeshEdges = true;
			_navGeo.bGatherPolyEdges = false;
			navMeshData->BeginBatchQuery();
			navMeshData->GetDebugGeometry(_navGeo);
			navMeshData->FinishBatchQuery();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No NavSystem found!"));
		}
	}

	// init cover point data
	UpdateCoverPointData();

	DrawNavEdges();
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
	//const float obstacleEdgeCheckDist = 
	// loop over all nav mesh edges
	int numEdges = _navGeo.NavMeshEdges.Num();
	//UE_LOG(LogTemp, Warning, TEXT("num edges: %d"), numEdges);

	for (int i = 0; i < numEdges; i += 2)
	{
		FVector v1 = _navGeo.NavMeshEdges[i];
		FVector v2 = _navGeo.NavMeshEdges[i + 1];
		
		float edgeLength = (v1 - v2).Size();
		//UE_LOG(LogTemp, Warning, TEXT("edge length: %f"), edgeLength);
	
		FVector edgeDir = (v2 - v1);
		edgeDir.Normalize();

		if (!AreaAlreadyHasCoverPoint(v1)) _coverPoints->AddElement(FCoverPointOctreeElement(MakeShared<FCoverPoint>(v1, FVector(0.0f)), _coverPointMinDistance));
		if (!AreaAlreadyHasCoverPoint(v2)) _coverPoints->AddElement(FCoverPointOctreeElement(MakeShared<FCoverPoint>(v2, FVector(0.0f)), _coverPointMinDistance));
		int numCoverPoints = (int)(edgeLength / _coverPointMinDistance);
		int numInternalPoints = numCoverPoints - 2; // exclude points on vertex

		// optionally clamp to max. number of cover points for nav mesh edge
		if (_maxNumPointsPerEdge > -1) numInternalPoints = FMath::Clamp<int>(numInternalPoints, 0, _maxNumPointsPerEdge);

		if (numInternalPoints < 2)
		{
			FVector pointLocation = v1 + edgeDir * edgeLength * 0.5f;
			// attempt to place cover point in middle of edge
			//if (!AreaAlreadyHasCoverPoint(pointLocation)) _coverPoints->AddElement(FCoverPointOctreeElement(MakeShared<FCoverPoint>(pointLocation, FVector(0.0f)), _coverPointMinDistance));

			_coverPoints->AddElement(FCoverPointOctreeElement(MakeShared<FCoverPoint>(pointLocation, FVector(0.0f)), _coverPointMinDistance));
		}
		else
		{
			// attempt to place cover points on edge vertices
			// place cover points on the edge
			
		
			UE_LOG(LogTemp, Warning, TEXT("Num internal points: %d"), numInternalPoints);
		
			float coverPointInterval = edgeLength / (float)(numInternalPoints + 1);
		
			FVector startLoc = v1 + edgeDir * coverPointInterval;

			for (int idx = 0; idx < numInternalPoints; idx++)
			{
				FVector pointLocation = startLoc + idx * edgeDir * coverPointInterval;
				_coverPoints->AddElement(FCoverPointOctreeElement(MakeShared<FCoverPoint>(pointLocation, FVector(0.0f)), _coverPointMinDistance));
			}
		}
	}
}

bool ACoverPointGenerator::AreaAlreadyHasCoverPoint(const FVector& position) const
{
	FBox bbox(position - FVector(0.0f), position + FVector(0.0f));

	// example code how to query octree
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints.Get(), bbox); it.HasPendingElements(); it.Advance())
	{
		if ((it.GetCurrentElement()._coverPoint->_location - position).Size() < _coverPointMinDistance)
		{
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
	}

	int numVerts = _navGeo.MeshVerts.Num();
	for (int i = 0; i < numVerts; i++)
	{
		DrawDebugSphere(world, _navGeo.MeshVerts[i], 10.0f, 5, FColor::Red, true);
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
	for (TCoverPointOctree::TConstElementBoxIterator<> it(*_coverPoints.Get(), _coverPoints->GetRootBounds()); it.HasPendingElements(); it.Advance())
	{
		FCoverPointOctreeElement el = it.GetCurrentElement();
		TSharedPtr<FCoverPoint> cp = el._coverPoint;
		
		DrawDebugSphere(world, cp->_location, debugSphereExtent, 7, FColor::Cyan, true);
	}
}
