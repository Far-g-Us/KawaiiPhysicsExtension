// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComputeShaderDeclaration.h"
#include "KawaiiPhysicsGPUCompute.generated.h"

/**
 * GPU-accelerated simulation for KawaiiPhysics.
 * Offloads Verlet integration and stiffness to the GPU via compute shaders.
 * Collision and bone constraints remain on CPU (hybrid approach).
 *
 * Usage: Call TickGPU() from AnimNode evaluation when bEnableGPUSimulation is true.
 */
UCLASS(BlueprintType)
class KAWAIIPHYSICS_API UKawaiiPhysicsGPUCompute : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Whether GPU simulation is available on this platform.
	 */
	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|GPU")
	static bool IsGPUSimulationAvailable();

	/**
	 * Initialize the GPU simulation with the given number of bones.
	 */
	bool Initialize(int32 NumBones);

	/**
	 * Upload bone data to GPU, run compute shader, read back results.
	 * @param BoneLocations - Current bone locations (in/out)
	 * @param BonePrevLocations - Previous bone locations (in/out)
	 * @param BonePoseLocations - Target pose locations (in)
	 * @param BoneSettings - Physics settings per bone (damping, stiffness, etc.)
	 * @param DeltaTime - Frame delta time
	 * @param PredictionScale - Prediction scale for world damping
	 */
	void TickGPU(
		TArray<FVector>& BoneLocations,
		TArray<FVector>& BonePrevLocations,
		const TArray<FVector>& BonePoseLocations,
		const TArray<FVector4>& BoneSettings,
		int32 NumActiveBones,
		float DeltaTime,
		float PredictionScale,
		const FVector& SkelCompMoveVector,
		const FQuat& SkelCompMoveRotation
	);

	/**
	 * Release GPU resources.
	 */
	void Release();

private:
	/** Whether GPU resources are initialized */
	bool bInitialized = false;

	/** Number of bones the buffers were allocated for */
	int32 AllocatedBoneCount = 0;

	// GPU buffers would be here in a full implementation
	// TRefCountPtr<FRHIStructuredBuffer> BoneBuffer;
	// TRefCountPtr<FRHIUnorderedAccessView> BoneBufferUAV;
};
