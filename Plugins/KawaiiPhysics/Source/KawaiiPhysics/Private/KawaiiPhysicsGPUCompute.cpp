// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "KawaiiPhysicsGPUCompute.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "PhysicsUtilities.h"

DEFINE_LOG_CATEGORY_STATIC(LogKawaiiPhysicsGPU, Log, All);

bool UKawaiiPhysicsGPUCompute::IsGPUSimulationAvailable()
{
	return GRHISupportsComputeShaders && !IsOpenGLPlatform(GMaxRHIShaderPlatform);
}

bool UKawaiiPhysicsGPUCompute::Initialize(int32 NumBones)
{
	if (!IsGPUSimulationAvailable())
	{
		UE_LOG(LogKawaiiPhysicsGPU, Warning, TEXT("GPU simulation not available on this platform."));
		return false;
	}

	if (NumBones <= 0)
	{
		UE_LOG(LogKawaiiPhysicsGPU, Warning, TEXT("Invalid bone count: %d"), NumBones);
		return false;
	}

	AllocatedBoneCount = NumBones;
	bInitialized = true;

	UE_LOG(LogKawaiiPhysicsGPU, Log, TEXT("GPU simulation initialized for %d bones."), NumBones);
	return true;
}

void UKawaiiPhysicsGPUCompute::TickGPU(
	TArray<FVector>& BoneLocations,
	TArray<FVector>& BonePrevLocations,
	const TArray<FVector>& BonePoseLocations,
	const TArray<FVector4>& BoneSettings,
	int32 NumActiveBones,
	float DeltaTime,
	float PredictionScale,
	const FVector& SkelCompMoveVector,
	const FQuat& SkelCompMoveRotation)
{
	if (!bInitialized || NumActiveBones <= 0)
	{
		return;
	}

	// ============================================================
	// PROOF OF CONCEPT ARCHITECTURE
	// ============================================================
	// In a full implementation, this would:
	//
	// 1. Create/upload structured buffer with bone data
	// 2. Dispatch compute shader (numGroups = ceil(NumBones / 64))
	// 3. Read back results from GPU buffer
	//
	// For now, this is a CPU fallback that demonstrates the algorithm.
	// The actual GPU path requires:
	// - FRHIStructuredBuffer for bone data
	// - FRHIUnorderedAccessView for UAV access
	// - FGlobalShader subclass for the compute shader
	// - FRHICommandList::DispatchComputeShader()
	// ============================================================

	// CPU fallback: run the same algorithm as the compute shader
	const float PredMove = PredictionScale;

	for (int32 i = 0; i < NumActiveBones; ++i)
	{
		FVector& Location = BoneLocations[i];
		FVector& PrevLocation = BonePrevLocations[i];
		const FVector& PoseLocation = BonePoseLocations[i];

		if (!BoneSettings.IsValidIndex(i))
			continue;

		const float Damping = BoneSettings[i].X;
		const float WorldDampingLoc = BoneSettings[i].Y;
		const float Stiffness = BoneSettings[i].W; // PoseLocation.w stores stiffness

		// Verlet integration
		const FVector Velocity = (Location - PrevLocation) * (1.0f - Damping);
		FVector NewLocation = Location + Velocity;

		// Stiffness: pull towards pose
		const FVector PullDir = PoseLocation - NewLocation;
		NewLocation += PullDir * Stiffness;

		// World damping with prediction
		const FVector PredictedMove = SkelCompMoveVector * (1.0f + PredMove);
		NewLocation += PredictedMove * (1.0f - WorldDampingLoc);

		// Store results
		PrevLocation = Location;
		Location = NewLocation;
	}
}

void UKawaiiPhysicsGPUCompute::Release()
{
	bInitialized = false;
	AllocatedBoneCount = 0;

	UE_LOG(LogKawaiiPhysicsGPU, Log, TEXT("GPU simulation resources released."));
}
