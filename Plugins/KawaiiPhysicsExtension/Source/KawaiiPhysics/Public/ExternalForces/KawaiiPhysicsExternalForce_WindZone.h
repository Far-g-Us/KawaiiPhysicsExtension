// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "ExternalForces/KawaiiPhysicsExternalForce.h"
#include "Curves/CurveFloat.h"

#include "KawaiiPhysicsExternalForce_WindZone.generated.h"

/**
 * Wind Zone: local wind source with shape and falloff.
 * Unlike the global Wind force (which uses WindDirectionalSource),
 * WindZone defines a local area where wind affects bones.
 *
 * Use cases:
 * - Fan blowing in a 200 unit radius
 * - Doorway creating a draft
 * - Explosion with radial impulse fading by distance
 * - Local turbulence zone
 */
USTRUCT(BlueprintType, DisplayName = "Wind Zone")
struct KAWAIIPHYSICSEXTENSION_API FKawaiiPhysics_ExternalForce_WindZone : public FKawaiiPhysics_ExternalForce
{
	GENERATED_BODY()

	FKawaiiPhysics_ExternalForce_WindZone()
	{
		bCanSelectForceSpace = false;
		ExternalForceSpace = EExternalForceSpace::WorldSpace;
	}

	/** Shape of the wind zone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone")
	EKawaiiPhysicsWindZoneShape Shape = EKawaiiPhysicsWindZoneShape::Sphere;

	/** Size of the wind zone (Sphere=radius, Box=half-extent, Cone=length) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone",
		meta = (ClampMin = "1"))
	float ZoneSize = 200.0f;

	/** Secondary size for Box (Y half-extent) and Cone (base radius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone",
		meta = (ClampMin = "1", EditCondition = "Shape == EKawaiiPhysicsWindZoneShape::Box || Shape == EKawaiiPhysicsWindZoneShape::Cone"))
	float ZoneSizeY = 200.0f;

	/** Tertiary size for Box (Z half-extent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone",
		meta = (ClampMin = "1", EditCondition = "Shape == EKawaiiPhysicsWindZoneShape::Box"))
	float ZoneSizeZ = 200.0f;

	/** Wind direction in world space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone")
	FVector WindDirection = FVector(1, 0, 0);

	/** Wind strength (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone",
		meta = (ClampMin = "0"))
	float WindStrength = 100.0f;

	/**
	 * Falloff curve: X = normalized distance (0=center, 1=edge), Y = strength multiplier.
	 * If empty, uses linear falloff (1 - distance).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone")
	FRuntimeFloatCurve FalloffCurve;

	/** Wind direction noise angle (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone",
		meta = (Units = "Degrees", ClampMin = "0"))
	float WindDirectionNoiseAngle = 5.0f;

	/** Actor to attach this wind zone to (optional). If set, zone follows this actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone")
	TWeakObjectPtr<AActor> AttachActor;

	/** Offset from AttachActor location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics|WindZone",
		meta = (EditCondition = "AttachActor != nullptr"))
	FVector AttachOffset = FVector::ZeroVector;

public:
	virtual void PreApply(FAnimNode_KawaiiPhysics& Node, FComponentSpacePoseContext& PoseContext) override;
	virtual void Apply(FKawaiiPhysicsModifyBone& Bone, FAnimNode_KawaiiPhysics& Node,
	                   FComponentSpacePoseContext& PoseContext,
	                   const FTransform& BoneTM = FTransform::Identity) override;

private:
	/** Get the world position of the wind zone center */
	FVector GetZoneCenter() const;

	/** Check if a point is inside the zone shape */
	bool IsInsideZone(const FVector& Point) const;

	/** Get falloff multiplier at normalized distance (0-1) */
	float GetFalloffMultiplier(float NormalizedDistance) const;
};

/**
 * Enum for wind zone shapes
 */
UENUM(BlueprintType)
enum class EKawaiiPhysicsWindZoneShape : uint8
{
	/** Sphere: radius-based falloff */
	Sphere,
	/** Box: axis-aligned box */
	Box,
	/** Cone: directional cone from base to tip */
	Cone
};
