// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "ExternalForces/KawaiiPhysicsExternalForce_WindZone.h"
#include "AnimNode_KawaiiPhysics.h"
#include "DrawDebugHelpers.h"

void FKawaiiPhysics_ExternalForce_WindZone::PreApply(FAnimNode_KawaiiPhysics& Node,
                                                       FComponentSpacePoseContext& PoseContext)
{
	Super::PreApply(Node, PoseContext);
}

void FKawaiiPhysics_ExternalForce_WindZone::Apply(FKawaiiPhysicsModifyBone& Bone,
                                                    FAnimNode_KawaiiPhysics& Node,
                                                    FComponentSpacePoseContext& PoseContext,
                                                    const FTransform& BoneTM)
{
	const FVector ZoneCenter = GetZoneCenter();
	const FVector BoneWorldPos = PoseContext.AnimInstanceProxy->GetComponentTransform().TransformPosition(Bone.Location);
	const FVector ToBone = BoneWorldPos - ZoneCenter;

	// Check if bone is inside the zone
	if (!IsInsideZone(ToBone))
	{
		return;
	}

	// Calculate normalized distance from center (0=center, 1=edge)
	float NormalizedDistance = 0.0f;
	switch (Shape)
	{
	case EKawaiiPhysicsWindZoneShape::Sphere:
		NormalizedDistance = ToBone.Size() / ZoneSize;
		break;
	case EKawaiiPhysicsWindZoneShape::Box:
		NormalizedDistance = FMath::Max3(
			FMath::Abs(ToBone.X) / ZoneSize,
			FMath::Abs(ToBone.Y) / ZoneSizeY,
			FMath::Abs(ToBone.Z) / ZoneSizeZ);
		break;
	case EKawaiiPhysicsWindZoneShape::Cone:
		// Distance along cone axis / length
		NormalizedDistance = ToBone.X / ZoneSize;
		break;
	}

	NormalizedDistance = FMath::Clamp(NormalizedDistance, 0.0f, 1.0f);

	// Get falloff multiplier
	const float FalloffMultiplier = GetFalloffMultiplier(NormalizedDistance);

	// Calculate wind velocity
	FVector WindVelocity = WindDirection.GetSafeNormal() * WindStrength * FalloffMultiplier;

	// Add noise
	if (WindDirectionNoiseAngle > 0)
	{
		WindVelocity = FMath::VRandCone(WindVelocity, FMath::DegreesToRadians(WindDirectionNoiseAngle));
	}

	// Convert to simulation space and apply to bone
	const FVector WindInSimSpace = Node.ConvertSimulationSpaceVector(
		PoseContext, EKawaiiPhysicsSimulationSpace::WorldSpace, Node.SimulationSpace, WindVelocity);

	// Apply wind as velocity offset (modify PrevLocation like Verlet integration)
	const float DeltaTime = FMath::Max(Node.DeltaTimeOld, 0.001f);
	Bone.PrevLocation -= WindInSimSpace * DeltaTime;
}

FVector FKawaiiPhysics_ExternalForce_WindZone::GetZoneCenter() const
{
	if (AttachActor.IsValid())
	{
		return AttachActor->GetActorLocation() + AttachOffset;
	}
	return AttachOffset;
}

bool FKawaiiPhysics_ExternalForce_WindZone::IsInsideZone(const FVector& ToBone) const
{
	switch (Shape)
	{
	case EKawaiiPhysicsWindZoneShape::Sphere:
		return ToBone.SizeSquared() <= ZoneSize * ZoneSize;

	case EKawaiiPhysicsWindZoneShape::Box:
		return FMath::Abs(ToBone.X) <= ZoneSize
			&& FMath::Abs(ToBone.Y) <= ZoneSizeY
			&& FMath::Abs(ToBone.Z) <= ZoneSizeZ;

	case EKawaiiPhysicsWindZoneShape::Cone:
		{
			// Cone along X axis: tip at (ZoneSize, 0, 0), base at (0, 0, 0)
			if (ToBone.X < 0 || ToBone.X > ZoneSize)
			{
				return false;
			}
			const float RadiusAtX = ZoneSizeY * (ToBone.X / ZoneSize);
			const float DistanceFromAxis = FVector(0, ToBone.Y, ToBone.Z).Size();
			return DistanceFromAxis <= RadiusAtX;
		}

	default:
		return false;
	}
}

float FKawaiiPhysics_ExternalForce_WindZone::GetFalloffMultiplier(float NormalizedDistance) const
{
	// Use curve if available
	if (FalloffCurve.GetRichCurve()->GetNumKeys() > 0)
	{
		return FalloffCurve.GetRichCurve()->Eval(NormalizedDistance);
	}

	// Default: linear falloff
	return 1.0f - NormalizedDistance;
}
