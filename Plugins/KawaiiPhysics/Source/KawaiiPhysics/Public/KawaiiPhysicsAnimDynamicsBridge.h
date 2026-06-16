// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_AnimDynamics.h"
#include "KawaiiPhysicsTypes.h"
#include "KawaiiPhysicsAnimDynamicsBridge.generated.h"

/**
 * Converter from AnimDynamics settings to KawaiiPhysics settings.
 * Helps migrate from AnimDynamics to KawaiiPhysics.
 */
UCLASS(BlueprintType)
class KAWAIIPHYSICS_API UKawaiiPhysicsAnimDynamicsBridge : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Convert AnimDynamics settings to KawaiiPhysics settings.
	 *
	 * @param AnimDynamics Settings to convert
	 * @return Equivalent KawaiiPhysics settings
	 *
	 * NOTE: Tested with UE 5.3-5.5 AnimNode_AnimDynamics.
	 * Some parameters have no direct equivalent and are documented as warnings.
	 * AnimDynamics per-axis constraints (LinearX/Y/Z, AngularX/Y/Z) are NOT
	 * supported — KawaiiPhysics uses Twist/Swing decomposition instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Migration")
	static FKawaiiPhysicsSettings ConvertFromAnimDynamics(const FAnimNode_AnimDynamics& AnimDynamics)
	{
		FKawaiiPhysicsSettings Settings;

		// Damping: AnimDynamics uses LinearDamping (0-10), KawaiiPhysics uses Damping (0-1)
		Settings.Damping = FMath::Clamp(AnimDynamics.LinearDamping / 10.0f, 0.0f, 1.0f);

		// Stiffness: AnimDynamics uses SpringStiffness (0-1000), KawaiiPhysics uses Stiffness (0-1)
		Settings.Stiffness = FMath::Clamp(AnimDynamics.SpringStiffness / 100.0f, 0.0f, 1.0f);

		// LimitAngle: AnimDynamics uses AngleConstraint
		Settings.LimitAngle = AnimDynamics.AngularConstraintAngle;

		// Collision Radius: AnimDynamics uses SphereCollisionRadius
		Settings.Radius = AnimDynamics.SphereCollisionRadius;

		// World Damping
		Settings.WorldDampingLocation = FMath::Clamp(1.0f - AnimDynamics.WorldDampingTranslation, 0.0f, 1.0f);
		Settings.WorldDampingRotation = FMath::Clamp(1.0f - AnimDynamics.WorldDampingRotation, 0.0f, 1.0f);

		// Gravity Scale: AnimDynamics has bApplyGravity, KawaiiPhysics uses Gravity vector
		// NOTE: AnimDynamics gravity is per-bone scale, KawaiiPhysics uses global Gravity vector
		// User must manually set Gravity in KawaiiPhysics if AnimDynamics had gravity enabled

		// Per-axis constraints: AnimDynamics has LinearX/Y/Z and AngularX/Y/Z
		// KawaiiPhysics uses Twist/Swing decomposition instead
		// If AnimDynamics had asymmetric constraints, user must manually configure Twist/Swing

		// Snap to reference pose: AnimDynamics has SnapPosition/SnapRotation
		// KawaiiPhysics has no direct equivalent — use Damping/Stiffness to control return-to-pose

		// BoundBone: AnimDynamics can bind simulation to another bone
		// KawaiiPhysics uses SimulationBaseBone for similar purpose
		// User must manually set SimulationBaseBone if AnimDynamics had BoundBone

		return Settings;
	}

	/**
	 * Check if a bone has AnimDynamics component attached.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Migration")
	static bool HasAnimDynamics(USkeletalMeshComponent* SkelComp, FName BoneName)
	{
		if (!SkelComp)
		{
			return false;
		}

		UAnimInstance* AnimInstance = SkelComp->GetAnimInstance();
		if (!AnimInstance)
		{
			return false;
		}

		TArray<UAnimInstance*> LinkedInstances;
		AnimInstance->GetLinkedAnimInstances(LinkedInstances);

		for (UAnimInstance* LinkedInstance : LinkedInstances)
		{
			if (!LinkedInstance)
			{
				continue;
			}

			const UClass* InstanceClass = LinkedInstance->GetClass();
			if (InstanceClass && InstanceClass->FindPropertyByName(FName("AnimDynamics")))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Get recommended KawaiiPhysics settings for common AnimDynamics use cases.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Migration")
	static FKawaiiPhysicsSettings GetRecommendedSettings(FName UseCase)
	{
		FKawaiiPhysicsSettings Settings;

		if (UseCase == FName("Hair"))
		{
			Settings.Damping = 0.5f;
			Settings.Stiffness = 0.03f;
			Settings.LimitAngle = 60.0f;
			Settings.Radius = 2.0f;
		}
		else if (UseCase == FName("Cloth"))
		{
			Settings.Damping = 0.4f;
			Settings.Stiffness = 0.05f;
			Settings.LimitAngle = 45.0f;
			Settings.Radius = 1.5f;
		}
		else if (UseCase == FName("Breast"))
		{
			Settings.Damping = 0.35f;
			Settings.Stiffness = 0.05f;
			Settings.LimitAngle = 35.0f;
			Settings.Radius = 3.0f;
			Settings.VolumePreservation = 0.5f;
			Settings.Pressure = 1.0f;
			Settings.bUseTwistSwingLimits = true;
			Settings.TwistLimitAngle = 20.0f;
			Settings.SwingLimitYAngle = 30.0f;
			Settings.SwingLimitZAngle = 35.0f;
		}
		else if (UseCase == FName("Tail"))
		{
			Settings.Damping = 0.4f;
			Settings.Stiffness = 0.02f;
			Settings.LimitAngle = 45.0f;
			Settings.Radius = 2.5f;
		}

		return Settings;
	}
};
