// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ExternalForces/KawaiiPhysicsExternalForce.h"
#include "KawaiiPhysicsExternalForceField.generated.h"

/**
 * Vortex Force Field - rotational force around an axis.
 * Useful for whirlwinds, tornados, spinning effects.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICSEXTENSION_API FKawaiiPhysics_ExternalForce_Vortex : public FKawaiiPhysics_ExternalForce
{
	GENERATED_BODY()

	/** Center location of the vortex (world space) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field")
	FVector Center = FVector::ZeroVector;

	/** Axis of rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field")
	FVector Axis = FVector::UpVector;

	/** Strength of the rotational force */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field", meta=(ClampMin="0"))
	float Strength = 100.0f;

	/** Falloff distance from center (0 = infinite range) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field", meta=(ClampMin="0"))
	float FalloffRadius = 500.0f;

	/** Upward lift along the axis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field")
	float Lift = 0.0f;

	virtual void Apply(FKawaiiPhysicsModifyBone& Bone, FAnimNode_KawaiiPhysics& Node,
	                   FComponentSpacePoseContext& PoseContext, const FTransform& BoneTM = FTransform::Identity) override
	{
		if (!bIsEnabled) return;

		const FVector BoneWorldLoc = Node.ConvertSimulationSpaceLocation(
			PoseContext, Node.SimulationSpace, EKawaiiPhysicsSimulationSpace::WorldSpace, Bone.Location);
		const FVector ToBone = BoneWorldLoc - Center;
		const float Dist = ToBone.Size();

		if (FalloffRadius > 0 && Dist > FalloffRadius) return;
		if (Dist < KINDA_SMALL_NUMBER) return;

		const float Falloff = FalloffRadius > 0 ? (1.0f - Dist / FalloffRadius) : 1.0f;
		const FVector Tangent = FVector::CrossProduct(Axis.GetSafeNormal(), ToBone.GetSafeNormal());
		const FVector VortexForce = Tangent * Strength * Falloff + Axis.GetSafeNormal() * Lift * Falloff;

		const FVector ForceSim = Node.ConvertSimulationSpaceVector(
			PoseContext, EKawaiiPhysicsSimulationSpace::WorldSpace, Node.SimulationSpace, VortexForce);
		Bone.Location += ForceSim * Node.GetStepDeltaTime();
	}
};

/**
 * Radial Force Field - pushes or pulls bones from a center point.
 * Useful for explosions, implosions, magnetic effects.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICSEXTENSION_API FKawaiiPhysics_ExternalForce_Radial : public FKawaiiPhysics_ExternalForce
{
	GENERATED_BODY()

	/** Center location of the radial force (world space) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field")
	FVector Center = FVector::ZeroVector;

	/** Strength of the radial force (positive = push away, negative = pull towards) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field")
	float Strength = 200.0f;

	/** Falloff distance from center (0 = infinite range) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field", meta=(ClampMin="0"))
	float FalloffRadius = 500.0f;

	/** Upward bias (adds vertical component) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field")
	float UpwardBias = 0.0f;

	virtual void Apply(FKawaiiPhysicsModifyBone& Bone, FAnimNode_KawaiiPhysics& Node,
	                   FComponentSpacePoseContext& PoseContext, const FTransform& BoneTM = FTransform::Identity) override
	{
		if (!bIsEnabled) return;

		const FVector BoneWorldLoc = Node.ConvertSimulationSpaceLocation(
			PoseContext, Node.SimulationSpace, EKawaiiPhysicsSimulationSpace::WorldSpace, Bone.Location);
		const FVector ToBone = BoneWorldLoc - Center;
		const float Dist = ToBone.Size();

		if (FalloffRadius > 0 && Dist > FalloffRadius) return;
		if (Dist < KINDA_SMALL_NUMBER) return;

		const float Falloff = FalloffRadius > 0 ? (1.0f - Dist / FalloffRadius) : 1.0f;
		const FVector Direction = ToBone.GetSafeNormal();
		const FVector RadialForce = Direction * Strength * Falloff + FVector::UpVector * UpwardBias * Falloff;

		const FVector ForceSim = Node.ConvertSimulationSpaceVector(
			PoseContext, EKawaiiPhysicsSimulationSpace::WorldSpace, Node.SimulationSpace, RadialForce);
		Bone.Location += ForceSim * Node.GetStepDeltaTime();
	}
};

/**
 * Turbulence Force Field - Perlin noise-based random force.
 * Useful for wind gusts, underwater currents, magical effects.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICSEXTENSION_API FKawaiiPhysics_ExternalForce_Turbulence : public FKawaiiPhysics_ExternalForce
{
	GENERATED_BODY()

	/** Base strength of turbulence */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field", meta=(ClampMin="0"))
	float Strength = 50.0f;

	/** Noise frequency (higher = more chaotic) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field", meta=(ClampMin="0.001"))
	float Frequency = 0.5f;

	/** Noise speed (how fast the pattern evolves) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field", meta=(ClampMin="0"))
	float Speed = 1.0f;

	/** Spatial scale of the noise */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field", meta=(ClampMin="0.001"))
	float SpatialScale = 0.01f;

	/** Time offset for this force instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Force Field")
	float TimeOffset = 0.0f;

	virtual void Apply(FKawaiiPhysicsModifyBone& Bone, FAnimNode_KawaiiPhysics& Node,
	                   FComponentSpacePoseContext& PoseContext, const FTransform& BoneTM = FTransform::Identity) override
	{
		if (!bIsEnabled) return;

		const float Time = FPlatformTime::Seconds() * Speed + TimeOffset;
		const FVector BoneWorldLoc = Node.ConvertSimulationSpaceLocation(
			PoseContext, Node.SimulationSpace, EKawaiiPhysicsSimulationSpace::WorldSpace, Bone.Location);

		// 3D Perlin noise for each axis
		const float NoiseX = FMath::PerlinNoise3D(FVector(BoneWorldLoc * SpatialScale + FVector(Time, 0, 0)));
		const float NoiseY = FMath::PerlinNoise3D(FVector(BoneWorldLoc * SpatialScale + FVector(0, Time, 0)));
		const float NoiseZ = FMath::PerlinNoise3D(FVector(BoneWorldLoc * SpatialScale + FVector(0, 0, Time)));

		const FVector TurbForce = FVector(NoiseX, NoiseY, NoiseZ) * Strength;

		const FVector ForceSim = Node.ConvertSimulationSpaceVector(
			PoseContext, EKawaiiPhysicsSimulationSpace::WorldSpace, Node.SimulationSpace, TurbForce);
		Bone.Location += ForceSim * Node.GetStepDeltaTime();
	}
};
