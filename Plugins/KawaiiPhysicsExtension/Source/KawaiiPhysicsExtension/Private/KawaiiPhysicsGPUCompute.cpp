// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "KawaiiPhysicsGPUCompute.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RHIShaderPlatform.h"
#include "DataDrivenShaderPlatformInfo.h"

DEFINE_LOG_CATEGORY_STATIC(LogKawaiiPhysicsGPU, Log, All);

bool UKawaiiPhysicsGPUCompute::IsGPUSimulationAvailable()
{
	// Compute shaders require DX12/Vulkan/Metal — OpenGL does not support them.
	// On Vulkan, IsOpenGLPlatform returns false (correct). DX12 also returns false.
	return !IsOpenGLPlatform(GMaxRHIShaderPlatform);
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

static constexpr int32 GPUMinBoneCountThreshold = 50;

// GPU sync overhead exceeds CPU benefit for small bone counts
// Recommend CPU simulation for < GPUMinBoneCountThreshold bones
if (NumBones < GPUMinBoneCountThreshold)
	{
		UE_LOG(LogKawaiiPhysicsGPU, Warning,
			TEXT("GPU simulation disabled: %d bones is below the %d-bone threshold. "
			     "GPU sync overhead exceeds CPU benefit for small chains. Use CPU simulation."),
			NumBones, GPUMinBoneCountThreshold);
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
	const FQuat& SkelCompMoveRotation,
	const FVector& GravityInSimSpace)
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

	// Bounds validation: ensure caller-provided count doesn't exceed array sizes
	const int32 MaxBones = FMath::Min3(BoneLocations.Num(), BonePrevLocations.Num(), BonePoseLocations.Num());
	const int32 SafeNumBones = FMath::Min(NumActiveBones, MaxBones);

	for (int32 i = 0; i < SafeNumBones; ++i)
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
		FVector NewLocation = Location + Velocity * DeltaTime;

		// Apply gravity
		NewLocation += GravityInSimSpace * DeltaTime * DeltaTime;

		// Stiffness: pull towards pose
		const FVector PullDir = PoseLocation - NewLocation;
		NewLocation += PullDir * Stiffness;

		// World damping with prediction
		const FVector PredictedMove = SkelCompMoveVector * (1.0f + PredMove);
		NewLocation += PredictedMove * (1.0f - WorldDampingLoc);

		// Apply rotation if provided
		if (!SkelCompMoveRotation.IsIdentity())
		{
			NewLocation = SkelCompMoveRotation.RotateVector(NewLocation - PoseLocation) + PoseLocation;
		}

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
