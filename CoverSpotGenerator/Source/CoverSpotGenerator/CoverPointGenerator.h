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
	float _coverPointMinDistance = 75.0f;

	UPROPERTY()
	int _maxNumPointsPerEdge = 3;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void UpdateCoverPointData();

	bool AreaAlreadyHasCoverPoint(const FVector& position) const;

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
