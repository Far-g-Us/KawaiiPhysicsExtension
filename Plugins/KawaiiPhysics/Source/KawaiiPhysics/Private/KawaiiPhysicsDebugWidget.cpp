// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "KawaiiPhysicsDebugWidget.h"
#include "KawaiiPhysicsLibrary.h"

void UKawaiiPhysicsDebugWidget::SetKawaiiPhysicsNode(const FKawaiiPhysicsReference& InKawaiiPhysics)
{
	KawaiiPhysicsNode = InKawaiiPhysics;
	bIsActive = true;
}

void UKawaiiPhysicsDebugWidget::GetDebugData(TArray<FString>& OutBoneNames, TArray<float>& OutDamping,
                                             TArray<float>& OutStiffness, TArray<float>& OutVelocities,
                                             int32& OutBoneCount, int32& OutCollisionCount,
                                             float& OutSimTimeMs) const
{
	OutBoneNames = CachedBoneNames;
	OutDamping = CachedDamping;
	OutStiffness = CachedStiffness;
	OutVelocities = CachedVelocities;
	OutBoneCount = CachedBoneCount;
	OutCollisionCount = CachedCollisionCount;
	OutSimTimeMs = CachedSimTimeMs;
}

void UKawaiiPhysicsDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsActive)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastRefreshTime < RefreshRate)
	{
		return;
	}
	LastRefreshTime = CurrentTime;

	// Refresh cached data from the AnimNode
	KawaiiPhysicsNode.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
		TEXT("DebugWidgetTick"),
		[this](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
			CachedBoneNames.Empty();
			CachedDamping.Empty();
			CachedStiffness.Empty();
			CachedVelocities.Empty();
			CachedBoneCount = 0;
			CachedCollisionCount = 0;

			for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
			{
				if (Bone.bDummy || Bone.bBridgeDummy)
				{
					continue;
				}

				CachedBoneNames.Add(Bone.BoneRef.BoneName.ToString());
				CachedDamping.Add(Bone.PhysicsSettings.Damping);
				CachedStiffness.Add(Bone.PhysicsSettings.Stiffness);

				const float Velocity = (Bone.Location - Bone.PrevLocation).Size()
					/ FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);
				CachedVelocities.Add(Velocity);
				CachedBoneCount++;
			}

			// Count active collisions
			CachedCollisionCount = InKawaiiPhysics.SphericalLimits.Num()
				+ InKawaiiPhysics.CapsuleLimits.Num()
				+ InKawaiiPhysics.BoxLimits.Num();

			CachedSimTimeMs = InKawaiiPhysics.DeltaTime * 1000.0f;
		});
}
