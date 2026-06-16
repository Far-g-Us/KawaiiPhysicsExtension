// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_KawaiiPhysicsImpulse.generated.h"

/**
 * AnimNotify that applies an impulse to a specific bone in KawaiiPhysics simulation.
 * Useful for footstep reactions, impacts, landings, and other point-based physics events.
 *
 * Usage:
 * 1. Add this notify to your animation sequence at the desired frame
 * 2. Set BoneName to the bone that receives the impulse (e.g., pelvis, thigh_L)
 * 3. Set Direction and Strength of the impulse
 * 4. The impulse is applied to the bone's velocity in the next KawaiiPhysics simulation step
 *
 * Common use cases:
 * - Footstep: pelvis bone, upward direction, strength 50-100
 * - Impact: hit bone, away-from-attacker direction, strength 100-300
 * - Landing: pelvis bone, downward-then-upward (bounce), strength 80-150
 * - Explosion: all chain roots, radial direction from explosion center, strength 200-500
 */
UCLASS(Blueprintable, meta = (DisplayName = "KawaiiPhysics: Impulse"))
class KAWAIIPHYSICS_API UAnimNotify_KawaiiPhysicsImpulse : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_KawaiiPhysicsImpulse(const FObjectInitializer& ObjectInitializer);

	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                    const FAnimNotifyEventReference& EventReference) override;

public:
	/** Name of the bone to apply the impulse to (must be in KawaiiPhysics ModifyBones) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impulse")
	FName BoneName = FName("pelvis");

	/** Direction of the impulse in component space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impulse")
	FVector Direction = FVector(0, 0, 1);

	/** Strength of the impulse (cm/s). Negative values push in opposite direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impulse",
		meta = (ClampMin = "-1000", ClampMax = "1000"))
	float Strength = 100.0f;

	/**
	 * If true, Direction is interpreted as world space and automatically converted to component space.
	 * Useful for explosions where direction depends on actor position.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impulse")
	bool bWorldSpaceDirection = false;

	/**
	 * If true, apply impulse to ALL bones in the chain starting from BoneName.
	 * Strength decays by DecayMultiplier per bone down the chain.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impulse")
	bool bApplyToEntireChain = false;

	/**
	 * Strength multiplier per bone down the chain (0-1).
	 * Example: 0.7 means each bone gets 70% of the previous bone's impulse.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impulse",
		meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bApplyToEntireChain"))
	float DecayMultiplier = 0.7f;

#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif
};
