// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "AnimNode_KawaiiPhysics.h"

#include "AnimationRuntime.h"
#include "KawaiiPhysicsBoneConstraintsDataAsset.h"
#include "KawaiiPhysicsCustomExternalForce.h"
#include "ExternalForces/KawaiiPhysicsExternalForce.h"
#include "KawaiiPhysicsLimitsDataAsset.h"
#include "KawaiiPhysicsSharedCollisionSubsystem.h"
#include "Animation/AnimInstanceProxy.h"
#include "Curves/CurveFloat.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SceneInterface.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "StaticMeshResources.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsSettings.h"

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
#include "PhysicsEngine/SkeletalBodySetup.h"
#endif

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 6
#include "Animation/AnimInstance.h"
#endif

#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

#include "KawaiiPhysics.h"
#include "AnimNode_KawaiiPhysicsInternal.h"

void FAnimNode_KawaiiPhysics::ApplyLimitsDataAsset(const FBoneContainer& RequiredBones)
{
	auto Initialize = [&RequiredBones](auto& Targets)
	{
		for (auto& Target : Targets)
		{
			Target.DrivingBone.Initialize(RequiredBones);
		}
	};
	auto RemoveAllSourceDataAssets = [](auto& Targets)
	{
		Targets.RemoveAll([](const FCollisionLimitBase& Limit)
		{
			return Limit.SourceType == ECollisionSourceType::DataAsset;
		});
	};

	RemoveAllSourceDataAssets(SphericalLimitsData);
	RemoveAllSourceDataAssets(CapsuleLimitsData);
	RemoveAllSourceDataAssets(BoxLimitsData);
	RemoveAllSourceDataAssets(PlanarLimitsData);

	if (LimitsDataAsset)
	{
		SphericalLimitsData.Append(LimitsDataAsset->SphericalLimits);
		CapsuleLimitsData.Append(LimitsDataAsset->CapsuleLimits);
		BoxLimitsData.Append(LimitsDataAsset->BoxLimits);
		PlanarLimitsData.Append(LimitsDataAsset->PlanarLimits);

		Initialize(SphericalLimitsData);
		Initialize(CapsuleLimitsData);
		Initialize(BoxLimitsData);
		Initialize(PlanarLimitsData);
	}
}

void FAnimNode_KawaiiPhysics::ApplyPhysicsAsset(const FBoneContainer& RequiredBones)
{
	auto Initialize = [&RequiredBones](auto& Targets)
	{
		for (auto& Target : Targets)
		{
			Target.DrivingBone.Initialize(RequiredBones);
		}
	};
	auto RemoveAllSourcePhysicsAssets = [](auto& Targets)
	{
		Targets.RemoveAll([](const FCollisionLimitBase& Limit)
		{
			return Limit.SourceType == ECollisionSourceType::PhysicsAsset;
		});
	};

	RemoveAllSourcePhysicsAssets(SphericalLimitsData);
	RemoveAllSourcePhysicsAssets(CapsuleLimitsData);
	RemoveAllSourcePhysicsAssets(BoxLimitsData);

	if (PhysicsAssetForLimits)
	{
		for (const auto& BodySetup : PhysicsAssetForLimits->SkeletalBodySetups)
		{
			if (!BodySetup)
			{
				continue;
			}
			FBoneReference DrivingBone = BodySetup->BoneName;
			DrivingBone.Initialize(RequiredBones);
			if (!DrivingBone.IsValidToEvaluate(RequiredBones))
			{
				continue;
			}

			const FKAggregateGeom& AggGeom = BodySetup->AggGeom;
			for (const auto& SphereElem : AggGeom.SphereElems)
			{
				FSphericalLimit NewLimit;
				NewLimit.DrivingBone = DrivingBone;
				NewLimit.OffsetLocation = SphereElem.Center;
				NewLimit.Radius = SphereElem.Radius;
				NewLimit.SourceType = ECollisionSourceType::PhysicsAsset;
				SphericalLimitsData.Add(NewLimit);
			}
			for (const auto& CapsuleElem : AggGeom.SphylElems)
			{
				FCapsuleLimit NewLimit;
				NewLimit.DrivingBone = DrivingBone;
				NewLimit.OffsetLocation = CapsuleElem.Center;
				NewLimit.OffsetRotation = CapsuleElem.Rotation;
				NewLimit.Length = CapsuleElem.Length;
				NewLimit.Radius = CapsuleElem.Radius;
				NewLimit.SourceType = ECollisionSourceType::PhysicsAsset;
				CapsuleLimitsData.Add(NewLimit);
			}
			for (const auto& BoxElem : AggGeom.BoxElems)
			{
				FBoxLimit NewLimit;
				NewLimit.DrivingBone = DrivingBone;
				NewLimit.OffsetLocation = BoxElem.Center;
				NewLimit.OffsetRotation = BoxElem.Rotation;
				NewLimit.Extent = FVector(BoxElem.X, BoxElem.Y, BoxElem.Z) / 2.0f;
				NewLimit.SourceType = ECollisionSourceType::PhysicsAsset;
				BoxLimitsData.Add(NewLimit);
			}
		}

		Initialize(SphericalLimitsData);
		Initialize(CapsuleLimitsData);
		Initialize(BoxLimitsData);
	}
}

void FAnimNode_KawaiiPhysics::ApplyBoneConstraintDataAsset(const FBoneContainer& RequiredBones)
{
	BoneConstraintsData.Empty();
	if (BoneConstraintsDataAsset)
	{
		BoneConstraintsData = BoneConstraintsDataAsset->GenerateBoneConstraints();
		for (auto& BoneConstraint : BoneConstraintsData)
		{
			BoneConstraint.InitializeBone(RequiredBones);
		}
	}
}

void FAnimNode_KawaiiPhysics::UpdateSphericalLimits(TArray<FSphericalLimit>& Limits, FComponentSpacePoseContext& Output,
                                                    const FBoneContainer& BoneContainer,
                                                    const FTransform& ComponentTransform) const
{
	for (auto& Sphere : Limits)
	{
		SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_UpdateSphericalLimit);

		if (Sphere.DrivingBone.IsValidToEvaluate(BoneContainer))
		{
			const FCompactPoseBoneIndex CompactPoseIndex = Sphere.DrivingBone.GetCompactPoseIndex(BoneContainer);
			FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(CompactPoseIndex);

			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, BoneTransform,
			                                                 CompactPoseIndex, BCS_BoneSpace);
			BoneTransform.SetRotation(Sphere.OffsetRotation.Quaternion() * BoneTransform.GetRotation());
			BoneTransform.AddToTranslation(Sphere.OffsetLocation);

			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, BoneTransform,
			                                                 CompactPoseIndex, BCS_BoneSpace);

			BoneTransform =
				ConvertSimulationSpaceTransform(Output, EKawaiiPhysicsSimulationSpace::ComponentSpace, SimulationSpace,
				                                BoneTransform);
			Sphere.Location = BoneTransform.GetLocation();
			Sphere.Rotation = BoneTransform.GetRotation();

			Sphere.bEnable = true;
		}
		else
		{
			Sphere.bEnable = false;
		}
	}
}

void FAnimNode_KawaiiPhysics::UpdateCapsuleLimits(TArray<FCapsuleLimit>& Limits, FComponentSpacePoseContext& Output,
                                                  const FBoneContainer& BoneContainer,
                                                  const FTransform& ComponentTransform) const
{
	for (auto& Capsule : Limits)
	{
		SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_UpdateCapsuleLimit);

		if (Capsule.DrivingBone.IsValidToEvaluate(BoneContainer))
		{
			const FCompactPoseBoneIndex CompactPoseIndex = Capsule.DrivingBone.GetCompactPoseIndex(BoneContainer);
			FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(CompactPoseIndex);

			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, BoneTransform,
			                                                 CompactPoseIndex, BCS_BoneSpace);
			BoneTransform.SetRotation(Capsule.OffsetRotation.Quaternion() * BoneTransform.GetRotation());
			BoneTransform.AddToTranslation(Capsule.OffsetLocation);

			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, BoneTransform,
			                                                 CompactPoseIndex, BCS_BoneSpace);

			BoneTransform =
				ConvertSimulationSpaceTransform(Output, EKawaiiPhysicsSimulationSpace::ComponentSpace, SimulationSpace,
				                                BoneTransform);
			Capsule.Location = BoneTransform.GetLocation();
			Capsule.Rotation = BoneTransform.GetRotation();

			Capsule.bEnable = true;
		}
		else
		{
			Capsule.bEnable = false;
		}
	}
}

void FAnimNode_KawaiiPhysics::UpdateBoxLimits(TArray<FBoxLimit>& Limits, FComponentSpacePoseContext& Output,
                                              const FBoneContainer& BoneContainer,
                                              const FTransform& ComponentTransform) const
{
	for (auto& Box : Limits)
	{
		SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_UpdateBoxLimit);

		if (Box.DrivingBone.IsValidToEvaluate(BoneContainer))
		{
			const FCompactPoseBoneIndex CompactPoseIndex = Box.DrivingBone.GetCompactPoseIndex(BoneContainer);
			FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(CompactPoseIndex);

			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, BoneTransform,
			                                                 CompactPoseIndex, BCS_BoneSpace);
			BoneTransform.SetRotation(Box.OffsetRotation.Quaternion() * BoneTransform.GetRotation());
			BoneTransform.AddToTranslation(Box.OffsetLocation);

			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, BoneTransform,
			                                                 CompactPoseIndex, BCS_BoneSpace);

			BoneTransform =
				//GetSimSpaceTransformFromComponentSpace(SimulationSpace, Output, BoneTransform);
				ConvertSimulationSpaceTransform(Output, EKawaiiPhysicsSimulationSpace::ComponentSpace, SimulationSpace,
				                                BoneTransform);
			Box.Location = BoneTransform.GetLocation();
			Box.Rotation = BoneTransform.GetRotation();

			Box.bEnable = true;
		}
		else
		{
			Box.bEnable = false;
		}
	}
}

void FAnimNode_KawaiiPhysics::UpdatePlanerLimits(TArray<FPlanarLimit>& Limits, FComponentSpacePoseContext& Output,
                                                 const FBoneContainer& BoneContainer,
                                                 const FTransform& ComponentTransform) const
{
	for (auto& Planar : Limits)
	{
		SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_UpdatePlanerLimit);

		if (Planar.DrivingBone.IsValidToEvaluate(BoneContainer))
		{
			const FCompactPoseBoneIndex CompactPoseIndex = Planar.DrivingBone.GetCompactPoseIndex(BoneContainer);
			FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(CompactPoseIndex);

			FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, BoneTransform,
			                                                 CompactPoseIndex, BCS_BoneSpace);
			BoneTransform.SetRotation(Planar.OffsetRotation.Quaternion() * BoneTransform.GetRotation());
			BoneTransform.AddToTranslation(Planar.OffsetLocation);

			FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, BoneTransform,
			                                                 CompactPoseIndex, BCS_BoneSpace);

			BoneTransform = ConvertSimulationSpaceTransform(Output, EKawaiiPhysicsSimulationSpace::ComponentSpace,
			                                                SimulationSpace, BoneTransform);
			Planar.Location = BoneTransform.GetLocation();
			Planar.Rotation = BoneTransform.GetRotation();
			Planar.Rotation.Normalize();
			Planar.Plane = FPlane(Planar.Location, Planar.Rotation.GetUpVector());

			Planar.bEnable = true;
		}
		else
		{
			if (Planar.DrivingBone.BoneName.IsNone())
			{
				// Maybe the DrivingBone is set to empty for the floor
				FTransform OffsetTransform(Planar.OffsetRotation, Planar.OffsetLocation);
				OffsetTransform = ConvertSimulationSpaceTransform(Output, EKawaiiPhysicsSimulationSpace::ComponentSpace,
				                                                  SimulationSpace, OffsetTransform);

				Planar.Location = OffsetTransform.GetLocation();
				Planar.Rotation = OffsetTransform.GetRotation();
				Planar.Rotation.Normalize();
				Planar.Plane = FPlane(Planar.Location, Planar.Rotation.GetUpVector());
				Planar.bEnable = true;
			}
			else
			{
				Planar.bEnable = false;
			}
		}
	}
}

// World collision: sweep bone sphere from PrevLocation to Location against scene geometry.
// Uses sweep test (not overlap) to catch fast-moving bones that might tunnel through walls.
// Returns true if collision occurred and bone position was adjusted.
void FAnimNode_KawaiiPhysics::AdjustByWorldCollision(FComponentSpacePoseContext& Output, FKawaiiPhysicsModifyBone& Bone,
                                                     const USkeletalMeshComponent* OwningComp)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_WorldCollision);

	// bridge dummy は ParentIndex<0 だがコリジョン代理として World Collision に参加させる（PrevLocation→Location でスイープ）
	// BridgeDummy dummies have ParentIndex<0 but must still sweep against world geometry (they are collision proxies)
	if (!OwningComp || !OwningComp->GetWorld() || (Bone.ParentIndex < 0 && !Bone.bBridgeDummy))
	{
		return;
	}


	/** the trace is not done in game thread, so TraceTag does not draw debug traces*/
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KawaiiCollision));

	if (bIgnoreSelfComponent)
	{
		Params.AddIgnoredComponent(OwningComp);
	}

	// Get collision settings from component	
	ECollisionChannel TraceChannel = bOverrideCollisionParams
		                                 ? CollisionChannelSettings.GetObjectType()
		                                 : OwningComp->GetCollisionObjectType();
	FCollisionResponseParams ResponseParams = bOverrideCollisionParams
		                                          ? FCollisionResponseParams(
			                                          CollisionChannelSettings.GetResponseToChannels())
		                                          : FCollisionResponseParams(
			                                          OwningComp->GetCollisionResponseToChannels());
	const UWorld* World = OwningComp->GetWorld();

	const FVector TraceStartLocationWS =
		ConvertSimulationSpaceLocation(Output, SimulationSpace, EKawaiiPhysicsSimulationSpace::WorldSpace,
		                               Bone.PrevLocation);
	const FVector TraceEndLocationWS =
		ConvertSimulationSpaceLocation(Output, SimulationSpace, EKawaiiPhysicsSimulationSpace::WorldSpace,
		                               Bone.Location);

	if (bIgnoreSelfComponent)
	{
		// Do sphere sweep
		FHitResult Result;
		bool bHit = World->SweepSingleByChannel(
			Result, TraceStartLocationWS, TraceEndLocationWS, FQuat::Identity,
			TraceChannel, FCollisionShape::MakeSphere(Bone.PhysicsSettings.Radius), Params, ResponseParams);
		if (bHit)
		{
			if (Result.bStartPenetrating)
			{
				Bone.Location =
					ConvertSimulationSpaceLocation(Output, EKawaiiPhysicsSimulationSpace::WorldSpace, SimulationSpace,
					                               TraceEndLocationWS + Result.Normal * Result.PenetrationDepth);
			}
			else
			{
				Bone.Location =
					ConvertSimulationSpaceLocation(Output, EKawaiiPhysicsSimulationSpace::WorldSpace, SimulationSpace,
					                               Result.Location);
			}
		}
	}
	else
	{
		// Do sphere sweep and ignore bones later
		WorldCollisionHitsScratch.Reset();
		bool bHit = World->SweepMultiByChannel(WorldCollisionHitsScratch, TraceStartLocationWS,
		                                       TraceEndLocationWS, FQuat::Identity, TraceChannel,
		                                       FCollisionShape::MakeSphere(Bone.PhysicsSettings.Radius), Params,
		                                       ResponseParams);
		if (!bHit)
		{
			return;
		}

		bool IsIgnoreHit;
		if (IgnoreBoneNamePrefixCache != IgnoreBoneNamePrefix)
		{
			IgnoreBoneNamePrefixCache = IgnoreBoneNamePrefix;
			IgnoreBoneNamePrefixStrings.Reset(IgnoreBoneNamePrefix.Num());
			for (const FName& BoneNamePrefix : IgnoreBoneNamePrefix)
			{
				if (!BoneNamePrefix.IsNone())
				{
					IgnoreBoneNamePrefixStrings.Add(BoneNamePrefix.ToString());
				}
			}
		}

		for (const auto& Result : WorldCollisionHitsScratch)
		{
			if (!Result.bBlockingHit)
			{
				continue;
			}

			//should we ignore this hit?
			IsIgnoreHit = false;
			if (Result.Component == OwningComp && Result.BoneName != NAME_None)
			{
				IsIgnoreHit = Result.BoneName == Bone.BoneRef.BoneName;
				if (!IsIgnoreHit)
				{
					for (const auto& BoneRef : IgnoreBones)
					{
						if (BoneRef.BoneName == Result.BoneName)
						{
							IsIgnoreHit = true;
							break;
						}
					}
				}
				// プレフィックス未設定（一般的なケース）ではToString自体を回避
				// Skip the ToString entirely when no prefixes are set (the common case)
				if (!IsIgnoreHit && !IgnoreBoneNamePrefixStrings.IsEmpty())
				{
					const FString ResultBoneNameString = Result.BoneName.ToString();
					for (const FString& BoneNamePrefix : IgnoreBoneNamePrefixStrings)
					{
						if (ResultBoneNameString.StartsWith(BoneNamePrefix))
						{
							IsIgnoreHit = true;
							break;
						}
					}
				}
			}

			//found the blocking hit we shouldn't ignore!
			if (!IsIgnoreHit)
			{
				if (Result.bStartPenetrating)
				{
					Bone.Location =
						ConvertSimulationSpaceLocation(Output, EKawaiiPhysicsSimulationSpace::WorldSpace,
						                               SimulationSpace,
						                               TraceEndLocationWS + Result.Normal * Result.PenetrationDepth);
				}
				else
				{
					Bone.Location =
						ConvertSimulationSpaceLocation(Output, EKawaiiPhysicsSimulationSpace::WorldSpace,
						                               SimulationSpace,
						                               Result.Location);
				}
				break;
			}
		}
	}
}

// Sphere collision: push bone out of (Outer) or into (Inner) spherical limits.
// Outer: bone must stay outside the sphere (push out if penetrated)
// Inner: bone must stay inside the sphere (pull in if escaped)
void FAnimNode_KawaiiPhysics::AdjustBySphereCollision(FKawaiiPhysicsModifyBone& Bone, TArray<FSphericalLimit>& Limits)
{
	for (auto& Sphere : Limits)
	{
		if (!Sphere.bEnable || Sphere.Radius <= 0.0f)
		{
			continue;
		}

		// Collision layer filtering: skip if layers don't match
		if (CollisionLayer != 0 && Sphere.CollisionLayer != 0 && (CollisionLayer & Sphere.CollisionLayer) == 0)
		{
			continue;
		}

		if (Sphere.LimitType == ESphericalLimitType::Outer)
		{
			const float LimitDistanceOuter = Sphere.Radius + Bone.PhysicsSettings.Radius;
			const FVector Delta = Bone.Location - Sphere.Location;
			const float DistSq = Delta.SizeSquared();
			if (DistSq > LimitDistanceOuter * LimitDistanceOuter)
			{
				continue;
			}

			const float Dist = FMath::Sqrt(DistSq);
			if (Dist > KINDA_SMALL_NUMBER)
			{
				const FVector CollisionNormal = Delta / Dist;
				Bone.Location += (LimitDistanceOuter - Dist) * CollisionNormal;
				RecordBoneCollisionEvent(Bone.BoneRef.BoneName, Bone.Location, CollisionNormal);
			}
		}
		else
		{
			// ボーン半径がスフィア半径以上（内側に収まらない退化ケース）では実効内半径を0にクランプし、
			// ガードと補正で同一値を使うことで中心へピン留め（符号反転による反対側への飛びを防止）
			// Clamp the effective inner radius to 0 for the degenerate case where the bone radius >= sphere radius,
			// and reuse it for both the guard and the correction so the bone is pinned to the center (no sign-flip overshoot)
			const float LimitDistanceInner = FMath::Max(Sphere.Radius - Bone.PhysicsSettings.Radius, 0.0f);
			const FVector Delta = Bone.Location - Sphere.Location;
			const float DistSq = Delta.SizeSquared();
			if (DistSq < LimitDistanceInner * LimitDistanceInner)
			{
				continue;
			}

			const float Dist = FMath::Sqrt(DistSq);
			Bone.Location = Dist > KINDA_SMALL_NUMBER
				                ? Sphere.Location + LimitDistanceInner * (Delta / Dist)
				                : Sphere.Location;
		}
	}
}

// Capsule collision: push bone out of capsule volume.
// Finds closest point on capsule segment to bone center, pushes outward by (bone radius + capsule radius).
void FAnimNode_KawaiiPhysics::AdjustByCapsuleCollision(FKawaiiPhysicsModifyBone& Bone, TArray<FCapsuleLimit>& Limits)
{
	for (auto& Capsule : Limits)
	{
		if (!Capsule.bEnable || Capsule.Radius <= 0.0f || Capsule.Length <= 0.0f)
		{
			continue;
		}

		// Collision layer filtering: skip if layers don't match
		if (CollisionLayer != 0 && Capsule.CollisionLayer != 0 && (CollisionLayer & Capsule.CollisionLayer) == 0)
		{
			continue;
		}

		FVector StartPoint = Capsule.Location + Capsule.Rotation.GetAxisZ() * Capsule.Length * 0.5f;
		FVector EndPoint = Capsule.Location + Capsule.Rotation.GetAxisZ() * Capsule.Length * -0.5f;
		const float DistSquared = FMath::PointDistToSegmentSquared(Bone.Location, StartPoint, EndPoint);

		const float LimitDistance = Bone.PhysicsSettings.Radius + Capsule.Radius;
		if (DistSquared < LimitDistance * LimitDistance)
		{
			FVector ClosestPoint = FMath::ClosestPointOnSegment(Bone.Location, StartPoint, EndPoint);
			FVector CollisionNormal = (Bone.Location - ClosestPoint);
			// If bone is on the capsule axis, use a perpendicular direction
			if (CollisionNormal.IsNearlyZero())
			{
				CollisionNormal = Capsule.Rotation.GetAxisX();
			}
			else
			{
				CollisionNormal = CollisionNormal.GetSafeNormal();
			}
			Bone.Location = ClosestPoint + CollisionNormal * LimitDistance;
			RecordBoneCollisionEvent(Bone.BoneRef.BoneName, Bone.Location, CollisionNormal);
		}
	}
}

// Box collision: push bone sphere out of box volume.
// Transforms bone position to box local space, finds closest point on box surface,
// pushes outward along the shortest penetration axis.
void FAnimNode_KawaiiPhysics::AdjustByBoxCollision(FKawaiiPhysicsModifyBone& Bone, TArray<FBoxLimit>& Limits)
{
	for (auto& Box : Limits)
	{
		if (!Box.bEnable)
		{
			continue;
		}

		// Collision layer filtering: skip if layers don't match
		if (CollisionLayer != 0 && Box.CollisionLayer != 0 && (CollisionLayer & Box.CollisionLayer) == 0)
		{
			continue;
		}

		FTransform BoxTransform(Box.Rotation, Box.Location);
		float SphereRadius = Bone.PhysicsSettings.Radius;

		FVector LocalSphereCenter = BoxTransform.InverseTransformPosition(Bone.Location);
		FBox LocalBox(-Box.Extent, Box.Extent);
		if (FMath::SphereAABBIntersection(FSphere(LocalSphereCenter, SphereRadius), LocalBox))
		{
			// Calculate the point of the Box closest to the center of the Sphere
			FVector ClosestPoint = LocalSphereCenter;
			ClosestPoint.X = FMath::Clamp(ClosestPoint.X, LocalBox.Min.X, LocalBox.Max.X);
			ClosestPoint.Y = FMath::Clamp(ClosestPoint.Y, LocalBox.Min.Y, LocalBox.Max.Y);
			ClosestPoint.Z = FMath::Clamp(ClosestPoint.Z, LocalBox.Min.Z, LocalBox.Max.Z);

			FVector PushOutVector = LocalSphereCenter - ClosestPoint;
			float Distance = PushOutVector.Size();

			// When the bone sphere is completely buried inside the box, forced to push.
			if (PushOutVector.IsNearlyZero())
			{
				PushOutVector = LocalSphereCenter;
				Distance = SphereRadius;
			}

			// push
			if (Distance <= SphereRadius)
			{
				FVector PushOutDirection = PushOutVector.GetSafeNormal();
				FVector NewLocalSphereCenter = ClosestPoint + PushOutDirection * SphereRadius;
				Bone.Location = BoxTransform.TransformPosition(NewLocalSphereCenter);
				const FVector WorldNormal = BoxTransform.TransformVectorNoScale(PushOutDirection);
				RecordBoneCollisionEvent(Bone.BoneRef.BoneName, Bone.Location, WorldNormal);
			}
		}
	}
}

// Planar collision: push bone to the positive side of a plane.
// Used as a floor/ceiling/wall constraint. Bone radius is added to the plane offset.
void FAnimNode_KawaiiPhysics::AdjustByPlanerCollision(FKawaiiPhysicsModifyBone& Bone, TArray<FPlanarLimit>& Limits)
{
	for (auto& Planar : Limits)
	{
		if (!Planar.bEnable)
		{
			continue;
		}

		FVector PointOnPlane = FVector::PointPlaneProject(Bone.Location, Planar.Plane);
		const float DistSquared = (Bone.Location - PointOnPlane).SizeSquared();

		FVector IntersectionPoint;
		if (DistSquared < Bone.PhysicsSettings.Radius * Bone.PhysicsSettings.Radius ||
			FMath::SegmentPlaneIntersection(Bone.Location, Bone.PrevLocation, Planar.Plane, IntersectionPoint))
		{
			Bone.Location = PointOnPlane + Planar.Rotation.GetUpVector() * Bone.PhysicsSettings.Radius;
		}
	}
}

// Angle limit: decompose bone rotation into twist and swing components relative to parent,
// then clamp each component to its configured limit. Uses quaternion decomposition:
//   twist = projection of rotation axis onto parent forward direction
//   swing = perpendicular component
// If any component exceeds its limit, the rotation is rebuilt with clamped angles.
void FAnimNode_KawaiiPhysics::AdjustByAngleLimit(
	FKawaiiPhysicsModifyBone& Bone,
	const FKawaiiPhysicsModifyBone& ParentBone)
{
	if (Bone.PhysicsSettings.bUseTwistSwingLimits)
	{
		// Twist/Swing decomposition mode
		if (Bone.TwistLimitAngle == 0.0f && Bone.SwingLimitYAngle == 0.0f && Bone.SwingLimitZAngle == 0.0f)
		{
			return;
		}

		const FVector BoneDir = (Bone.Location - ParentBone.Location).GetSafeNormal();
		const FVector PoseDir = (Bone.PoseLocation - ParentBone.PoseLocation).GetSafeNormal();

		if (BoneDir.IsNearlyZero() || PoseDir.IsNearlyZero())
		{
			return;
		}

		// Build local frame from pose direction
		// Forward axis = pose direction (bone chain direction)
		const FVector Forward = PoseDir;
		// Build a consistent right/up from the forward axis
		const FVector Arbitrary = (FMath::Abs(FVector::DotProduct(Forward, FVector::UpVector)) < 0.99f)
			? FVector::UpVector
			: FVector::RightVector;
		const FVector Right = FVector::CrossProduct(Forward, Arbitrary).GetSafeNormal();
		const FVector Up = FVector::CrossProduct(Right, Forward).GetSafeNormal();

		// Decompose the rotation from pose to current into twist + swing
		const FQuat PoseQuat = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
		const FVector Cross1 = FVector::CrossProduct(BoneDir, Right);
		FVector CurrentUp;
		if (Cross1.IsNearlyZero())
		{
			// BoneDir is nearly parallel to Right — use Up from the pose frame directly
			CurrentUp = Up;
		}
		else
		{
			CurrentUp = FVector::CrossProduct(Cross1, BoneDir).GetSafeNormal();
			if (CurrentUp.IsNearlyZero())
			{
				CurrentUp = Up;
			}
		}
		const FQuat CurrentQuat = FRotationMatrix::MakeFromXZ(BoneDir, CurrentUp).ToQuat();
		const FQuat DeltaQuat = PoseQuat.Inverse() * CurrentQuat;

		// Convert to axis-angle
		FVector Axis;
		float Angle;
		DeltaQuat.ToAxisAndAngle(Axis, Angle);
		Angle = FMath::RadiansToDegrees(Angle);

		if (Angle < KINDA_SMALL_NUMBER)
		{
			return;
		}

		// Decompose into twist (along forward) and swing (perpendicular)
		const float TwistComponent = FMath::Clamp(FVector::DotProduct(Axis, Forward), -1.0f, 1.0f);
		const float TwistAngle = FMath::Abs(Angle * TwistComponent);
		const float SwingAngle = FMath::Abs(Angle * FMath::Sqrt(FMath::Max(0.0f, 1.0f - TwistComponent * TwistComponent)));

		// Project swing into Y and Z components
		const FVector SwingAxis = (Axis - Forward * TwistComponent).GetSafeNormal();
		const float SwingYComponent = FVector::DotProduct(SwingAxis, Right);
		const float SwingZComponent = FVector::DotProduct(SwingAxis, Up);
		const float SwingYAngle = FMath::Abs(SwingAngle * SwingYComponent);
		const float SwingZAngle = FMath::Abs(SwingAngle * SwingZComponent);

		// Clamp each component
		float OverTwist = TwistAngle - Bone.TwistLimitAngle;
		float OverSwingY = SwingYAngle - Bone.SwingLimitYAngle;
		float OverSwingZ = SwingZAngle - Bone.SwingLimitZAngle;

		// If any component is over limit, rebuild the clamped rotation
		if (OverTwist > 0.0f || OverSwingY > 0.0f || OverSwingZ > 0.0f)
		{
			// Rebuild clamped angle
			float ClampedTwistAngle = FMath::Clamp(TwistAngle, 0.0f, Bone.TwistLimitAngle);
			float ClampedSwingYAngle = FMath::Clamp(SwingYAngle, 0.0f, Bone.SwingLimitYAngle);
			float ClampedSwingZAngle = FMath::Clamp(SwingZAngle, 0.0f, Bone.SwingLimitZAngle);

			// Rebuild swing axis from clamped components
			FVector ClampedSwingAxis = FVector::ZeroVector;
			if (SwingAngle > KINDA_SMALL_NUMBER)
			{
				ClampedSwingAxis = (Right * SwingYComponent + Up * SwingZComponent).GetSafeNormal();
			}

			// Rebuild the total clamped angle
			const float ClampedSwingTotal = FMath::Sqrt(
				ClampedSwingYAngle * ClampedSwingYAngle + ClampedSwingZAngle * ClampedSwingZAngle);
			const float ClampedTotalAngle = FMath::DegreesToRadians(
				FMath::Sqrt(ClampedTwistAngle * ClampedTwistAngle + ClampedSwingTotal * ClampedSwingTotal));

			if (ClampedTotalAngle > KINDA_SMALL_NUMBER)
			{
				FVector ClampedAxis;
				if (ClampedSwingTotal > KINDA_SMALL_NUMBER)
				{
					ClampedAxis = (Forward * TwistComponent + ClampedSwingAxis).GetSafeNormal();
				}
				else
				{
					ClampedAxis = Forward;
				}

				const FQuat ClampedDelta = FQuat(ClampedAxis, ClampedTotalAngle);
				const FQuat ClampedQuat = PoseQuat * ClampedDelta;
				const FVector NewDir = ClampedQuat.GetForwardVector();
				Bone.Location = NewDir * (Bone.Location - ParentBone.Location).Size() + ParentBone.Location;
			}
		}
	}
	else
	{
		// Legacy single-angle mode
		if (Bone.PhysicsSettings.LimitAngle == 0.0f)
		{
			return;
		}

		FVector BoneDir = (Bone.Location - ParentBone.Location).GetSafeNormal();
		const FVector PoseDir = (Bone.PoseLocation - ParentBone.PoseLocation).GetSafeNormal();
		const FVector Axis = FVector::CrossProduct(PoseDir, BoneDir);
		const float Angle = FMath::Atan2(Axis.Size(), FVector::DotProduct(PoseDir, BoneDir));
		const float AngleOverLimit = FMath::RadiansToDegrees(Angle) - Bone.PhysicsSettings.LimitAngle;

		if (AngleOverLimit > 0.0f)
		{
			const FVector SafeAxis = Axis.GetSafeNormal();
			if (!SafeAxis.IsNearlyZero())
			{
				BoneDir = BoneDir.RotateAngleAxis(-AngleOverLimit, SafeAxis);
				Bone.Location = BoneDir * (Bone.Location - ParentBone.Location).Size() + ParentBone.Location;
			}
		}
	}
}

void FAnimNode_KawaiiPhysics::AdjustByPlanarConstraint(FKawaiiPhysicsModifyBone& Bone,
                                                       const FKawaiiPhysicsModifyBone& ParentBone)
{
	if (PlanarConstraint != EPlanarConstraint::None)
	{
		FVector PlaneNormal;
		switch (PlanarConstraint)
		{
		case EPlanarConstraint::X:
			PlaneNormal = ParentBone.PoseRotation.GetAxisX();
			break;
		case EPlanarConstraint::Y:
			PlaneNormal = ParentBone.PoseRotation.GetAxisY();
			break;
		case EPlanarConstraint::Z:
			PlaneNormal = ParentBone.PoseRotation.GetAxisZ();
			break;
		default:
			return;
		}
		FPlane Plane(ParentBone.Location, PlaneNormal);
		Bone.Location = FVector::PointPlaneProject(Bone.Location, Plane);
	}
}

const TArray<float> XPBDComplianceValues =
{
	0.00000000004f, // 0.04 x 10^(-9) (M^2/N) Concrete
	0.00000000016f, // 0.16 x 10^(-9) (M^2/N) Wood
	0.000000001f, // 1.0  x 10^(-8) (M^2/N) Leather
	0.000000002f, // 0.2  x 10^(-7) (M^2/N) Tendon
	0.0000001f, // 1.0  x 10^(-6) (M^2/N) Rubber
	0.00002f, // 0.2  x 10^(-3) (M^2/N) Muscle
	0.0001f, // 1.0  x 10^(-3) (M^2/N) Fat
};

// XPBD bone constraints: solve distance constraints between pairs of bones.
// Uses Extended Position-Based Dynamics (XPBD) with compliance-based stiffness.
// Compliance values are material-dependent (Concrete < Wood < Leather < Rubber < Muscle < Fat).
// Two passes: before collision (stabilize structure) and after collision (restore distances).
void FAnimNode_KawaiiPhysics::AdjustByBoneConstraints()
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_AdjustByBoneConstraint);

	for (FModifyBoneConstraint& BoneConstraint : MergedBoneConstraints)
	{
		// IsValid()はLength>0のみ確認するため、indexの範囲も明示的に検証（堅牢化）
		// IsValid() only checks Length>0; validate indices explicitly to avoid OOB access (hardening)
		if (!BoneConstraint.IsValid() ||
			!ModifyBones.IsValidIndex(BoneConstraint.ModifyBoneIndex1) ||
			!ModifyBones.IsValidIndex(BoneConstraint.ModifyBoneIndex2))
		{
			continue;
		}

		FKawaiiPhysicsModifyBone& ModifyBone1 = ModifyBones[BoneConstraint.ModifyBoneIndex1];
		FKawaiiPhysicsModifyBone& ModifyBone2 = ModifyBones[BoneConstraint.ModifyBoneIndex2];
		EXPBDComplianceType ComplianceType = BoneConstraint.bOverrideCompliance
			                                     ? BoneConstraint.ComplianceType
			                                     : BoneConstraintGlobalComplianceType;

		FVector Delta = ModifyBone2.Location - ModifyBone1.Location;
		float DeltaLength = Delta.Size();
		if (DeltaLength < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// PBD
		// Delta *= (DeltaLength - BoneConstraint.Length) / DeltaLength * 0.5f;
		// ModifyBone1.Location += Delta * Stiffness;
		// ModifyBone2.Location -= Delta * Stiffness;

		// XBPD
		float Constraint = DeltaLength - BoneConstraint.Length;
		const int32 ComplianceIndex = static_cast<int32>(ComplianceType);
		float Compliance = (ComplianceIndex >= 0 && ComplianceIndex < XPBDComplianceValues.Num())
			? XPBDComplianceValues[ComplianceIndex] : 0.001f;
		const float StepDt = GetStepDeltaTime();
		if (StepDt > SMALL_NUMBER)
		{
			Compliance /= (StepDt * StepDt);
		}
		else
		{
			Compliance = 0.001f;
		}
		float DeltaLambda = (Constraint - Compliance * BoneConstraint.Lambda) / (2 + Compliance); // 2 = SumMass
		Delta = (Delta / DeltaLength) * DeltaLambda;

		ModifyBone1.Location += Delta;
		ModifyBone2.Location -= Delta;
		BoneConstraint.Lambda += DeltaLambda;
	}
}

// Heuristic solve order: sort constraints from leaves to root for faster XPBD convergence.
// Constraints involving bones with fewer children (leaves) are solved first.
void FAnimNode_KawaiiPhysics::SortConstraintsHeuristic()
{
	if (!bHeuristicSolveOrder || MergedBoneConstraints.Num() < 2)
	{
		return;
	}

	// Compute depth for each bone (distance from root)
	TMap<int32, int32> BoneDepth;
	for (int32 i = 0; i < ModifyBones.Num(); ++i)
	{
		int32 Depth = 0;
		int32 Current = i;
		while (ModifyBones.IsValidIndex(Current) && ModifyBones[Current].ParentIndex >= 0
		       && ModifyBones[Current].ParentIndex != Current)
		{
			Depth++;
			Current = ModifyBones[Current].ParentIndex;
			if (Depth > ModifyBones.Num()) break; // cycle guard
		}
		BoneDepth.Add(i, Depth);
	}

	// Sort constraints: deepest bone first (leaves to root)
	MergedBoneConstraints.Sort([&BoneDepth](const FModifyBoneConstraint& A, const FModifyBoneConstraint& B)
	{
		const int32 DepthA = BoneDepth.FindRef(A.ModifyBoneIndex1);
		const int32 DepthB = BoneDepth.FindRef(B.ModifyBoneIndex1);
		return DepthA > DepthB; // deeper bones first
	});
}

void FAnimNode_KawaiiPhysics::InitBoneConstraints()
{
	MergedBoneConstraints = BoneConstraints;
	MergedBoneConstraints.Append(BoneConstraintsData);

	TArray<FModifyBoneConstraint> DummyBoneConstraint;
	for (FModifyBoneConstraint& Constraint : MergedBoneConstraints)
	{
		Constraint.ModifyBoneIndex1 =
			ModifyBones.IndexOfByPredicate([Constraint](const FKawaiiPhysicsModifyBone& ModifyBone)
			{
				return ModifyBone.BoneRef == Constraint.Bone1;
			});
		if (Constraint.ModifyBoneIndex1 < 0)
		{
			continue;
		}

		Constraint.ModifyBoneIndex2 =
			ModifyBones.IndexOfByPredicate([Constraint](const FKawaiiPhysicsModifyBone& ModifyBone)
			{
				return ModifyBone.BoneRef == Constraint.Bone2;
			});
		if (Constraint.ModifyBoneIndex2 < 0)
		{
			continue;
		}

		Constraint.Length =
			(ModifyBones[Constraint.ModifyBoneIndex1].Location - ModifyBones[Constraint.ModifyBoneIndex2].Location).
			Size();

		// DummyBone's constraint
		if (bAutoAddChildDummyBoneConstraint)
		{
			// tip dummy constraint（inter-bone dummyを除外）
			const int32 ChildDummyBoneIndex1 = ModifyBones[Constraint.ModifyBoneIndex1].ChildIndices.IndexOfByPredicate(
				[&](int32 Index)
				{
					return Index >= 0 && ModifyBones[Index].bDummy && !ModifyBones[Index].bInterBoneDummy;
				});
			const int32 ChildDummyBoneIndex2 = ModifyBones[Constraint.ModifyBoneIndex2].ChildIndices.IndexOfByPredicate(
				[&](int32 Index)
				{
					return Index >= 0 && ModifyBones[Index].bDummy && !ModifyBones[Index].bInterBoneDummy;
				});

			if (ChildDummyBoneIndex1 >= 0 && ChildDummyBoneIndex2 >= 0)
			{
				FModifyBoneConstraint NewDummyBoneConstraint;
				NewDummyBoneConstraint.ModifyBoneIndex1 = ModifyBones[Constraint.ModifyBoneIndex1].ChildIndices[
					ChildDummyBoneIndex1];
				NewDummyBoneConstraint.ModifyBoneIndex2 = ModifyBones[Constraint.ModifyBoneIndex2].ChildIndices[
					ChildDummyBoneIndex2];
				NewDummyBoneConstraint.Length =
					(ModifyBones[NewDummyBoneConstraint.ModifyBoneIndex1].Location - ModifyBones[NewDummyBoneConstraint.
						ModifyBoneIndex2].Location).
					Size();
				NewDummyBoneConstraint.bIsDummy = true;
				// 細分化の除外設定のみ継承（complianceは既存挙動を変えないため継承しない）
				// Inherit only the subdivision opt-out (NOT compliance — keep existing behavior byte-identical)
				NewDummyBoneConstraint.bExcludeFromSubdivision = Constraint.bExcludeFromSubdivision;
				DummyBoneConstraint.Add(NewDummyBoneConstraint);
			}

			// inter-bone dummy間の横方向Constraint自動生成
			// Auto-generate lateral constraints between inter-bone dummies of adjacent chains
			auto CollectInterBoneDummies = [&](int32 BoneIdx) -> TArray<int32>
			{
				TArray<int32> Dummies;
				for (const int32 ChildIdx : ModifyBones[BoneIdx].ChildIndices)
				{
					if (ChildIdx >= 0 && ModifyBones[ChildIdx].bInterBoneDummy)
					{
						int32 Idx = ChildIdx;
						while (Idx >= 0 && ModifyBones[Idx].bInterBoneDummy)
						{
							Dummies.Add(Idx);
							int32 NextIdx = -1;
							for (const int32 CI : ModifyBones[Idx].ChildIndices)
							{
								if (CI >= 0 && ModifyBones[CI].bInterBoneDummy)
								{
									NextIdx = CI;
									break;
								}
							}
							Idx = NextIdx;
						}

						// 末端区間が分割されている場合、チェーン末尾の tip dummy も横方向ペア対象に含める
						// （tip dummy は ID_N の後ろに移動し直接子探索では見つからないため）
						// If the terminal segment is subdivided, also include the chain-tail tip dummy in the
						// lateral pairing (it now sits behind ID_N and isn't found by the direct-child search)
						if (Dummies.Num() > 0)
						{
							const int32 LastDummy = Dummies.Last();
							for (const int32 CI : ModifyBones[LastDummy].ChildIndices)
							{
								if (CI >= 0 && ModifyBones[CI].bDummy && !ModifyBones[CI].bInterBoneDummy)
								{
									Dummies.Add(CI);
									break;
								}
							}
						}
						break;
					}
				}
				return Dummies;
			};

			const TArray<int32> Dummies1 = CollectInterBoneDummies(Constraint.ModifyBoneIndex1);
			const TArray<int32> Dummies2 = CollectInterBoneDummies(Constraint.ModifyBoneIndex2);
			const int32 PairCount = FMath::Min(Dummies1.Num(), Dummies2.Num());

			for (int32 k = 0; k < PairCount; k++)
			{
				FModifyBoneConstraint NewConstraint;
				NewConstraint.ModifyBoneIndex1 = Dummies1[k];
				NewConstraint.ModifyBoneIndex2 = Dummies2[k];
				NewConstraint.Length =
					(ModifyBones[Dummies1[k]].Location - ModifyBones[Dummies2[k]].Location).Size();
				NewConstraint.bIsDummy = true;
				// 細分化の除外設定のみ継承（complianceは既存挙動を変えないため継承しない）
				// Inherit only the subdivision opt-out (NOT compliance — keep existing behavior byte-identical)
				NewConstraint.bExcludeFromSubdivision = Constraint.bExcludeFromSubdivision;
				DummyBoneConstraint.Add(NewConstraint);
			}
		}
	}

	MergedBoneConstraints.Append(DummyBoneConstraint);

	// 横方向Constraintに沿ってbridge dummy（コリジョンセンサー）を挿入。元Constraintは温存（置換しない）
	// Insert bridge collision-SENSOR dummies along horizontal constraints (original constraints are kept intact;
	// feedback to real bones is the per-frame direct displacement transfer in SimulateModifyBones)
	InsertBridgeDummiesForConstraints();
}

void FAnimNode_KawaiiPhysics::InsertBridgeDummiesForConstraints()
{
	if (BoneConstraintSubdivisionCount <= 0)
	{
		return;
	}


	const FRichCurve* RadiusCurve = RadiusCurveData.GetRichCurveConst();

	// 元のMergedBoneConstraintsは置換せず温存する（列間隔の剛性を維持）。
	// ここではコリジョンセンサーとなる bridge dummy を ModifyBones に追加するだけ。
	// 実ボーンへのフィードバックは毎フレームの「直接変位転送パス」(SimulateModifyBones) が行う。
	// Keep the original constraints intact (preserves column spacing). Here we only add bridge collision-SENSOR
	// dummies to ModifyBones; feedback to the real bones is done per-frame by the direct displacement-transfer pass.
	// MergedBoneConstraints is not modified, so the range-for is safe even though ModifyBones grows.
	for (const FModifyBoneConstraint& Constraint : MergedBoneConstraints)
	{
		if (Constraint.bExcludeFromSubdivision)
		{
			continue;
		}
		if (!Constraint.IsBoneReferenceValid() ||
			!ModifyBones.IsValidIndex(Constraint.ModifyBoneIndex1) ||
			!ModifyBones.IsValidIndex(Constraint.ModifyBoneIndex2))
		{
			continue;
		}

		const int32 I1 = Constraint.ModifyBoneIndex1;
		const int32 I2 = Constraint.ModifyBoneIndex2;

		const FVector P1 = ModifyBones[I1].Location;
		const FVector P2 = ModifyBones[I2].Location;
		const float Dist = (P2 - P1).Size();

		// 端点ごとの実効Radiusを各端点のLengthRateでカーブ評価（テーパー対応。グローバル最大半径は使わない）。
		// Per-endpoint effective radius from the curve at each endpoint's LengthRate (taper-aware; not a global max).
		const float LR1 = ModifyBones[I1].LengthRateFromRoot;
		const float LR2 = ModifyBones[I2].LengthRateFromRoot;
		const float R1 = PhysicsSettings.Radius * FMath::Max(RadiusCurve->Eval(LR1, 1.0f), 0.0f);
		const float R2 = PhysicsSettings.Radius * FMath::Max(RadiusCurve->Eval(LR2, 1.0f), 0.0f);

		// コリジョン被覆用: 端点スフィアが既に重なる(Dist<=R1+R2)なら隙間が無いのでセンサー不要。
		// それ以外は指定数をそのまま使う（被覆には重なりが必要なので縦のCalcInterBoneDummyCountは使わない）。
		// Coverage: if the endpoint spheres already overlap (Dist <= R1+R2) there is no gap -> no sensors.
		// Otherwise use the requested count directly (coverage needs overlap; not CalcInterBoneDummyCount).
		if (Dist <= FMath::Max(R1 + R2, KINDA_SMALL_NUMBER))
		{
			continue;
		}

		const int32 N = BoneConstraintSubdivisionCount;
		const FQuat Q1 = ModifyBones[I1].PrevRotation;
		const FQuat Q2 = ModifyBones[I2].PrevRotation;
		const FVector ScaleA = ModifyBones[I1].PoseScale;
		const FVector ScaleB = ModifyBones[I2].PoseScale;
		const auto BaseSettings = ModifyBones[I1].PhysicsSettings;

		for (int32 k = 0; k < N; ++k)
		{
			const float LerpAlpha = static_cast<float>(k + 1) / static_cast<float>(N + 1);

			FKawaiiPhysicsModifyBone BridgeDummy;
			BridgeDummy.bDummy = true;
			BridgeDummy.bBridgeDummy = true;
			// 配置用に InterBone* フィールドを端点1/端点2/補間率として流用 / Reuse InterBone* fields for placement & feedback
			BridgeDummy.InterBoneRealParentIndex = I1;
			BridgeDummy.InterBoneRealChildIndex = I2;
			BridgeDummy.InterBoneAlpha = LerpAlpha;
			BridgeDummy.Location = FMath::Lerp(P1, P2, LerpAlpha);
			BridgeDummy.PrevLocation = BridgeDummy.Location;
			BridgeDummy.PoseLocation = BridgeDummy.Location;
			BridgeDummy.PrevRotation = FQuat::Slerp(Q1, Q2, LerpAlpha);
			BridgeDummy.PoseRotation = BridgeDummy.PrevRotation;
			BridgeDummy.PoseScale = FMath::Lerp(ScaleA, ScaleB, LerpAlpha);
			BridgeDummy.ParentIndex = -1; // 縦階層に属さない / not part of the vertical hierarchy
			BridgeDummy.BoneLength = Dist / (N + 1);
			// LengthRateは端点平均（毎フレームのUpdatePhysicsSettingsがこれを基にRadius等を再計算するため必須）
			// LengthRate = average of endpoints (per-frame UpdatePhysicsSettings derives Radius from it — required)
			BridgeDummy.LengthRateFromRoot = 0.5f * (LR1 + LR2);
			BridgeDummy.PhysicsSettings = BaseSettings;
			BridgeDummy.PhysicsSettings.Radius = 0.5f * (R1 + R2);

			const int32 Idx = ModifyBones.Add(BridgeDummy);
			ModifyBones[Idx].Index = Idx;
		}
	}
}

// -------------------------------------------------------------------
// Shared Collision
// -------------------------------------------------------------------

void FAnimNode_KawaiiPhysics::InitializeSharedCollision(const UAnimInstance* InAnimInstance)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_InitializeSharedCollision);
	if (bSharedCollisionInitialized)
	{
		return;
	}

	const USkeletalMeshComponent* SkelComp = InAnimInstance->GetSkelMeshComponent();
	if (!SkelComp)
	{
		return;
	}

	const UWorld* World = InAnimInstance->GetWorld();
	if (!World)
	{
		return;
	}

	UKawaiiPhysicsSharedCollisionSubsystem* Subsystem = World->GetSubsystem<UKawaiiPhysicsSharedCollisionSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	AActor* OwnerActor = SkelComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (bSharedCollisionSource)
	{
		CachedSharedCollisionEntry = Subsystem->FindOrCreateEntry(OwnerActor, SharedCollisionGroupTag);
		if (CachedSharedCollisionEntry.IsValid())
		{
			const uint64 SourceID = static_cast<uint64>(reinterpret_cast<uintptr_t>(this));
			CachedSourceSlot = CachedSharedCollisionEntry->GetOrCreateSlot(SourceID);
		}
	}

	if (bUseSharedCollision && !bSharedCollisionSource)
	{
		if (!CachedSharedCollisionEntry.IsValid())
		{
			CachedSharedCollisionEntry = Subsystem->FindEntry(OwnerActor, SharedCollisionGroupTag);
		}
	}

	// Targetの場合、Entry取得成功時のみ初期化完了（未取得時は次フレームでリトライ）
	// For targets: only mark initialized if entry was found (retry next frame otherwise)
	if (!bUseSharedCollision || bSharedCollisionSource || CachedSharedCollisionEntry.IsValid())
	{
		bSharedCollisionInitialized = true;
	}
}

void FAnimNode_KawaiiPhysics::WriteSharedCollisionToSubsystem(
	FComponentSpacePoseContext& Output, const FTransform& ComponentTransform)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_WriteSharedCollisionToSubsystem);
	if (!CachedSourceSlot.IsValid())
	{
		return;
	}

	FKawaiiPhysicsSharedCollisionData Data;

	// 汎用ヘルパー: 有効なコリジョンをシミュレーション空間→ワールド空間に変換して収集
	// Generic helper: collect enabled collision limits and convert from simulation space to world space
	auto ConvertAndAppend = [&](const auto& InLimits, auto& OutLimits, auto PostConvert)
	{
		for (const auto& Limit : InLimits)
		{
			if (!Limit.bEnable)
			{
				continue;
			}
			auto Converted = Limit;
			const FTransform SimTransform(Limit.Rotation, Limit.Location);
			const FTransform WorldTransform = ConvertSimulationSpaceTransform(
				Output, SimulationSpace, EKawaiiPhysicsSimulationSpace::WorldSpace, SimTransform);
			Converted.Location = WorldTransform.GetLocation();
			Converted.Rotation = WorldTransform.GetRotation();
			PostConvert(Converted, WorldTransform);
			OutLimits.Add(Converted);
		}
	};

	auto NoOp = [](auto&, const FTransform&) {};
	auto RecomputePlane = [](FPlanarLimit& L, const FTransform& T)
	{
		L.Plane = FPlane(L.Location, T.GetRotation().GetUpVector());
	};

	// 全コリジョンソースを収集 / Collect from all collision sources
	ConvertAndAppend(SphericalLimits,     Data.SphericalLimits, NoOp);
	ConvertAndAppend(SphericalLimitsData, Data.SphericalLimits, NoOp);
	ConvertAndAppend(CapsuleLimits,       Data.CapsuleLimits,   NoOp);
	ConvertAndAppend(CapsuleLimitsData,   Data.CapsuleLimits,   NoOp);
	ConvertAndAppend(BoxLimits,           Data.BoxLimits,        NoOp);
	ConvertAndAppend(BoxLimitsData,       Data.BoxLimits,        NoOp);
	ConvertAndAppend(PlanarLimits,        Data.PlanarLimits,     RecomputePlane);
	ConvertAndAppend(PlanarLimitsData,    Data.PlanarLimits,     RecomputePlane);

	CachedSourceSlot->Publish(Data);
}

void FAnimNode_KawaiiPhysics::UpdateSharedCollisionLimits(
	FComponentSpacePoseContext& Output)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_UpdateSharedCollisionLimits);
	SharedSphericalLimits.Reset();
	SharedCapsuleLimits.Reset();
	SharedBoxLimits.Reset();
	SharedPlanarLimits.Reset();

	if (!CachedSharedCollisionEntry.IsValid())
	{
		return;
	}

	CachedSharedCollisionEntry->ReadMerged(SharedCollisionMergedData);

	if (SharedCollisionMergedData.IsEmpty())
	{
		return;
	}

	// 汎用ヘルパー: ワールド空間→シミュレーション空間に変換して格納
	// Generic helper: convert from world space to simulation space and store
	auto ConvertAndStore = [&](const auto& InLimits, auto& OutLimits, auto PostConvert)
	{
		OutLimits.Reserve(InLimits.Num());
		for (const auto& Limit : InLimits)
		{
			auto Converted = Limit;
			const FTransform WorldTransform(Limit.Rotation, Limit.Location);
			const FTransform SimTransform = ConvertSimulationSpaceTransform(
				Output, EKawaiiPhysicsSimulationSpace::WorldSpace, SimulationSpace, WorldTransform);
			Converted.Location = SimTransform.GetLocation();
			Converted.Rotation = SimTransform.GetRotation();
			Converted.bEnable = true;
			PostConvert(Converted, SimTransform);
			OutLimits.Add(Converted);
		}
	};

	auto NoOp = [](auto&, const FTransform&) {};
	auto RecomputePlane = [](FPlanarLimit& L, const FTransform& T)
	{
		L.Plane = FPlane(L.Location, T.GetRotation().GetUpVector());
	};

	ConvertAndStore(SharedCollisionMergedData.SphericalLimits, SharedSphericalLimits, NoOp);
	ConvertAndStore(SharedCollisionMergedData.CapsuleLimits,   SharedCapsuleLimits,   NoOp);
	ConvertAndStore(SharedCollisionMergedData.BoxLimits,       SharedBoxLimits,        NoOp);
	ConvertAndStore(SharedCollisionMergedData.PlanarLimits,    SharedPlanarLimits,     RecomputePlane);
}

// Mesh-based collision using simplified convex hull approach.
// For each bone: compute centroid-to-vertex face normals, check if bone is inside hull,
// push out along the nearest face normal if inside. Uses bounding box corners as hull vertices.
// This is NOT a triangle mesh sweep — it uses pre-computed convex hull vertices
// and checks against each face plane. Performance: O(n * h) where n = bones, h = hull vertices.
void FAnimNode_KawaiiPhysics::AdjustByMeshCollision(FKawaiiPhysicsModifyBone& Bone,
                                                     TArray<FKawaiiPhysicsMeshCollision>& Collisions,
                                                     FComponentSpacePoseContext& Output)
{
	if (Collisions.Num() == 0)
	{
		return;
	}

	const float BoneRadius = Bone.PhysicsSettings.Radius;

	for (FKawaiiPhysicsMeshCollision& MeshCol : Collisions)
	{
		if (!MeshCol.bEnable)
		{
			continue;
		}

		// Get driving bone transform
		FTransform MeshTransform = MeshCol.MeshTransform;
		if (MeshCol.DrivingBone.BoneIndex >= 0)
		{
			const int32 BoneIndex = MeshCol.DrivingBone.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()).GetInt();
			if (BoneIndex >= 0)
			{
				const FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(BoneIndex));
				MeshTransform = BoneTransform * MeshCol.MeshTransform;
			}
		}

		// Build convex hull from vertices or use sphere fallback
		TArray<FVector> HullVertices;

		if (MeshCol.MeshSource == EMeshCollisionSourceType::ConvexHull && MeshCol.ConvexHullVertices.Num() >= 4)
		{
			// Use provided convex hull vertices
			HullVertices.Reserve(MeshCol.ConvexHullVertices.Num());
			for (const FVector& V : MeshCol.ConvexHullVertices)
			{
				HullVertices.Add(MeshTransform.TransformPosition(V * MeshCol.MeshScale));
			}
		}
		else if (MeshCol.MeshSource == EMeshCollisionSourceType::StaticMesh && MeshCol.CollisionMesh)
		{
			// Get convex hull from static mesh
			// Use simplified approach: get bounding hull from render data
			const FStaticMeshRenderData* RenderData = MeshCol.CollisionMesh->GetRenderData();
			if (RenderData && RenderData->LODResources.Num() > 0)
			{
				const FStaticMeshLODResources& LODResource = RenderData->LODResources[0];
				const FPositionVertexBuffer& VertexBuffer = LODResource.VertexBuffers.PositionVertexBuffer;
				const int32 NumVerts = VertexBuffer.GetNumVertices();

				if (NumVerts > 0)
				{
					// Compute bounding box from vertex positions
					FVector FirstPos = FVector(VertexBuffer.VertexPosition(0)) * MeshCol.MeshScale;
					FBox Bounds(FirstPos, FirstPos);
					for (int32 i = 1; i < NumVerts; ++i)
					{
						FVector VertPosF = FVector(VertexBuffer.VertexPosition(i)) * MeshCol.MeshScale;
						Bounds += VertPosF;
					}

					// Transform bounds to world space
					Bounds = Bounds.TransformBy(MeshTransform);

					// Use bounding box corners as simplified convex hull
					const FVector Center = Bounds.GetCenter();
					const FVector Extent = Bounds.GetExtent();

					HullVertices.Reserve(8);
					HullVertices.Add(Center + FVector(Extent.X, Extent.Y, Extent.Z));
					HullVertices.Add(Center + FVector(Extent.X, Extent.Y, -Extent.Z));
					HullVertices.Add(Center + FVector(Extent.X, -Extent.Y, Extent.Z));
					HullVertices.Add(Center + FVector(Extent.X, -Extent.Y, -Extent.Z));
					HullVertices.Add(Center + FVector(-Extent.X, Extent.Y, Extent.Z));
					HullVertices.Add(Center + FVector(-Extent.X, Extent.Y, -Extent.Z));
					HullVertices.Add(Center + FVector(-Extent.X, -Extent.Y, Extent.Z));
					HullVertices.Add(Center + FVector(-Extent.X, -Extent.Y, -Extent.Z));
					HullVertices.Add(Center); // Center point
				}
			}
		}
		else if (MeshCol.MeshSource == EMeshCollisionSourceType::PhysicsAsset && MeshCol.PhysicsAsset)
		{
			// Use physics body from physics asset
			for (const TSoftObjectPtr<UBodySetup>& BodySetupPtr : MeshCol.PhysicsAsset->SkeletalBodySetups)
			{
				const UBodySetup* BodySetup = BodySetupPtr.LoadSynchronous();
				if (BodySetup && BodySetup->AggGeom.ConvexElems.Num() > 0)
				{
					for (const FKConvexElem& ConvexElem : BodySetup->AggGeom.ConvexElems)
					{
						if (ConvexElem.VertexData.Num() >= 4)
						{
							for (const FVector& V : ConvexElem.VertexData)
							{
								HullVertices.Add(MeshTransform.TransformPosition(V * MeshCol.MeshScale));
							}
						}
					}
				}
			}
		}

		if (HullVertices.Num() < 4)
		{
			continue;
		}

		// Simple convex hull collision: find closest point on hull to bone and push out
		// This is a simplified GJK-like approach for real-time performance
		FVector ClosestPoint = HullVertices[0];
		float MinDistSq = FVector::DistSquared(Bone.Location, ClosestPoint);

		for (const FVector& V : HullVertices)
		{
			const float DistSq = FVector::DistSquared(Bone.Location, V);
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				ClosestPoint = V;
			}
		}

		// Check if bone is inside convex hull (simple approach: check against multiple planes)
		bool bInsideHull = true;
		FVector CollisionNormal = FVector::ZeroVector;

		// Compute centroid of hull for face normal estimation
		FVector Centroid = FVector::ZeroVector;
		for (const FVector& V : HullVertices)
		{
			Centroid += V;
		}
		if (HullVertices.Num() > 0)
		{
			Centroid /= HullVertices.Num();
		}

		// Check against each face of the simplified convex hull
		// Use centroid-to-vertex direction as face normals (each vertex is a "corner face" of the hull)
		for (int32 i = 0; i < HullVertices.Num() && bInsideHull; ++i)
		{
			const FVector FaceNormal = (HullVertices[i] - Centroid).GetSafeNormal();
			if (FaceNormal.IsNearlyZero())
			{
				continue;
			}

			// Signed distance from the plane passing through hull center along this normal
			const float DistToPlane = FVector::DotProduct(Bone.Location - Centroid, FaceNormal);

			// If bone is beyond the face plane by more than its radius, it's outside
			if (DistToPlane > -BoneRadius)
			{
				bInsideHull = false;
			}
		}

		// If bone is inside hull, push to nearest face
		if (bInsideHull)
		{
			float MinPushDist = MAX_FLT;
			FVector BestNormal = FVector::ZeroVector;

			for (int32 i = 0; i < HullVertices.Num(); ++i)
			{
				const FVector FaceNormal = (HullVertices[i] - Centroid).GetSafeNormal();
				if (FaceNormal.IsNearlyZero())
				{
					continue;
				}

				// Distance from bone to the face plane (negative = inside)
				const float DistToPlane = FVector::DotProduct(Bone.Location - Centroid, FaceNormal);
				// Push distance needed: move bone to the face plane + BoneRadius outside
				const float PushDist = BoneRadius + DistToPlane;

				if (PushDist < MinPushDist)
				{
					MinPushDist = PushDist;
					BestNormal = FaceNormal;
				}
			}

			if (!BestNormal.IsNearlyZero() && MinPushDist > 0)
			{
				Bone.Location += BestNormal * MinPushDist;
				CollisionNormal = BestNormal;
			}
		}

		// Record collision event
		if (!CollisionNormal.IsNearlyZero())
		{
			RecordBoneCollisionEvent(Bone.BoneRef.BoneName, Bone.Location, CollisionNormal);
		}
	}
}
