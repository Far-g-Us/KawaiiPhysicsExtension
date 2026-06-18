// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "AnimNode_KawaiiPhysics.h"

#include "AnimationRuntime.h"
#include "HAL/PlatformTime.h"
#include "KawaiiPhysicsBoneConstraintsDataAsset.h"
#include "KawaiiPhysicsCustomExternalForce.h"
#include "KawaiiPhysicsDeveloperSettings.h"
#include "ExternalForces/KawaiiPhysicsExternalForce.h"
#include "KawaiiPhysicsLimitsDataAsset.h"
#include "KawaiiPhysicsSharedCollisionSubsystem.h"
#include "Animation/AnimInstanceProxy.h"
#include "Curves/CurveFloat.h"
#include "Runtime/Launch/Resources/Version.h"

// Spatial hash grid for O(n) self-collision broadphase
struct FKawaiiSpatialGrid
{
	float CellSize;
	float InvCellSize;
	TMap<int32, TArray<int32>> Cells;

	void Init(float InCellSize)
	{
		CellSize = FMath::Max(InCellSize, KINDA_SMALL_NUMBER);
		InvCellSize = 1.0f / CellSize;
		Cells.Empty(64);
	}

	int32 HashCell(int32 X, int32 Y, int32 Z) const
	{
		// Cantor-like pairing for negative coordinates
		return ((X * 73856093) ^ (Y * 19349663) ^ (Z * 83492791)) & 0x7FFFFFFF;
	}

	int32 GetCellIndex(const FVector& Pos) const
	{
		return HashCell(
			FMath::FloorToInt(Pos.X * InvCellSize),
			FMath::FloorToInt(Pos.Y * InvCellSize),
			FMath::FloorToInt(Pos.Z * InvCellSize));
	}

	void Insert(int32 BoneIndex, const FVector& Pos)
	{
		Cells.FindOrAdd(GetCellIndex(Pos)).Add(BoneIndex);
	}

	void FindNearby(const FVector& Pos, float Radius, TArray<int32>& OutIndices) const
	{
		const int32 MinX = FMath::FloorToInt((Pos.X - Radius) * InvCellSize);
		const int32 MaxX = FMath::FloorToInt((Pos.X + Radius) * InvCellSize);
		const int32 MinY = FMath::FloorToInt((Pos.Y - Radius) * InvCellSize);
		const int32 MaxY = FMath::FloorToInt((Pos.Y + Radius) * InvCellSize);
		const int32 MinZ = FMath::FloorToInt((Pos.Z - Radius) * InvCellSize);
		const int32 MaxZ = FMath::FloorToInt((Pos.Z + Radius) * InvCellSize);

		for (int32 X = MinX; X <= MaxX; ++X)
		{
			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				for (int32 Z = MinZ; Z <= MaxZ; ++Z)
				{
					const int32 Key = HashCell(X, Y, Z);
					if (const TArray<int32>* Cell = Cells.Find(Key))
					{
						OutIndices.Append(*Cell);
					}
				}
			}
		}
	}
};
#include "SceneInterface.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsSettings.h"

static constexpr float PoseDetectionStandingThreshold = 0.7f;
static constexpr float PoseDetectionSittingThreshold = 0.3f;
static constexpr float SpringVolumeMinScale = 0.5f;
static constexpr float SpringVolumeMaxScale = 1.5f;
static constexpr float StiffnessNormalizationScale = 100.0f;
static constexpr float DampingNormalizationScale = 10.0f;
static constexpr float ClothConstraintForceScale = 0.1f;
static constexpr float MorphTargetSpeedNormalizationScale = 100.0f;

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
#include "KawaiiPhysicsGPUCompute.h"
#include "AnimNode_KawaiiPhysicsInternal.h"
#include "Engine/SkeletalMesh.h"

void FAnimNode_KawaiiPhysics::UpdatePhysicsSettingsOfModifyBones()
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_UpdatePhysicsSetting);

	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		const float LengthRate = Bone.LengthRateFromRoot;

		// Damping
		Bone.PhysicsSettings.Damping = FMath::Clamp(
			PhysicsSettings.Damping * DampingCurveData.GetRichCurveConst()->Eval(
				LengthRate, 1.0f), 0.0f, 1.0f);
		
		// WorldLocationDamping
		Bone.PhysicsSettings.WorldDampingLocation = FMath::Clamp(
			PhysicsSettings.WorldDampingLocation * WorldDampingLocationCurveData.GetRichCurveConst()->Eval(
				LengthRate, 1.0f), 0.0f, 1.0f);
		
		// WorldRotationDamping
		Bone.PhysicsSettings.WorldDampingRotation = FMath::Clamp(
			PhysicsSettings.WorldDampingRotation * WorldDampingRotationCurveData.GetRichCurveConst()->Eval(
				LengthRate, 1.0f), 0.0f, 1.0f);
		
		// Stiffness
		Bone.PhysicsSettings.Stiffness = FMath::Clamp(
			PhysicsSettings.Stiffness * StiffnessCurveData.GetRichCurveConst()->Eval(
				LengthRate, 1.0f), 0.0f, 1.0f);
		
		// Radius
		Bone.PhysicsSettings.Radius = FMath::Max(
			PhysicsSettings.Radius * RadiusCurveData.GetRichCurveConst()->Eval(
				LengthRate, 1.0f), 0.0f);
		
		// LimitAngle
		Bone.PhysicsSettings.LimitAngle = FMath::Max(
			PhysicsSettings.LimitAngle * LimitAngleCurveData.GetRichCurveConst()->Eval(
				LengthRate, 1.0f), 0.0f);

		// Twist/Swing Limits
		if (PhysicsSettings.bUseTwistSwingLimits)
		{
			Bone.TwistLimitAngle = FMath::Max(
				PhysicsSettings.TwistLimitAngle * TwistLimitAngleCurveData.GetRichCurveConst()->Eval(
					LengthRate, 1.0f), 0.0f);
			Bone.SwingLimitYAngle = FMath::Max(
				PhysicsSettings.SwingLimitYAngle * SwingLimitYAngleCurveData.GetRichCurveConst()->Eval(
					LengthRate, 1.0f), 0.0f);
			Bone.SwingLimitZAngle = FMath::Max(
				PhysicsSettings.SwingLimitZAngle * SwingLimitZAngleCurveData.GetRichCurveConst()->Eval(
					LengthRate, 1.0f), 0.0f);
		}
		else
		{
			Bone.TwistLimitAngle = 0.0f;
			Bone.SwingLimitYAngle = 0.0f;
			Bone.SwingLimitZAngle = 0.0f;
		}
	}
}


void FAnimNode_KawaiiPhysics::SimulateModifyBones(FComponentSpacePoseContext& Output,
                                                  const FTransform& ComponentTransform)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_SimulateModifyBones);

	if (DeltaTime <= 0.0f)
	{
		return;
	}

	// Budget Allocator: skip simulation if called too frequently
	// BudgetMicroSeconds = minimum interval between simulations (e.g., 5000 = max 200 Hz)
	if (bEnableBudgetAllocator && BudgetMicroSeconds > 0.0f)
	{
		const double Now = FPlatformTime::Seconds() * 1000000.0;
		const double ElapsedSinceLastFrame = Now - LastFrameEndTime;
		if (LastFrameEndTime > 0 && ElapsedSinceLastFrame < BudgetMicroSeconds)
		{
			bBudgetSkippedLastFrame = true;
			return;
		}
		bBudgetSkippedLastFrame = false;
	}

	// Physics Sleep: skip simulation if all bones are nearly still
	if (bEnablePhysicsSleep && bIsAsleep)
	{
		// Wake-up: check if actor/component moved significantly (teleport, ragdoll, etc.)
		const FVector CurrentCompLocation = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetComponentLocation();
		const float CompMovement = FVector::Dist(CurrentCompLocation, CachedComponentLocation);
		if (CompMovement > SleepVelocityThreshold * 10.0f)
		{
			bIsAsleep = false;
			SleepFrameCounter = 0;
			CachedComponentLocation = CurrentCompLocation;
		}
		else
		{
			return;
		}
	}
	CachedComponentLocation = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetComponentLocation();

	// Camera frustum culling: skip simulation if character is outside camera view
	if (bEnableCameraFrustumCulling && !bCachedIsInFrustum)
	{
		return;
	}

	// GPU compute: create/destroy on flag change (runtime Blueprint toggle)
	if (bEnableGPUSimulation && !GPUCompute)
	{
		USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
		if (SkelComp)
		{
			GPUCompute = NewObject<UKawaiiPhysicsGPUCompute>(SkelComp);
		}
	}
	else if (!bEnableGPUSimulation && GPUCompute)
	{
		GPUCompute->Release();
		GPUCompute = nullptr;
	}

	CurrentGravityMultiplier = 1.0f;

	// このフレームの実dtを保持（サブステップ中は DeltaTime を FixedDt として扱うため別保存）
	// Keep this frame's real dt (DeltaTime is treated as FixedDt during substeps)
	FrameDeltaTime = DeltaTime;

	// Deterministic mode: accumulator-based fixed timestep for network consistency
	if (bDeterministicMode)
	{
		SimulationTimeAccumulator += DeltaTime;
		if (SimulationTimeAccumulator < DeterministicFixedDt)
		{
			// Not enough time accumulated — skip this frame's simulation
			FrameDeltaTime = 0.0f;
			DeltaTime = 0.0f;
		}
		else
		{
			// Use fixed timestep, consume accumulator
			FrameDeltaTime = DeterministicFixedDt;
			DeltaTime = DeterministicFixedDt;
			SimulationTimeAccumulator -= DeterministicFixedDt;
			// Clamp to prevent spiral of death
			SimulationTimeAccumulator = FMath::Min(SimulationTimeAccumulator, DeterministicFixedDt);
		}
	}

	const USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();

	// Save Prev/Pose Info , Check SkipSimulate
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		// bridge dummyは縦親(ParentIndex<0)を持たないが、コリジョン代理として非skipにする
		// （Simulate()/長さ復元は別途スキップ。下のParentIndex<0分岐に落ちると誤ってskip&Pose固定される）
		// Bridge dummies have no vertical parent but must stay non-skip so collision runs on them
		// (Simulate()/length-restore are skipped separately; the ParentIndex<0 branch below must not catch them)
		if (Bone.bBridgeDummy)
		{
			Bone.bSkipSimulate = false;
			continue;
		}

		// Use INDEX_NONE (which equals -1) for consistent bone index validation
		if (Bone.BoneRef.BoneIndex == INDEX_NONE && !Bone.bDummy)
		{
			Bone.bSkipSimulate = true;
			continue;
		}

		if (Bone.ParentIndex < 0)
		{
			// root: kinematic。実ポーズ追従（PrevLocation/Location更新）は SimulateOnce 内で、
			// サブステップ補間後のポーズに対して毎ステップ行う。
			// root: kinematic; the actual pose-follow (PrevLocation/Location update) happens inside
			// SimulateOnce each step, against the per-substep interpolated pose.
			Bone.bSkipSimulate = true;
			continue;
		}

		Bone.bSkipSimulate = false;
	}

	// Gravity is computed after ApplyPoseBlending so pose-based GravityMultiplier is applied

	// SimpleExternalForce: compute once in SimulationSpace (avoid per-bone conversions)
	if (!SimpleExternalForce.IsNearlyZero())
	{
		if (bUseWorldSpaceSimpleExternalForce)
		{
			SimpleExternalForceInSimSpace = ConvertSimulationSpaceVector(
				Output,
				EKawaiiPhysicsSimulationSpace::WorldSpace,
				SimulationSpace,
				SimpleExternalForce);
		}
		else
		{
			SimpleExternalForceInSimSpace = SimpleExternalForce;
		}
	}
	else
	{
		SimpleExternalForceInSimSpace = FVector::ZeroVector;
	}

	// External Force : PreApply
	// NOTE: if use foreach, you may get issue ( Array has changed during ranged-for iteration )
	for (int i = 0; i < CustomExternalForces.Num(); ++i)
	{
		if (CustomExternalForces[i])
		{
			CustomExternalForces[i]->PreApply(*this, SkelComp);
		}
	}
	for (int i = 0; i < ExternalForces.Num(); ++i)
	{
		if (ExternalForces[i].IsValid())
		{
			auto& Force = ExternalForces[i].GetMutable<FKawaiiPhysics_ExternalForce>();
			Force.PreApply(*this, Output);
		}
	}

	// ===== サブステップ設定キャッシュ & Scene 取得（毎フレーム1回） =====
	const UKawaiiPhysicsDeveloperSettings* KawaiiSettings = GetDefault<UKawaiiPhysicsDeveloperSettings>();
	bUseFixedSubsteppingCached = KawaiiSettings ? KawaiiSettings->bUseFixedSubstepping : false;
	MaxSubstepsCached = KawaiiSettings ? FMath::Max(1, KawaiiSettings->MaxSubsteps) : 1;

	const UWorld* World = SkelComp ? SkelComp->GetWorld() : nullptr;
	const FSceneInterface* Scene = World ? World->Scene : nullptr;

	if (!SkelComp)
	{
		UE_LOG(LogKawaiiPhysicsExtension, Warning, TEXT("SimulateModifyBones: SkelComp is null — world collision disabled"));
	}

	// 現フレームのポーズ目標をスナップショット（サブステップ中 PoseLocation を補間で上書きするため退避）。
	// 初回/リセット後は前フレーム値を現在値で初期化（補間で飛ばないように）。
	// Snapshot this frame's pose target (PoseLocation is overwritten by interpolation during substeps).
	// On first frame / after reset, seed the previous-frame target with the current value (avoid a jump).
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		Bone.CurrentPoseLocation = Bone.PoseLocation;
		Bone.CurrentPoseRotation = Bone.PoseRotation;
		if (!bSubstepPoseInitialized)
		{
			Bone.PrevPoseLocation = Bone.PoseLocation;
			Bone.PrevPoseRotation = Bone.PoseRotation;
		}
	}
	bSubstepPoseInitialized = true;

	if (!bUseFixedSubsteppingCached)
	{
		// ===== Legacy: 実フレーム時間で1ステップ（GetStepDeltaTime()==DeltaTime） =====
		bInSubstep = false;
		SimulateOnce(Output, ComponentTransform, Scene, SkelComp);
		DeltaTimeOld = DeltaTime;
	}
	else
	{
		// ===== 固定タイムステップ・サブステップ（§4） =====
		const float FixedDt = 1.0f / GetEffectiveTargetFramerate();
		SubstepAccumulator += FrameDeltaTime;
		// spiral of death 防止：超過分は破棄 / clamp to avoid spiral of death (drop excess time)
		SubstepAccumulator = FMath::Min(SubstepAccumulator, MaxSubstepsCached * FixedDt);
		const int32 NumSteps = FMath::FloorToInt(SubstepAccumulator / FixedDt);
		SubstepAccumulator = FMath::Fmod(SubstepAccumulator, FixedDt);

		// world移動を各サブステップへ分配（並進=線形分配、回転=増分割合のSlerp）。§7-D
		// Distribute world movement across substeps (translation linear, rotation incremental fraction).
		const float MoveFrac = (FrameDeltaTime > KINDA_SMALL_NUMBER) ? (FixedDt / FrameDeltaTime) : 1.0f;
		const FVector FullSkelCompMove = SkelCompMoveVector;
		const FQuat FullSkelCompRot = SkelCompMoveRotation;

		bInSubstep = true;
		StepDeltaTime = FixedDt;
		DeltaTimeOld = FixedDt;
		for (int32 SubstepIndex = 0; SubstepIndex < NumSteps; ++SubstepIndex)
		{
			// 注: ローカル名は基底クラスのメンバ Alpha（ブレンド係数）を隠さないよう SubstepAlpha とする
			// Note: named SubstepAlpha to avoid shadowing the base-class member Alpha (blend weight)
			const float SubstepAlpha = static_cast<float>(SubstepIndex + 1) / static_cast<float>(NumSteps);

			// ポーズ目標をサブステップ補間（§5）
			for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
			{
				Bone.PoseLocation = FMath::Lerp(Bone.PrevPoseLocation, Bone.CurrentPoseLocation, SubstepAlpha);
				Bone.PoseRotation = FQuat::Slerp(Bone.PrevPoseRotation, Bone.CurrentPoseRotation, SubstepAlpha).GetNormalized();
			}

			// world移動の分配分だけを適用させる（Simulateはメンバを参照するため一時設定）
			SkelCompMoveVector = FullSkelCompMove * MoveFrac;
			SkelCompMoveRotation = FQuat::Slerp(FQuat::Identity, FullSkelCompRot, MoveFrac).GetNormalized();

			SimulateOnce(Output, ComponentTransform, Scene, SkelComp);
		}
		bInSubstep = false;

		// world移動メンバを元へ（次フレームの UpdateSkelCompMove で再計算されるが安全のため）
		SkelCompMoveVector = FullSkelCompMove;
		SkelCompMoveRotation = FullSkelCompRot;

		// PoseLocation を現フレームの真値へ戻す（出力・次フレーム捕捉の整合）
		for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
		{
			Bone.PoseLocation = Bone.CurrentPoseLocation;
			Bone.PoseRotation = Bone.CurrentPoseRotation;
		}
	}

	// 次フレームのポーズ補間用に現フレーム値を確定
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		Bone.PrevPoseLocation = Bone.CurrentPoseLocation;
		Bone.PrevPoseRotation = Bone.CurrentPoseRotation;
	}
}

void FAnimNode_KawaiiPhysics::SimulateOnce(FComponentSpacePoseContext& Output,
                                           const FTransform& ComponentTransform,
                                           const FSceneInterface* Scene,
                                           const USkeletalMeshComponent* SkelComp)
{
	// Apply deferred impulses queued from GameThread (thread-safe)
	{
		FScopeLock Lock(&GetImpulseMutex());
		for (const FDeferredImpulse& Impulse : PendingImpulses)
		{
			if (ModifyBones.IsValidIndex(Impulse.BoneIndex))
			{
				FKawaiiPhysicsModifyBone& Bone = ModifyBones[Impulse.BoneIndex];
				Bone.PrevLocation -= Impulse.Direction * Impulse.Strength * DeltaTimeOld;
			}
		}
		PendingImpulses.Empty();
	}

	// root bone（ParentIndex<0）の kinematic follow: （補間済み）ポーズへ追従。
	// 元の skip ループから移設。サブステップ毎に補間ポーズへ追従させる。
	// root bones follow the (interpolated) pose. Moved out of the skip loop so it runs every substep.
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		// Root bone (ParentIndex < 0) that is not a bridge dummy and has a valid bone index or is a dummy
		// Simplify: !(BoneRef.BoneIndex < 0 && !Bone.bDummy) == (BoneRef.BoneIndex >= 0 || Bone.bDummy)
		const bool bIsRootBone = Bone.ParentIndex < 0;
		const bool bIsValidRoot = bIsRootBone && !Bone.bBridgeDummy && (Bone.BoneRef.BoneIndex >= 0 || Bone.bDummy);
		if (bIsValidRoot)
		{
			Bone.PrevPrevLocation = Bone.PrevLocation;
			Bone.PrevLocation = Bone.Location;
			Bone.Location = Bone.PoseLocation;
		}
	}

	// GPU path: offload Verlet integration + stiffness + world move to GPU compute
	const bool bUseGPU = bEnableGPUSimulation && GPUCompute && GPUCompute->IsGPUSimulationAvailable();
	if (bUseGPU)
	{
		const int32 NumBones = ModifyBones.Num();
		TArray<FVector> GPUBoneLocations;
		TArray<FVector> GPUBonePrevLocations;
		TArray<FVector> GPUBonePoseLocations;
		TArray<FVector4> GPUBoneSettings;
		GPUBoneLocations.SetNumUninitialized(NumBones);
		GPUBonePrevLocations.SetNumUninitialized(NumBones);
		GPUBonePoseLocations.SetNumUninitialized(NumBones);
		GPUBoneSettings.SetNumUninitialized(NumBones);

		int32 NumActive = 0;
		for (int32 i = 0; i < NumBones; ++i)
		{
			const FKawaiiPhysicsModifyBone& Bone = ModifyBones[i];
			GPUBoneLocations[i] = Bone.Location;
			GPUBonePrevLocations[i] = Bone.PrevLocation;
			GPUBonePoseLocations[i] = Bone.PoseLocation;
			// Pack: X=Damping, Y=WorldDampingLocation, Z=unused, W=Stiffness
			GPUBoneSettings[i] = FVector4(
				Bone.PhysicsSettings.Damping,
				Bone.PhysicsSettings.WorldDampingLocation,
				0.0f,
				Bone.PhysicsSettings.Stiffness);
			if (!Bone.bSkipSimulate && !Bone.bBridgeDummy)
			{
				NumActive = i + 1;
			}
		}

		GPUCompute->TickGPU(
			GPUBoneLocations, GPUBonePrevLocations,
			GPUBonePoseLocations, GPUBoneSettings,
			NumActive, GetStepDeltaTime(), PredictionScale,
			SkelCompMoveVector, SkelCompMoveRotation,
			GravityInSimSpace);

		// Read back results and apply stiffness + world move follow
		const int32 EffectiveTargetFramerate = GetEffectiveTargetFramerate();
		const float Exponent = EffectiveTargetFramerate * GetStepDeltaTime();
		for (int32 i = 0; i < NumBones; ++i)
		{
			FKawaiiPhysicsModifyBone& Bone = ModifyBones[i];
			if (Bone.bSkipSimulate || Bone.bBridgeDummy)
			{
				continue;
			}

			Bone.PrevPrevLocation = Bone.PrevLocation;
			Bone.PrevLocation = Bone.Location;
			Bone.Location = GPUBoneLocations[i];

			// Apply stiffness (pull toward pose) — matches CPU path formula
			if (Bone.ParentIndex >= 0 && ModifyBones.IsValidIndex(Bone.ParentIndex))
			{
				const FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];
				ApplyStiffnessPull(Bone, ParentBone, Exponent);
			}

			// Apply world movement follow (ComponentSpace/WorldSpace non-BaseBoneSpace)
			if (SimulationSpace != EKawaiiPhysicsSimulationSpace::BaseBoneSpace
				&& TeleportType != ETeleportType::TeleportPhysics)
			{
				ApplyWorldMoveFollowNonBaseBone(Bone);
			}
		}
	}
	else
	{
	// CPU path: per-bone Simulate（Exponent は GetStepDeltaTime ベース。サブステップ時は TargetFramerate*FixedDt=1）
	const int32 EffectiveTargetFramerate = GetEffectiveTargetFramerate();
	const float Exponent = EffectiveTargetFramerate * GetStepDeltaTime();
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate)
		{
			continue;
		}

		// コリジョン専用モード: Simulate()をスキップ（コリジョンとbone length restorationは後で実行）
		// Collision-only mode: skip physics simulation, position from real bones after loop
		if (Bone.bInterBoneDummy && bBoneSubdivisionCollisionOnly)
		{
			continue;
		}

		// bridge dummyは常にSimulate()をスキップ（縦親が無くModifyBones[ParentIndex]参照でクラッシュする）
		// Bridge dummies always skip Simulate() (no vertical parent → ModifyBones[Bone.ParentIndex] would crash)
		if (Bone.bBridgeDummy)
		{
			continue;
		}

		Simulate(Bone, Scene, ComponentTransform, Exponent, SkelComp, Output);
	}
	}

	// コリジョン専用モード: 全実ボーンのシミュレーション完了後、シミュレーション済みのLocation間にダミーを配置
	// Collision-only: position dummies between simulated real bones (not PoseLocation)
	if (bBoneSubdivisionCollisionOnly)
	{
		for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
		{
			if (Bone.bInterBoneDummy)
			{
				if (!ensureMsgf(ModifyBones.IsValidIndex(Bone.InterBoneRealParentIndex) &&
				                ModifyBones.IsValidIndex(Bone.InterBoneRealChildIndex),
				                TEXT("KawaiiPhysics: invalid inter-bone dummy endpoint index.")))
				{
					continue;
				}

				Bone.PrevPrevLocation = Bone.PrevLocation;
				Bone.PrevLocation = Bone.Location;
				Bone.Location = FMath::Lerp(
					ModifyBones[Bone.InterBoneRealParentIndex].Location,
					ModifyBones[Bone.InterBoneRealChildIndex].Location,
					Bone.InterBoneAlpha);
			}
		}
	}

	// bridge dummy（横方向Constraintのコリジョン代理）を端点間で配置。
	// bBoneSubdivisionCollisionOnlyに依らず常に実行し、縦dummyの後（より大きいindex）に走るため端点は配置済み。
	// Position bridge dummies (horizontal-constraint collision proxies) between their two endpoints.
	// Always runs (independent of bBoneSubdivisionCollisionOnly); they have higher indices than vertical dummies, so endpoints are already placed.
	{
		SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_BridgeDummy);
		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
		for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
		{
			if (!Bone.bBridgeDummy)
			{
				continue;
			}

			if (!ensureMsgf(ModifyBones.IsValidIndex(Bone.InterBoneRealParentIndex) &&
			                ModifyBones.IsValidIndex(Bone.InterBoneRealChildIndex),
			                TEXT("KawaiiPhysics: invalid bridge dummy endpoint index.")))
			{
				continue;
			}

			const FKawaiiPhysicsModifyBone& EndA = ModifyBones[Bone.InterBoneRealParentIndex];
			const FKawaiiPhysicsModifyBone& EndB = ModifyBones[Bone.InterBoneRealChildIndex];

			// LOD安全: 実ボーン端点がLODでカルされている場合はproxyをスキップ（staleな端点での誤配置を防ぐ）
			// LOD safety: skip when a real-bone endpoint is culled by the current LOD (avoid placing from stale endpoints)
			if ((!EndA.bDummy && EndA.BoneRef.BoneIndex >= 0 && EndA.BoneRef.GetCompactPoseIndex(BoneContainer) < 0) ||
				(!EndB.bDummy && EndB.BoneRef.BoneIndex >= 0 && EndB.BoneRef.GetCompactPoseIndex(BoneContainer) < 0))
			{
				// 端点がLODでカル → proxyを無効化し、後続のコリジョンループからも除外する（stale位置で当たらないように）。
				// 次フレーム冒頭のダミー配置ループでbSkipSimulate=falseに戻り再評価される。
				// Endpoint culled by LOD -> disable the proxy and exclude it from the following collision loop
				// (so it doesn't collide at a stale position). The dummy-placement loop at the start of the next
				// frame resets bSkipSimulate=false and re-evaluates.
				Bone.bSkipSimulate = true;
				continue;
			}

			Bone.PrevPrevLocation = Bone.PrevLocation;
			Bone.PrevLocation = Bone.Location;
			Bone.Location = FMath::Lerp(EndA.Location, EndB.Location, Bone.InterBoneAlpha);
			// LERP基準位置をPoseLocationに退避（コリジョン後の押し出し量 = Location - PoseLocation を測るため。
			// bridge dummyのPoseLocationは他で未使用なので流用）。
			// Stash the LERP base in PoseLocation so the post-collision push = (Location - PoseLocation) can be measured.
			// PoseLocation is otherwise unused for bridge dummies.
			Bone.PoseLocation = Bone.Location;
		}
	}

	// External Force : PostApply
	// Collect one-shot forces to remove after iteration to avoid invalidating the array
	TArray<int32> OneShotForceIndices;
	for (int i = 0; i < ExternalForces.Num(); ++i)
	{
		if (ExternalForces[i].IsValid())
		{
			auto& Force = ExternalForces[i].GetMutable<FKawaiiPhysics_ExternalForce>();
			Force.PostApply(*this, Output);
			if (Force.bIsOneShot)
			{
				OneShotForceIndices.Add(i);
			}
		}
	}
	// Remove one-shot forces in reverse order to preserve indices
	for (int32 idx = OneShotForceIndices.Num() - 1; idx >= 0; --idx)
	{
		ExternalForces.RemoveAt(OneShotForceIndices[idx]);
	}

	// Adjust by Bone Constraints Before Collision
	if (BoneConstraintIterationCountBeforeCollision > 0)
	{
		// Heuristic solve order: sort from leaves to root for faster convergence
		SortConstraintsHeuristic();
		for (FModifyBoneConstraint& BoneConstraint : MergedBoneConstraints)
		{
			BoneConstraint.Lambda = 0.0f;
		}
		for (int i = 0; i < BoneConstraintIterationCountBeforeCollision; ++i)
		{
			AdjustByBoneConstraints();
		}
	}

	// Adjust by collisions
	// NOTE: 形状ごとにループを分けると ModifyBones を複数回走査してキャッシュ効率が落ちるため
	// （ボーン数が多いケースで負荷増）、従来どおりボーン外側の1パスで全形状を処理する。
	// World判定の時間は関数内の既存STAT（STAT_KawaiiPhysics_WorldCollision）で計測する。
	// NOTE: keep a single per-bone pass for all shapes. Splitting per shape would re-traverse
	// ModifyBones multiple times and hurt cache locality (a regression for high bone counts).
	// World-check time is measured by the existing STAT inside AdjustByWorldCollision.
	int32 NumWorldChecks = 0;
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate)
		{
			continue;
		}

		SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_AdjustByCollision);

		AdjustBySphereCollision(Bone, SphericalLimits);
		AdjustBySphereCollision(Bone, SphericalLimitsData);
		AdjustByCapsuleCollision(Bone, CapsuleLimits);
		AdjustByCapsuleCollision(Bone, CapsuleLimitsData);
		AdjustByBoxCollision(Bone, BoxLimits);
		AdjustByBoxCollision(Bone, BoxLimitsData);
		AdjustByPlanerCollision(Bone, PlanarLimits);
		AdjustByPlanerCollision(Bone, PlanarLimitsData);

		// Mesh-based collision (GJK/EPA via Chaos Physics)
		AdjustByMeshCollision(Bone, MeshCollisions, Output);

		// 共有コリジョン / Shared collision from other KawaiiPhysics nodes
		if (bUseSharedCollision && !bSharedCollisionSource)
		{
			AdjustBySphereCollision(Bone, SharedSphericalLimits);
			AdjustByCapsuleCollision(Bone, SharedCapsuleLimits);
			AdjustByBoxCollision(Bone, SharedBoxLimits);
			AdjustByPlanerCollision(Bone, SharedPlanarLimits);
		}

		if (bAllowWorldCollision)
		{
			AdjustByWorldCollision(Output, Bone, SkelComp);
			++NumWorldChecks; // 発行したワールドスイープ回数 / world sweeps issued this frame
		}
	}
	SET_DWORD_STAT(STAT_KawaiiPhysics_NumWorldCollisionChecks, NumWorldChecks);

	// Volume Preservation: adjust chain lengths to maintain volume
	ApplyVolumePreservation();

	// Bone Scaling: scale bones based on compression/stretch (UBE-style visual effect)
	ApplyBoneScaling();

	// Morph Target Driving: drive morph targets based on bone velocity
	ApplyMorphTargetDriving(Output);

	// Morph Target Physics Influence: morph weight → physics parameters (reverse direction)
	ApplyMorphTargetPhysicsInfluence(Output);

	// Apply physics material per-bone settings (before pose blending so multipliers apply on top)
	ApplyPhysicsMaterial();

	// Apply state-driven physics switching (before pose blending so multipliers apply on top)
	ApplyStateDrivenPhysics();

	// Detect pose and apply pose-based blending (must run before gravity computation
	// so CurrentGravityMultiplier is set before GravityInSimSpace is scaled)
	DetectPose();
	ApplyPoseBlending();

	// Gravity (after pose blending so CurrentGravityMultiplier is applied)
	GravityInSimSpace = ConvertSimulationSpaceVector(Output,
	                                                 bUseWorldSpaceGravity
		                                                 ? EKawaiiPhysicsSimulationSpace::WorldSpace
		                                                 : EKawaiiPhysicsSimulationSpace::ComponentSpace,
	                                                 SimulationSpace, Gravity);
	if (bUseDefaultGravityZProjectSetting)
	{
		GravityInSimSpace *= FMath::Abs(UPhysicsSettings::Get()->DefaultGravityZ);
	}
	GravityInSimSpace *= CurrentGravityMultiplier;

	// Apply advanced spring/damper model
	ApplySpringDamper();

	// Apply cloth-like constraints (shear/bend/stretch)
	ApplyClothConstraints();

	// Apply ragdoll blend
	ApplyRagdollBlend();

	// Apply bone filter (skip non-moving bones)
	ApplyBoneFilter();

	// Apply weapon/accessory pendulum physics
	ApplyWeaponPhysics();

	// Self-Collision: prevent bone overlap
	ApplySelfCollision();

	// Spring-Based Volume Preservation
	ApplySpringVolume();

	// bridge dummy のコリジョン変位を端点ボーンへ直接転送（実ボーンを押し出すフィードバックの本体）。
	// コリジョン後・Constraint/length復元前に実行。Push = Location(押し出し後) - PoseLocation(上のLERP配置で退避したLERP基準)。
	// 端点へ距離比 (1-α):α で配分し、Scaleで強さを調整。端点が縦dummyの場合も後段のlength復元で実子へ伝播する。
	// Direct displacement transfer: push the bridge dummy's collision displacement into its endpoint bones (this is
	// what makes a collider moving between columns actually move the cloth). Runs after collision, before constraints/
	// length restore. Push = Location - PoseLocation (LERP base stashed above during bridge-dummy placement),
	// distributed (1-alpha):alpha, scaled.
	if (BoneConstraintSubdivisionCount > 0 && BoneConstraintSubdivisionFeedbackScale > 0.0f)
	{
		SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_BridgeDummy);
		// 端点ごとに「重み付き押し出し量の合計」と「重みの合計」を集計し、最後に divisor=max(1,重み合計) で割る。
		// 寄与が少ない（重み合計<=1, 局所接触の通常ケース）ときは単純加算と同一の挙動を保ち、
		// 多数のdummyが同じ端点を押す病的ケースでのみ加重平均化してN倍のオーバーシュート/発振を防ぐ。
		// Accumulate per-endpoint weighted push and total weight, then divide by max(1, total weight).
		// When few dummies contribute (weight <= 1, typical localized contact) this equals plain additive transfer;
		// only when many dummies push the same endpoint does it average out, preventing N-fold overshoot/oscillation.
		TMap<int32, FVector> AccumPush;
		TMap<int32, float> AccumWeight;
		// 端点インデックスは最大でもModifyBones数。事前確保で毎フレームの再ハッシュを回避
		// Endpoint indices are bounded by ModifyBones count; reserve to avoid per-frame rehashing
		AccumPush.Reserve(ModifyBones.Num());
		AccumWeight.Reserve(ModifyBones.Num());

		for (const FKawaiiPhysicsModifyBone& Bone : ModifyBones)
		{
			if (!Bone.bBridgeDummy || Bone.bSkipSimulate)
			{
				continue;
			}
			if (!ModifyBones.IsValidIndex(Bone.InterBoneRealParentIndex) ||
				!ModifyBones.IsValidIndex(Bone.InterBoneRealChildIndex))
			{
				continue;
			}

			const FVector Push = Bone.Location - Bone.PoseLocation; // コリジョンによる押し出し量 / collision displacement
			if (Push.IsNearlyZero())
			{
				continue;
			}

			const float A = Bone.InterBoneAlpha;
			const int32 E1 = Bone.InterBoneRealParentIndex;
			const int32 E2 = Bone.InterBoneRealChildIndex;
			const float W1 = 1.0f - A; // 端点1に近いほど寄与大 / closer to endpoint1 => larger weight
			const float W2 = A;

			AccumPush.FindOrAdd(E1) += Push * W1;
			AccumWeight.FindOrAdd(E1) += W1;
			AccumPush.FindOrAdd(E2) += Push * W2;
			AccumWeight.FindOrAdd(E2) += W2;
		}

		for (const TPair<int32, FVector>& Entry : AccumPush)
		{
			if (ModifyBones.IsValidIndex(Entry.Key))
			{
				const float W = AccumWeight.FindChecked(Entry.Key);
				ModifyBones[Entry.Key].Location += (Entry.Value / FMath::Max(1.0f, W)) * BoneConstraintSubdivisionFeedbackScale;
			}
		}
	}

	// CCD: prevent tunneling during fast movements
	ApplyCCD();

	// Backstop: prevent bones from penetrating character mesh
	ApplyBackstopCollision();

	// Adjust by Bone Constraints After Collision
	if (BoneConstraintIterationCountAfterCollision > 0)
	{
		for (FModifyBoneConstraint& BoneConstraint : MergedBoneConstraints)
		{
			BoneConstraint.Lambda = 0.0f;
		}
		for (int i = 0; i < BoneConstraintIterationCountAfterCollision; ++i)
		{
			AdjustByBoneConstraints();
		}
	}

	// Adjust by Limits and Bone Length（角度制限 + 平面制約 + ボーン長復元のO(N)ループをまとめて計測）
	// Adjust by limits and bone length (measure the angle-limit + planar-constraint + length-restore O(N) loop)
	{
		SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_AdjustByLimitsAndLength);
		for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
		{
			if (Bone.bSkipSimulate)
			{
				continue;
			}

			// bridge dummyは縦親を持たないため長さ/角度復元をスキップ（ParentIndex=-1参照でクラッシュ）。
			// 位置は直前のconstraint solveで確定済みで、次フレーム冒頭で端点間に再LERPされる。
			// Bridge dummies have no vertical parent → skip length/angle restore (would deref ParentIndex=-1).
			// Their position is finalized by the preceding constraint solve and re-LERP'd next frame.
			if (Bone.bBridgeDummy)
			{
				continue;
			}

			auto& ParentBone = ModifyBones[Bone.ParentIndex];

			// Adjust by angle limit
			AdjustByAngleLimit(Bone, ParentBone);

			// Adjust by Planar Constraint
			AdjustByPlanarConstraint(Bone, ParentBone);

			// Restore Bone Length
			const float BoneLength = (Bone.PoseLocation - ParentBone.PoseLocation).Size();
			Bone.Location = (Bone.Location - ParentBone.Location).GetSafeNormal() * BoneLength + ParentBone.Location;
		}
	}
	// 注: DeltaTimeOld は呼び出し元 SimulateModifyBones（legacy=DeltaTime / substep=FixedDt）で設定
	// Note: DeltaTimeOld is set by the caller SimulateModifyBones (legacy=DeltaTime / substep=FixedDt)
	FrameCount++;
}

// Volume Preservation: adjust bone chain lengths to maintain original volume
void FAnimNode_KawaiiPhysics::ApplyVolumePreservation()
{
	// Find chain roots (bones with ParentIndex < 0 that have VolumePreservation > 0)
	for (int32 i = 0; i < ModifyBones.Num(); ++i)
	{
		FKawaiiPhysicsModifyBone& Bone = ModifyBones[i];
		if (Bone.ParentIndex >= 0 || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		float VP = Bone.PhysicsSettings.VolumePreservation;
		if (VP <= 0.0f)
		{
			continue;
		}

		// Calculate current chain length
		float CurrentLength = 0.0f;
		int32 CurrentIdx = i;
		while (ModifyBones.IsValidIndex(CurrentIdx))
		{
			const FKawaiiPhysicsModifyBone& Cur = ModifyBones[CurrentIdx];
			if (Cur.ParentIndex >= 0 && ModifyBones.IsValidIndex(Cur.ParentIndex))
			{
				const FKawaiiPhysicsModifyBone& Parent = ModifyBones[Cur.ParentIndex];
				CurrentLength += (Cur.Location - Parent.Location).Size();
			}
			// Move to first child
			if (Cur.ChildIndices.Num() > 0)
			{
				CurrentIdx = Cur.ChildIndices[0];
			}
			else
			{
				break;
			}
		}

		// Get or compute original chain length
		float& OriginalLength = OriginalChainLengths.FindOrAdd(i);
		if (OriginalLength <= 0.0f)
		{
			OriginalLength = CurrentLength;
		}

		if (OriginalLength <= 0.0f || CurrentLength <= 0.0f)
		{
			continue;
		}

		// Scale factor: blend between current and original
		float TargetLength = FMath::Lerp(CurrentLength, OriginalLength, VP);
		float Scale = (CurrentLength > KINDA_SMALL_NUMBER) ? TargetLength / CurrentLength : 1.0f;

		// Apply pressure: if compressed, push outward
		float PressureScale = 1.0f;
		if (Bone.PhysicsSettings.Pressure > 0.0f && CurrentLength < OriginalLength)
		{
			float CompressionRatio = 1.0f - (CurrentLength / OriginalLength);
			PressureScale = 1.0f + CompressionRatio * Bone.PhysicsSettings.Pressure;
		}

		float FinalScale = Scale * PressureScale;

		// Scale bone positions relative to root - traverse all children, not just first child
		FVector RootLocation = Bone.Location;
		CurrentIdx = i;
		TArray<int32> Queue;
		Queue.Add(i);
		int32 QueueHead = 0;
		while (QueueHead < Queue.Num())
		{
			const int32 ProcessIdx = Queue[QueueHead++];

			if (!ModifyBones.IsValidIndex(ProcessIdx))
			{
				continue;
			}

			FKawaiiPhysicsModifyBone& Cur = ModifyBones[ProcessIdx];
			if (Cur.ParentIndex >= 0 && ModifyBones.IsValidIndex(Cur.ParentIndex))
			{
				FVector Offset = Cur.Location - RootLocation;
				Cur.Location = RootLocation + Offset * FinalScale;
			}

			// Add all children to the queue
			for (int32 ChildIdx : Cur.ChildIndices)
			{
				if (ModifyBones.IsValidIndex(ChildIdx))
				{
					Queue.Add(ChildIdx);
				}
			}
		}
	}
}

// Bone Scaling: scale bones based on compression/stretch (UBE-style visual effect)
void FAnimNode_KawaiiPhysics::ApplyBoneScaling()
{
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		float Compensation = Bone.PhysicsSettings.BoneScaleCompensation;
		if (Compensation <= 0.0f || Bone.ParentIndex < 0 || !ModifyBones.IsValidIndex(Bone.ParentIndex))
		{
			continue;
		}

		const FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];

		// Calculate compression/stretch ratio
		const float CurrentLength = (Bone.Location - ParentBone.Location).Size();
		const float PoseLength = (Bone.PoseLocation - ParentBone.PoseLocation).Size();

		if (PoseLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Ratio = CurrentLength / PoseLength;
		const float MaxScale = Bone.PhysicsSettings.MaxBoneScale;

		// Scale = 1 + (1 - Ratio) * Compensation
		// When compressed (Ratio < 1): Scale > 1 (bone gets thicker)
		// When stretched (Ratio > 1): Scale < 1 (bone gets thinner)
		float Scale = 1.0f + (1.0f - Ratio) * Compensation;
		Scale = FMath::Clamp(Scale, 1.0f / MaxScale, MaxScale);

		// Apply scale to bone based on axis setting
		const int32 Axis = Bone.PhysicsSettings.BoneScaleAxis;
		switch (Axis)
		{
		case 0: // Uniform (all axes)
			Bone.BoneScale = FVector(Scale);
			break;
		case 1: // X-axis only (width)
			Bone.BoneScale = FVector(Scale, 1.0f, 1.0f);
			break;
		case 2: // Y-axis only (depth)
			Bone.BoneScale = FVector(1.0f, Scale, 1.0f);
			break;
		case 3: // Z-axis only (height)
			Bone.BoneScale = FVector(1.0f, 1.0f, Scale);
			break;
		default:
			Bone.BoneScale = FVector(Scale);
			break;
		}
	}
}

// Morph Target Driving: drive morph targets based on bone velocity
void FAnimNode_KawaiiPhysics::ApplyMorphTargetDriving(FComponentSpacePoseContext& Output)
{
	if (!bEnableMorphTargetDriving || MorphTargetBoneMapping.Num() == 0)
	{
		return;
	}

	if (!Output.AnimInstanceProxy)
	{
		return;
	}

	USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	if (!SkelComp)
	{
		return;
	}

	for (const auto& Mapping : MorphTargetBoneMapping)
	{
		const FName& MorphTargetName = Mapping.Key;
		const FName& BoneName = Mapping.Value;

		// Find bone
		for (const FKawaiiPhysicsModifyBone& Bone : ModifyBones)
		{
			if (Bone.BoneRef.BoneName == BoneName)
			{
				// Calculate bone speed
				const FVector Velocity = Bone.Location - Bone.PrevLocation;
				const float Speed = Velocity.Size() * GetEffectiveTargetFramerate();

				// Map speed to morph target value (0-1)
				float MorphValue = 0.0f;
				if (Speed > MorphTargetSpeedThreshold)
				{
					MorphValue = FMath::Clamp((Speed - MorphTargetSpeedThreshold) / MorphTargetSpeedNormalizationScale, 0.0f, 1.0f);
					MorphValue *= MorphTargetMultiplier;
				}

				// Apply morph target
				SkelComp->SetMorphTarget(MorphTargetName, MorphValue);
				break;
			}
		}
	}
}

// Apply morph target influence on physics parameters (reverse direction: morph weight → physics)
void FAnimNode_KawaiiPhysics::ApplyMorphTargetPhysicsInfluence(FComponentSpacePoseContext& Output)
{
	if (!bEnableMorphTargetPhysicsInfluence || MorphTargetPhysicsInfluence.Num() == 0)
	{
		return;
	}

	if (!Output.AnimInstanceProxy)
	{
		return;
	}

	USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	if (!SkelComp)
	{
		return;
	}

	for (auto& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Check each morph target influence mapping
		for (const auto& Influence : MorphTargetPhysicsInfluence)
		{
			const FName& MorphTargetName = Influence.Key;
			const FMorphTargetPhysicsInfluence& Params = Influence.Value;

			// Get morph target weight from skeletal mesh
			const float MorphWeight = SkelComp->GetMorphTarget(MorphTargetName);

			if (MorphWeight > KINDA_SMALL_NUMBER)
			{
				// Interpolate multipliers based on morph weight
				const float DampingMult = FMath::Lerp(1.0f, Params.DampingMultiplier, MorphWeight);
				const float StiffnessMult = FMath::Lerp(1.0f, Params.StiffnessMultiplier, MorphWeight);
				const float LimitAngleMult = FMath::Lerp(1.0f, Params.LimitAngleMultiplier, MorphWeight);

				// Apply multipliers to physics settings
				Bone.PhysicsSettings.Damping *= DampingMult;
				Bone.PhysicsSettings.Stiffness *= StiffnessMult;
				if (Bone.PhysicsSettings.LimitAngle > 0)
				{
					Bone.PhysicsSettings.LimitAngle *= LimitAngleMult;
				}
			}
		}
	}
}

// Detect character pose based on pelvis/spine orientation
void FAnimNode_KawaiiPhysics::DetectPose()
{
	if (!bEnablePoseBlending || ModifyBones.Num() == 0)
	{
		return;
	}

	// Find pelvis or root bone to determine pose
	for (const FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.ParentIndex < 0 && !Bone.bDummy)
		{
			// Use bone's Z-axis direction to determine pose
			const FVector BoneUp = Bone.PoseRotation.GetUpVector();
			const float ZComponent = BoneUp.Z;

			if (ZComponent > PoseDetectionStandingThreshold)
			{
				DetectedPose = EKawaiiPhysicsDetectedPose::Standing;
			}
			else if (ZComponent > PoseDetectionSittingThreshold)
			{
				DetectedPose = EKawaiiPhysicsDetectedPose::Sitting;
			}
			else
			{
				DetectedPose = EKawaiiPhysicsDetectedPose::Lying;
			}
			break;
		}
	}
}

// Apply pose-based physics blending
void FAnimNode_KawaiiPhysics::ApplyPoseBlending()
{
	if (!bEnablePoseBlending)
	{
		return;
	}

	float GravityMultiplier = 1.0f;
	float DampingMultiplier = 1.0f;
	float StiffnessMultiplier = 1.0f;

	switch (DetectedPose)
	{
	case EKawaiiPhysicsDetectedPose::Standing:
		GravityMultiplier = PoseGravityStanding;
		DampingMultiplier = 1.0f;
		StiffnessMultiplier = 1.0f;
		break;
	case EKawaiiPhysicsDetectedPose::Sitting:
		GravityMultiplier = PoseGravitySitting;
		DampingMultiplier = PoseDampingSitting;
		StiffnessMultiplier = 1.0f;
		break;
	case EKawaiiPhysicsDetectedPose::Lying:
		GravityMultiplier = PoseGravityLying;
		DampingMultiplier = 1.0f;
		StiffnessMultiplier = PoseStiffnessLying;
		break;
	default:
		break;
	}

	// Store gravity multiplier for use in gravity computation (after this function returns)
	CurrentGravityMultiplier = GravityMultiplier;

	// Apply multipliers to bone physics settings
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Always refresh original values from current settings before applying multipliers.
		// This ensures pose blending is applied on top of whatever UpdatePhysicsSettingsOfModifyBones
		// or LOD overrides set this frame, rather than compounding stale first-frame values.
		Bone.OriginalDamping = Bone.PhysicsSettings.Damping;
		Bone.OriginalStiffness = Bone.PhysicsSettings.Stiffness;

		// Apply pose-based multipliers
		Bone.PhysicsSettings.Damping = Bone.OriginalDamping * DampingMultiplier;
		Bone.PhysicsSettings.Stiffness = Bone.OriginalStiffness * StiffnessMultiplier;
	}
}

// Apply self-collision between bones
// Uses spatial hash grid for O(n) broadphase instead of O(n²) pairwise check
void FAnimNode_KawaiiPhysics::ApplySelfCollision()
{
	if (!bEnableSelfCollision || ModifyBones.Num() < 2)
	{
		return;
	}

	const float MinDist = SelfCollisionMinDistance * SelfCollisionRadiusMultiplier;
	const float MaxCheckDist = MinDist * SelfCollisionMaxCheckDistMultiplier;
	const float MinDistSq = MinDist * MinDist;

	// Build spatial hash grid
	FKawaiiSpatialGrid Grid;
	Grid.Init(MaxCheckDist);

	// Insert valid bones into grid
	for (int32 i = 0; i < ModifyBones.Num(); ++i)
	{
		const FKawaiiPhysicsModifyBone& Bone = ModifyBones[i];
		if (!Bone.bSkipSimulate && !Bone.bDummy && !Bone.bBridgeDummy)
		{
			Grid.Insert(i, Bone.Location);
		}
	}

	// Check only spatially nearby pairs
	TSet<int32> ProcessedPairs;
	for (int32 i = 0; i < ModifyBones.Num(); ++i)
	{
		FKawaiiPhysicsModifyBone& BoneA = ModifyBones[i];
		if (BoneA.bSkipSimulate || BoneA.bDummy || BoneA.bBridgeDummy)
		{
			continue;
		}

		// Find bones in nearby cells
		TArray<int32> NearbyIndices;
		NearbyIndices.Reserve(32);
		Grid.FindNearby(BoneA.Location, MaxCheckDist, NearbyIndices);

		for (int32 j : NearbyIndices)
		{
			if (j <= i)
			{
				continue; // avoid duplicate pairs and self
			}

			FKawaiiPhysicsModifyBone& BoneB = ModifyBones[j];
			if (BoneB.bSkipSimulate || BoneB.bDummy || BoneB.bBridgeDummy)
			{
				continue;
			}

			// Skip if same chain (parent-child)
			if (BoneA.ParentIndex == j || BoneB.ParentIndex == i)
			{
				continue;
			}

			// Check pair not already processed (spatial grid may return duplicates)
			const int32 PairKey = FMath::Min(i, j) * 65536 + FMath::Max(i, j);
			if (ProcessedPairs.Contains(PairKey))
			{
				continue;
			}
			ProcessedPairs.Add(PairKey);

			// Distance check — push apart only if bones are closer than MinDist
			const FVector Delta = BoneB.Location - BoneA.Location;
			const float DistSq = Delta.SizeSquared();
			if (DistSq <= MinDistSq && DistSq > KINDA_SMALL_NUMBER)
			{
				const float Dist = FMath::Sqrt(DistSq);
				const FVector PushDir = Delta / Dist;
				const float PushAmount = (MinDist - Dist) * SelfCollisionPushSplitRatio;

				BoneA.Location -= PushDir * PushAmount;
				BoneB.Location += PushDir * PushAmount;
			}
		}
	}
}

// Apply spring-based volume preservation
void FAnimNode_KawaiiPhysics::ApplySpringVolume()
{
	if (!bEnableSpringVolume)
	{
		return;
	}

	const float DT = GetStepDeltaTime();

	for (int32 i = 0; i < ModifyBones.Num(); ++i)
	{
		FKawaiiPhysicsModifyBone& Bone = ModifyBones[i];
		if (Bone.ParentIndex >= 0 || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Calculate current chain length
		float CurrentLength = 0.0f;
		int32 CurrentIdx = i;
		while (ModifyBones.IsValidIndex(CurrentIdx))
		{
			const FKawaiiPhysicsModifyBone& Cur = ModifyBones[CurrentIdx];
			if (Cur.ParentIndex >= 0 && ModifyBones.IsValidIndex(Cur.ParentIndex))
			{
				const FKawaiiPhysicsModifyBone& Parent = ModifyBones[Cur.ParentIndex];
				CurrentLength += (Cur.Location - Parent.Location).Size();
			}
			if (Cur.ChildIndices.Num() > 0)
			{
				CurrentIdx = Cur.ChildIndices[0];
			}
			else
			{
				break;
			}
		}

		// Get original length
		float& OriginalLength = OriginalChainLengths.FindOrAdd(i);
		if (OriginalLength <= 0.0f)
		{
			OriginalLength = CurrentLength;
		}

		if (OriginalLength <= 0.0f || CurrentLength <= 0.0f)
		{
			continue;
		}

		// Spring physics: F = -k * (x - rest) - c * v
		float& Velocity = SpringVolumeVelocities.FindOrAdd(i);
		const float TargetLength = OriginalLength * SpringVolumeRestLength;
		const float Displacement = CurrentLength - TargetLength;
		const float SpringForce = -SpringVolumeStiffness * Displacement;
		const float DampingForce = -SpringVolumeDamping * Velocity;

		Velocity += (SpringForce + DampingForce) * DT;
		const float LengthCorrection = Velocity * DT;

		// Apply correction
		if (CurrentLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		float Scale = (CurrentLength + LengthCorrection) / CurrentLength;
		Scale = FMath::Clamp(Scale, SpringVolumeMinScale, SpringVolumeMaxScale);

		FVector RootLocation = Bone.Location;
		CurrentIdx = i;
		TArray<int32> Queue;
		Queue.Add(i);
		int32 QueueHead = 0;
		while (QueueHead < Queue.Num())
		{
			const int32 ProcessIdx = Queue[QueueHead++];

			if (!ModifyBones.IsValidIndex(ProcessIdx))
			{
				continue;
			}

			FKawaiiPhysicsModifyBone& Cur = ModifyBones[ProcessIdx];
			if (Cur.ParentIndex >= 0 && ModifyBones.IsValidIndex(Cur.ParentIndex))
			{
				FVector Offset = Cur.Location - RootLocation;
				Cur.Location = RootLocation + Offset * Scale;
			}

			for (int32 ChildIdx : Cur.ChildIndices)
			{
				if (ModifyBones.IsValidIndex(ChildIdx))
				{
					Queue.Add(ChildIdx);
				}
			}
		}
	}
}

// Apply animation state-driven physics switching
void FAnimNode_KawaiiPhysics::ApplyStateDrivenPhysics()
{
	if (!bEnableStateDrivenPhysics)
	{
		return;
	}

	const FKawaiiPhysicsSettings* StateSettings = StatePhysicsSettings.Find(CurrentAnimationState);
	if (!StateSettings)
	{
		return;
	}

	// Apply state-specific settings to all bones
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Store original on first frame (only if not already stored by pose blending)
		if (!Bone.bOriginalValuesStored)
		{
			Bone.OriginalDamping = Bone.PhysicsSettings.Damping;
			Bone.OriginalStiffness = Bone.PhysicsSettings.Stiffness;
			Bone.bOriginalValuesStored = true;
		}

		// Apply state settings
		Bone.PhysicsSettings.Damping = StateSettings->Damping;
		Bone.PhysicsSettings.Stiffness = StateSettings->Stiffness;
		Bone.PhysicsSettings.LimitAngle = StateSettings->LimitAngle;
		Bone.PhysicsSettings.Radius = StateSettings->Radius;
	}
}

// Apply physics material per-bone settings
// Priority: PerBoneMaterialOverrides > PhysicsMaterial > base settings
void FAnimNode_KawaiiPhysics::ApplyPhysicsMaterial()
{
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Priority 1: Per-bone explicit overrides (highest priority)
		if (FKawaiiPhysicsSettings* Override = PerBoneMaterialOverrides.Find(Bone.BoneRef.BoneName))
		{
			Bone.PhysicsSettings.Damping = Override->Damping;
			Bone.PhysicsSettings.Stiffness = Override->Stiffness;
			Bone.PhysicsSettings.LimitAngle = Override->LimitAngle;
			Bone.PhysicsSettings.Radius = Override->Radius;
			Bone.PhysicsSettings.VolumePreservation = Override->VolumePreservation;
			Bone.PhysicsSettings.Pressure = Override->Pressure;
			Bone.PhysicsSettings.BoneScaleCompensation = Override->BoneScaleCompensation;
			continue;
		}

		// Priority 2: Global PhysicsMaterial asset (prefix matching)
		if (PhysicsMaterial)
		{
			FKawaiiPhysicsSettings MaterialSettings;
			if (PhysicsMaterial->GetBoneSettings(Bone.BoneRef.BoneName, MaterialSettings))
			{
				Bone.PhysicsSettings.Damping = MaterialSettings.Damping;
				Bone.PhysicsSettings.Stiffness = MaterialSettings.Stiffness;
				Bone.PhysicsSettings.LimitAngle = MaterialSettings.LimitAngle;
				Bone.PhysicsSettings.Radius = MaterialSettings.Radius;
				Bone.PhysicsSettings.VolumePreservation = MaterialSettings.VolumePreservation;
				Bone.PhysicsSettings.Pressure = MaterialSettings.Pressure;
				Bone.PhysicsSettings.BoneScaleCompensation = MaterialSettings.BoneScaleCompensation;
			}
		}
	}
}

// Apply advanced spring/damper model (ACES-style)
void FAnimNode_KawaiiPhysics::ApplySpringDamper()
{
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy || !Bone.PhysicsSettings.bUseAdvancedSpring)
		{
			continue;
		}

		// Convert frequency/damping ratio to damping/stiffness
		// Spring frequency (Hz) -> angular frequency
		const float Omega = 2.0f * PI * Bone.PhysicsSettings.SpringFrequency;

		// Critical damping = 2 * sqrt(stiffness * mass)
	 // Assuming unit mass: stiffness = Omega^2
		const float Stiffness = Omega * Omega;

		// Damping coefficient = 2 * damping_ratio * sqrt(stiffness * mass)
	 const float Damping = 2.0f * Bone.PhysicsSettings.SpringDampingRatio * Omega;

		// Convert back to KawaiiPhysics scale (0-1)
		Bone.PhysicsSettings.Stiffness = FMath::Clamp(Stiffness / StiffnessNormalizationScale, 0.0f, 1.0f);
		Bone.PhysicsSettings.Damping = FMath::Clamp(Damping / DampingNormalizationScale, 0.0f, 1.0f);
	}
}

// Apply cloth-like constraints (shear/bend/stretch)
void FAnimNode_KawaiiPhysics::ApplyClothConstraints()
{
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy || Bone.ParentIndex < 0)
		{
			continue;
		}

		FKawaiiPhysicsSettings& Settings = Bone.PhysicsSettings;

		// Shear constraint: resist diagonal movement
		if (Settings.ShearStiffness > 0.0f && ModifyBones.IsValidIndex(Bone.ParentIndex))
		{
			// Calculate shear force based on bone angle deviation
			const FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];
			const FVector BoneDir = (Bone.Location - ParentBone.Location).GetSafeNormal();
			const FVector PoseDir = (Bone.PoseLocation - ParentBone.PoseLocation).GetSafeNormal();

			if (!BoneDir.IsNearlyZero() && !PoseDir.IsNearlyZero())
			{
				const float AngleDiff = FMath::Acos(FMath::Clamp(FVector::DotProduct(BoneDir, PoseDir), -1.0f, 1.0f));
				const float ShearForce = AngleDiff * Settings.ShearStiffness * ClothConstraintForceScale;

				// Apply shear correction
				Bone.Location = FMath::Lerp(Bone.Location, Bone.PoseLocation, ShearForce);
			}
		}

		// Stretch constraint - guard against zero distance to prevent NaN
		if (Bone.ParentIndex >= 0 && ModifyBones.IsValidIndex(Bone.ParentIndex))
		{
			const FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];
			const float CurrentDist = (Bone.Location - ParentBone.Location).Size();
			const float PoseDist = (Bone.PoseLocation - ParentBone.PoseLocation).Size();

			// Guard against zero/negative distances to prevent NaN
			if (PoseDist > KINDA_SMALL_NUMBER && CurrentDist > KINDA_SMALL_NUMBER)
			{
				const float StretchRatio = CurrentDist / PoseDist;

				// Stretch limit
				if (StretchRatio > Settings.StretchLimit)
				{
					const float TargetDist = PoseDist * Settings.StretchLimit;
					const FVector Dir = (Bone.Location - ParentBone.Location).GetSafeNormal();
					Bone.Location = ParentBone.Location + Dir * TargetDist;
				}

				// Compress limit
				if (StretchRatio < Settings.CompressLimit)
				{
					const float TargetDist = PoseDist * Settings.CompressLimit;
					const FVector Dir = (Bone.Location - ParentBone.Location).GetSafeNormal();
					Bone.Location = ParentBone.Location + Dir * TargetDist;
				}
			}
		}

		// Bend constraint (resistance to bending between consecutive bones)
		if (Settings.BendStiffness > 0.0f && Bone.ParentIndex >= 0 && ModifyBones.IsValidIndex(Bone.ParentIndex))
		{
			const FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];
			if (ParentBone.ParentIndex >= 0 && ModifyBones.IsValidIndex(ParentBone.ParentIndex))
			{
				const FKawaiiPhysicsModifyBone& GrandParentBone = ModifyBones[ParentBone.ParentIndex];

				// Calculate bend angle
				const FVector Dir1 = (ParentBone.Location - GrandParentBone.Location).GetSafeNormal();
				const FVector Dir2 = (Bone.Location - ParentBone.Location).GetSafeNormal();

				if (!Dir1.IsNearlyZero() && !Dir2.IsNearlyZero())
				{
					const float BendAngle = FMath::Acos(FMath::Clamp(FVector::DotProduct(Dir1, Dir2), -1.0f, 1.0f));
					const float BendForce = BendAngle * Settings.BendStiffness * ClothConstraintForceScale;

					// Apply bend correction (push toward straight line)
					const FVector StraightDir = Dir1;
					Bone.Location = FMath::Lerp(Bone.Location, ParentBone.Location + StraightDir * (Bone.Location - ParentBone.Location).Size(), BendForce);
				}
			}
		}
	}
}

// Apply ragdoll blend
// Smoothly blends between KawaiiPhysics simulation and pose position.
// RagdollBlendWeight controls the blend (0 = pure physics, 1 = pure pose).
// RagdollBlendSpeed controls the interpolation speed.
void FAnimNode_KawaiiPhysics::ApplyRagdollBlend()
{
	if (RagdollBlendWeight <= 0.0f)
	{
		return;
	}

	const float ClampedWeight = FMath::Clamp(RagdollBlendWeight, 0.0f, 1.0f);
	const float ClampedSpeed = FMath::Max(RagdollBlendSpeed, 0.001f);
	const float StepDt = FMath::Max(GetStepDeltaTime(), SMALL_NUMBER);

	// Blend between physics position and pose position
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Smooth blend toward target weight
		const float BlendAlpha = FMath::FInterpConstantTo(0.0f, ClampedWeight, StepDt, ClampedSpeed);

		// Initialize rotation from pose if not yet set (prevents NaN from uninitialized Slerp)
		if (Bone.Rotation.IsIdentity())
		{
			Bone.Rotation = Bone.PoseRotation;
		}

		// Position blend
		Bone.Location = FMath::Lerp(Bone.Location, Bone.PoseLocation, BlendAlpha);

		// Rotation blend (Slerp for smooth quaternion interpolation)
		if (!Bone.PoseRotation.IsIdentity(KINDA_SMALL_NUMBER))
		{
			Bone.Rotation = FQuat::Slerp(Bone.Rotation, Bone.PoseRotation, BlendAlpha);
		}
	}
}

// Auto-detect bone chains from skeleton (hair, skirt, tail, breast)
// Uses case-insensitive substring matching with topology-based fallback.
void FAnimNode_KawaiiPhysics::AutoDetectBoneChains(const USkeletalMeshComponent* SkelComp)
{
	if (!bAutoDetectBoneChains || !SkelComp)
	{
		return;
	}

	const USkeletalMesh* SkeletalMesh = SkelComp->GetSkeletalMeshAsset();
	if (!SkeletalMesh)
	{
		return;
	}

	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	const int32 NumBones = RefSkeleton.GetNum();

	// Chain type -> list of substrings to match (case-insensitive)
	struct FChainPattern
	{
		EBoneChainType Type;
		TArray<FString> Substrings;
	};

	const TArray<FChainPattern> DefaultPatterns = {
		{ EBoneChainType::Hair, { TEXT("hair"), TEXT("hairs"), TEXT("fringe"), TEXT("bang") } },
		{ EBoneChainType::Skirt, { TEXT("skirt"), TEXT("cape"), TEXT("cloth"), TEXT("coat") } },
		{ EBoneChainType::Tail, { TEXT("tail"), TEXT("tail_") } },
		{ EBoneChainType::Breast, { TEXT("breast"), TEXT("butt"), TEXT("chest"), TEXT("bust") } },
		{ EBoneChainType::Weapon, { TEXT("weapon"), TEXT("amulet"), TEXT("accessory"), TEXT("belt"), TEXT("pendant") } },
	};

	// Use custom patterns if provided, otherwise use defaults
	TArray<FChainPattern> Patterns;
	if (BoneChainDetectionPatterns.Num() > 0)
	{
		for (const auto& Pair : BoneChainDetectionPatterns)
		{
			FChainPattern P;
			P.Type = EBoneChainType::Custom;
			P.Substrings.Add(Pair.Value);
			Patterns.Add(P);
		}
	}
	else
	{
		Patterns = DefaultPatterns;
	}

	// Phase 1: Substring matching (case-insensitive)
	TSet<int32> DetectedRootIndices;
	for (const FChainPattern& Pattern : Patterns)
	{
		for (int32 i = 0; i < NumBones; ++i)
		{
			const FString BoneName = RefSkeleton.GetBoneName(i).ToString().ToLower();
			bool bMatched = false;
			for (const FString& Sub : Pattern.Substrings)
			{
				if (BoneName.Contains(Sub.ToLower()))
				{
					bMatched = true;
					break;
				}
			}
			if (!bMatched)
			{
				continue;
			}

			// Walk up to find chain root (first bone with >1 child or no parent in ModifyBones)
			int32 RootIdx = i;
			while (RootIdx >= 0)
			{
				const int32 ParentIdx = RefSkeleton.GetParentIndex(RootIdx);
				if (ParentIdx < 0)
				{
					break;
				}
				// Check if parent has other children (branching point = chain root)
				TArray<int32> Siblings;
				RefSkeleton.GetDirectChildBones(ParentIdx, Siblings);
				if (Siblings.Num() > 1)
				{
					break; // Parent is a branching point, current is the chain root
				}
				RootIdx = ParentIdx;
			}

			if (RootIdx >= 0 && !DetectedRootIndices.Contains(RootIdx))
			{
				DetectedRootIndices.Add(RootIdx);
			}
		}
	}

	// Phase 2: Topology-based fallback for unmatched chains
	// Find long single-child chains (likely hair/tail/skirt) that weren't detected by name
	if (DetectedRootIndices.Num() == 0)
	{
		for (int32 i = 0; i < NumBones; ++i)
		{
			if (DetectedRootIndices.Contains(i))
			{
				continue;
			}

			// Count chain length (single-child bones from this root)
			int32 ChainLength = 0;
			int32 Current = i;
			while (Current >= 0)
			{
				TArray<int32> Children;
				RefSkeleton.GetDirectChildBones(Current, Children);
				if (Children.Num() != 1)
				{
					break;
				}
				Current = Children[0];
				ChainLength++;
			}

			// Chains of 4+ bones are likely physics chains (hair, tail, skirt)
			if (ChainLength >= 4)
			{
				DetectedRootIndices.Add(i);
			}
		}
	}

	// Apply character profile settings to detected chains
	if (CharacterProfile)
	{
		for (const int32 RootIdx : DetectedRootIndices)
		{
			const FName RootBoneName = RefSkeleton.GetBoneName(RootIdx);
			FKawaiiPhysicsChainPreset Preset;
			if (CharacterProfile->GetChainPreset(RootBoneName, Preset))
			{
				for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
				{
					if (Bone.BoneRef.BoneName == RootBoneName)
					{
						Bone.PhysicsSettings = Preset.PhysicsSettings;
						break;
					}
				}
			}
		}
	}
}

// Apply weapon/accessory pendulum physics
void FAnimNode_KawaiiPhysics::ApplyWeaponPhysics()
{
	if (!bEnableWeaponPhysics || WeaponChainRoots.Num() == 0)
	{
		return;
	}

	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Check if this bone is in a weapon chain
		bool bIsWeaponBone = false;
		for (const FName& Root : WeaponChainRoots)
		{
			if (Bone.BoneRef.BoneName == Root)
			{
				bIsWeaponBone = true;
				break;
			}
		}

		if (!bIsWeaponBone)
		{
			continue;
		}

		// Apply pendulum physics (simplified spring-damper)
		const FVector Velocity = Bone.Location - Bone.PrevLocation;

		// Pendulum restoring force (pull toward rest pose)
		const FVector RestOffset = Bone.PoseLocation - Bone.Location;
		const FVector SpringForce = RestOffset * WeaponPendulumStiffness;

		// Damping
		const FVector DampingForce = -Velocity * WeaponPendulumDamping;

		// Apply forces: gravity is acceleration (needs dt²), spring/damping are velocities (need dt)
		const float DT = GetStepDeltaTime();
		Bone.Location += (SpringForce + DampingForce) * DT + GravityInSimSpace * DT * DT;
	}
}

// Skip non-moving bones for performance optimization
void FAnimNode_KawaiiPhysics::ApplyBoneFilter()
{
	if (SkipBoneNames.Num() == 0)
	{
		return;
	}

	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Check if bone should be skipped
		for (const FName& SkipBone : SkipBoneNames)
		{
			if (Bone.BoneRef.BoneName == SkipBone)
			{
				Bone.bSkipSimulate = true;
				break;
			}
		}
	}
}

// Auto-generate collision primitives from skeletal mesh bounds
void FAnimNode_KawaiiPhysics::AutoGenerateCollisions(const USkeletalMeshComponent* SkelComp, const TArray<FName>& ChainRoots)
{
	if (!SkelComp || ChainRoots.Num() == 0)
	{
		return;
	}

	const USkeletalMesh* SkeletalMesh = SkelComp->GetSkeletalMeshAsset();
	if (!SkeletalMesh)
	{
		return;
	}

	// Get reference skeleton and skeleton
	const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
	const USkeleton* Skeleton = SkeletalMesh->GetSkeleton();

	// Clear existing collisions
	SphericalLimits.Empty();
	CapsuleLimits.Empty();

	for (const FName& ChainRoot : ChainRoots)
	{
		const int32 RootIdx = RefSkeleton.FindBoneIndex(ChainRoot);
		if (RootIdx < 0)
		{
			continue;
		}

		// Get bone transform
		const FTransform BoneTransform = SkelComp->GetBoneTransform(RootIdx);
		const FVector BoneLocation = BoneTransform.GetLocation();

		// Get bone's children to determine extent
		TArray<int32> Children;
		RefSkeleton.GetDirectChildBones(RootIdx, Children);

		if (Children.Num() > 0)
		{
			// Create capsule along bone direction
			const int32 ChildIdx = Children[0];
			const FTransform ChildTransform = SkelComp->GetBoneTransform(ChildIdx);
			const FVector ChildLocation = ChildTransform.GetLocation();

			const FVector BoneDir = ChildLocation - BoneLocation;
			const float BoneLength = BoneDir.Size();

			if (BoneLength > KINDA_SMALL_NUMBER)
			{
				FCapsuleLimit Capsule;
				Capsule.DrivingBone.BoneName = ChainRoot;
				Capsule.DrivingBone.Initialize(Skeleton);
				Capsule.Location = BoneLocation + BoneDir * 0.5f;
				Capsule.Rotation = FRotationMatrix::MakeFromZ(BoneDir).ToQuat();
				Capsule.Radius = FMath::Max(BoneLength * 0.3f, 2.0f);
				Capsule.Length = BoneLength;
				Capsule.bEnable = true;
				CapsuleLimits.Add(Capsule);
			}
		}
		else
		{
			// Create sphere for leaf bones
			FSphericalLimit Sphere;
			Sphere.DrivingBone.BoneName = ChainRoot;
			Sphere.DrivingBone.Initialize(Skeleton);
			Sphere.Location = BoneLocation;
			Sphere.Radius = 3.0f;
			Sphere.LimitType = ESphericalLimitType::Outer;
			Sphere.bEnable = true;
			SphericalLimits.Add(Sphere);
		}
	}
}

void FAnimNode_KawaiiPhysics::Simulate(FKawaiiPhysicsModifyBone& Bone, const FSceneInterface* Scene,
                                       const FTransform& ComponentTransform,
                                       const float& Exponent, const USkeletalMeshComponent* SkelComp,
                                       FComponentSpacePoseContext& Output)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_Simulate);

	const FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];

	// wind（Output依存）を計算。減衰後の速度に足し込む寄与として ComputeVerletStepVelocity へ渡す。
	// Wind (Output-dependent). Passed to ComputeVerletStepVelocity as a contribution added onto the post-damping velocity.
	FVector WindVelocity = FVector::ZeroVector;
	if (bEnableWind && Scene)
	{
		WindVelocity = GetWindVelocity(Output, Scene, Bone) * GetEffectiveTargetFramerate();
	}

	// このステップの速度を作る（速度再構成 → damping → +wind → gravity）。
	// Build this step's velocity (reconstruct -> damping -> +wind -> gravity).
	FVector Velocity = ComputeVerletStepVelocity(Bone, WindVelocity);

	// ユーザー外力に「実際の速度」を渡す（gravity の後・位置更新の前）。ApplyToVelocity は
	// InOutVelocity を読む実装もあり得るため、集約せずここで実速度に対して呼ぶ。
	// Hand the real velocity to user external forces (after gravity, before position integration).
	// ApplyToVelocity may read InOutVelocity, so it runs on the real velocity here (not on a pre-gathered accumulator).
	for (int i = 0; i < ExternalForces.Num(); ++i)
	{
		if (ExternalForces[i].IsValid())
		{
			if (const auto ExForce = ExternalForces[i].GetMutablePtr<FKawaiiPhysics_ExternalForce>();
				ExForce->bIsEnabled)
			{
				ExForce->ApplyToVelocity(Bone, *this, Output, Velocity);
			}
		}
	}

	// 速度のぶんだけ位置を進める。 / Move the position by the velocity.
	IntegrateVerletStepPosition(Bone, Velocity);

	// Simple External Force（速度を経由しない位置オフセット） / position offset, not via velocity.
	ApplySimpleExternalForce(Bone);

	// Follow World Movement
	if (SimulationSpace == EKawaiiPhysicsSimulationSpace::BaseBoneSpace
		&& TeleportType != ETeleportType::TeleportPhysics)
	{
		// BaseBoneSpace は Output 依存の空間変換が必要なため、別関数に切り出さずここで実行。
		// BaseBoneSpace needs Output-dependent space conversions, so it is handled inline here.

		// Velocity-based inertia dampening: maps bone velocity → additional damping multiplier
		float InertiaDampingMultiplier = 0.0f;
		const FRichCurve* InertiaCurve = InertiaDampingVelocityCurve.GetRichCurveConst();
		if (InertiaCurve && InertiaCurve->GetNumKeys() > 0)
		{
			const float BoneVelocity = (Bone.Location - Bone.PrevLocation).Size() / FMath::Max(GetStepDeltaTime(), SMALL_NUMBER);
			const float ClampedVelocity = FMath::Clamp(BoneVelocity, 0.0f, InertiaDampingMaxVelocity);
			InertiaDampingMultiplier = InertiaCurve->Eval(ClampedVelocity);
		}

		// Follow Translation
		const FVector SkelCompMoveVectorBBS =
			ConvertSimulationSpaceVector(Output, EKawaiiPhysicsSimulationSpace::ComponentSpace,
			                             EKawaiiPhysicsSimulationSpace::BaseBoneSpace, SkelCompMoveVector);
		const float EffectiveLocationDamping = FMath::Clamp(
			Bone.PhysicsSettings.WorldDampingLocation + InertiaDampingMultiplier, 0.0f, 1.0f);
		Bone.Location += SkelCompMoveVectorBBS * (1.0f - EffectiveLocationDamping);

		// Follow Rotation
		const FVector PrevLocationCS = PrevBaseBoneSpace2ComponentSpace.TransformPosition(Bone.PrevLocation);
		const FVector RotatedLocationCS = SkelCompMoveRotation.RotateVector(PrevLocationCS);
		const FVector RotatedLocationBase = ConvertSimulationSpaceLocationCached(
			FSimulationSpaceCache(), CurrentEvalSimSpaceCache, RotatedLocationCS);
		const float EffectiveRotationDamping = FMath::Clamp(
			Bone.PhysicsSettings.WorldDampingRotation + InertiaDampingMultiplier, 0.0f, 1.0f);
		Bone.Location += (RotatedLocationBase - Bone.PrevLocation) * (1.0f - EffectiveRotationDamping);
	}
	else
	{
		// ComponentSpace / WorldSpace（BaseBoneSpace 以外） / Component & World space (non-BaseBoneSpace).
		ApplyWorldMoveFollowNonBaseBone(Bone);
	}

	// External Force
	// NOTE: if use foreach, you may get issue ( Array has changed during ranged-for iteration )
	for (int i = 0; i < CustomExternalForces.Num(); ++i)
	{
		if (CustomExternalForces[i] && CustomExternalForces[i]->bIsEnabled)
		{
			FTransform BoneTM = FTransform::Identity;
			if (Bone.bDummy)
			{
				// inter-bone dummy / 分割末端dummy は実親ボーンのTransformを使用（親がまた別のdummyでBoneRef空のときにクラッシュ防止）
				// Inter-bone & subdivided tip dummies use the real parent transform (prevents crash when the parent is another dummy with empty BoneRef)
				const FKawaiiPhysicsModifyBone& TransformBone = ModifyBones.IsValidIndex(Bone.InterBoneRealParentIndex)
					? ModifyBones[Bone.InterBoneRealParentIndex]
					: ParentBone;
				const FCompactPoseBoneIndex TransformCPI =
					TransformBone.BoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				if (TransformCPI >= 0)
				{
					BoneTM = GetBoneTransformInSimSpace(Output, TransformCPI);
				}
				// else: 無効CompactPose(LOD等) → BoneTMはIdentityのまま / invalid CompactPose → keep Identity
			}
			else
			{
				BoneTM = GetBoneTransformInSimSpace(
					Output, Bone.BoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()));
			}

			CustomExternalForces[i]->Apply(*this, Bone.Index, SkelComp, BoneTM);
		}
	}

	for (int i = 0; i < ExternalForces.Num(); ++i)
	{
		if (ExternalForces[i].IsValid())
		{
			if (const auto ExForce = ExternalForces[i].GetMutablePtr<FKawaiiPhysics_ExternalForce>();
				ExForce->bIsEnabled)
			{
				if (ExForce->ExternalForceSpace == EKawaiiPhysicsSimulationSpace::BaseBoneSpace)
				{
					FTransform BoneTM = FTransform::Identity;
					if (Bone.bDummy)
					{
						// inter-bone dummy / 分割末端dummy は実親ボーンのTransformを使用（BoneRef空クラッシュ防止）
						// Inter-bone & subdivided tip dummies use the real parent transform (prevents empty-BoneRef crash)
						const FKawaiiPhysicsModifyBone& TransformBone = ModifyBones.IsValidIndex(Bone.InterBoneRealParentIndex)
							? ModifyBones[Bone.InterBoneRealParentIndex]
							: ParentBone;
						const FCompactPoseBoneIndex TransformCPI =
							TransformBone.BoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
						if (TransformCPI >= 0)
						{
							BoneTM = GetBoneTransformInSimSpace(Output, TransformCPI);
						}
						// else: 無効CompactPose(LOD等) → BoneTMはIdentityのまま / invalid CompactPose → keep Identity
					}
					else
					{
						BoneTM = GetBoneTransformInSimSpace(
							Output, Bone.BoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()));
					}

					ExForce->Apply(Bone, *this, Output, BoneTM);
				}
				else
				{
					ExForce->Apply(Bone, *this, Output);
				}
			}
		}
	}

	// Pull to Pose Location（剛性） / Pull toward pose location (stiffness).
	ApplyStiffnessPull(Bone, ParentBone, Exponent);
}

// ============================================================================
//  物理計算の各ステップ（引数に FComponentSpacePoseContext を取らない）。Simulate() から呼ばれる。
//  Each physics step; takes no FComponentSpacePoseContext. Called from Simulate().
//  注: ここを変更したら、Simulate() 内の wind/ApplyToVelocity の呼び出し位置との整合も確認すること。
//  NOTE: if you change these, re-check consistency with the wind/ApplyToVelocity call sites in Simulate().
// ============================================================================

FVector FAnimNode_KawaiiPhysics::ComputeVerletStepVelocity(FKawaiiPhysicsModifyBone& Bone,
                                                     const FVector& WindVelocity)
{
	// Move using Velocity( = movement amount in pre frame ) and Damping
	// 速度は前ステップ変位を DeltaTimeOld で割って再構成。固定サブステップ時 DeltaTimeOld=FixedDt。
	// Velocity is reconstructed from the previous step's displacement / DeltaTimeOld (= FixedDt while substepping).
	FVector Velocity = (DeltaTimeOld > SMALL_NUMBER)
		? (Bone.Location - Bone.PrevLocation) / DeltaTimeOld
		: FVector::ZeroVector;
	Bone.PrevPrevLocation = Bone.PrevLocation;
	Bone.PrevLocation = Bone.Location;

	// 毎ステップ生の damping 係数（固定サブステップ化でフレームレート依存は解消済み）。
	// Raw per-step damping (frame-rate dependence is resolved by fixed substepping).
	Velocity *= (1.0f - Bone.PhysicsSettings.Damping);

	// wind（呼び出し元で計算済み） / wind (computed by the caller).
	Velocity += WindVelocity;

	// Gravity (apply just after wind; keep legacy compatibility via separate position term)
	const float StepDt = GetStepDeltaTime();
	if (!bUseLegacyGravity)
	{
		// AnimDynamics-like: integrate acceleration into velocity
		Velocity += GravityInSimSpace * StepDt;
	}
	else
	{
		// Legacy gravity: add 0.5 * g * dt^2 to position
		Bone.Location += 0.5 * GravityInSimSpace * StepDt * StepDt;
	}

	return Velocity;
}

void FAnimNode_KawaiiPhysics::IntegrateVerletStepPosition(FKawaiiPhysicsModifyBone& Bone, const FVector& Velocity)
{
	// Integrate position from velocity
	Bone.Location += Velocity * GetStepDeltaTime();
}

void FAnimNode_KawaiiPhysics::ApplySimpleExternalForce(FKawaiiPhysicsModifyBone& Bone)
{
	// Simple External Force（速度を経由しない位置オフセット。SimulateModifyBones でキャッシュ済み）。
	// world-move / stiffness と同じ「位置空間の後処理」なので Verlet ステップから分離している。
	// Simple external force: a position offset that does NOT go through Velocity (cached in SimulateModifyBones).
	// Split out from the Verlet step because it is a position-space post-op like world-move / stiffness.
	if (!SimpleExternalForceInSimSpace.IsNearlyZero())
	{
		Bone.Location += SimpleExternalForceInSimSpace * GetStepDeltaTime();
	}
}

void FAnimNode_KawaiiPhysics::ApplyWorldMoveFollowNonBaseBone(FKawaiiPhysicsModifyBone& Bone)
{
	// Follow World Movement（ComponentSpace/WorldSpace のみ。BaseBoneSpaceは Simulate() で別処理）
	// World-movement follow for Component/World space only (BaseBoneSpace handled inline in Simulate()).
	if (SimulationSpace != EKawaiiPhysicsSimulationSpace::WorldSpace
		&& TeleportType != ETeleportType::TeleportPhysics)
	{
		// Velocity-based inertia dampening
		float InertiaDampingMultiplier = 0.0f;
		const FRichCurve* InertiaCurve = InertiaDampingVelocityCurve.GetRichCurveConst();
		if (InertiaCurve && InertiaCurve->GetNumKeys() > 0)
		{
			const float BoneVelocity = (Bone.Location - Bone.PrevLocation).Size() / FMath::Max(GetStepDeltaTime(), SMALL_NUMBER);
			const float ClampedVelocity = FMath::Clamp(BoneVelocity, 0.0f, InertiaDampingMaxVelocity);
			InertiaDampingMultiplier = InertiaCurve->Eval(ClampedVelocity);
		}

		// Predictive physics: apply predicted movement to reduce latency
		const FVector PredictedMove = SkelCompMoveVector * PredictionScale;

		// Follow Translation
		const float EffectiveLocationDamping = FMath::Clamp(
			Bone.PhysicsSettings.WorldDampingLocation + InertiaDampingMultiplier, 0.0f, 1.0f);
		Bone.Location += PredictedMove * (1.0f - EffectiveLocationDamping);

		// Follow Rotation
		const float EffectiveRotationDamping = FMath::Clamp(
			Bone.PhysicsSettings.WorldDampingRotation + InertiaDampingMultiplier, 0.0f, 1.0f);
		Bone.Location += (SkelCompMoveRotation.RotateVector(Bone.PrevLocation) - Bone.PrevLocation)
			* (1.0f - EffectiveRotationDamping);
	}
}

void FAnimNode_KawaiiPhysics::ApplyStiffnessPull(FKawaiiPhysicsModifyBone& Bone,
                                                 const FKawaiiPhysicsModifyBone& ParentBone, float Exponent)
{
	// Pull to Pose Location
	const FVector BaseLocation = ParentBone.Location + (Bone.PoseLocation - ParentBone.PoseLocation);
	Bone.Location += (BaseLocation - Bone.Location) *
		(1.0f - FMath::Pow(1.0f - Bone.PhysicsSettings.Stiffness, Exponent));
}

FVector FAnimNode_KawaiiPhysics::GetWindVelocity(FComponentSpacePoseContext& Output, const FSceneInterface* Scene,
                                                 const FKawaiiPhysicsModifyBone& Bone) const
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_GetWindVelocity);

	if (WindScale == 0.0f || !Scene)
	{
		return FVector::ZeroVector;
	}

	FVector WindDirection = FVector::ZeroVector;
	float WindSpeed = 0.0f;
	float WindMinGust = 0.0f;
	float WindMaxGust = 0.0f;

	Scene->GetWindParameters(
		ConvertSimulationSpaceLocation(Output, SimulationSpace, EKawaiiPhysicsSimulationSpace::WorldSpace,
		                               Bone.PoseLocation),
		WindDirection, WindSpeed, WindMinGust, WindMaxGust);

	WindDirection =
		ConvertSimulationSpaceVector(Output, EKawaiiPhysicsSimulationSpace::WorldSpace, SimulationSpace, WindDirection);
	if (WindDirectionNoiseAngle > 0)
	{
		WindDirection = FMath::VRandCone(WindDirection, FMath::DegreesToRadians(WindDirectionNoiseAngle));
	}

	FVector WindVelocity = WindDirection * WindSpeed * WindScale;

	// Use deterministic noise to avoid visual divergence in networked play
	{
		const float NoisePhase = (float)(FrameCount % 1000) * 0.001f;
		WindVelocity *= (1.0f + FMath::Sin(NoisePhase * UE_TWO_PI) * 0.5f);
	}

	return WindVelocity;
}

void FAnimNode_KawaiiPhysics::WarmUp(FComponentSpacePoseContext& Output, const FBoneContainer& BoneContainer,
                                     FTransform& ComponentTransform)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_WarmUp);

	// サブステップ有効時は、各ウォームアップ反復がちょうど1固定ステップになるよう DeltaTime=FixedDt とし、
	// アキュムレータを退避（ウォームアップ分を本流の蓄積に混ぜない）。ポーズは現値固定で安定化させる。
	// When substepping, force DeltaTime = FixedDt so each warm-up iteration runs exactly one fixed step,
	// and isolate the accumulator from the main loop. Pose is held at the current value to settle.
	const UKawaiiPhysicsDeveloperSettings* KawaiiSettings = GetDefault<UKawaiiPhysicsDeveloperSettings>();
	const bool bSubstep = KawaiiSettings && KawaiiSettings->bUseFixedSubstepping;
	const float SavedDeltaTime = DeltaTime;
	const float SavedAccumulator = SubstepAccumulator;
	if (bSubstep)
	{
		DeltaTime = 1.0f / GetEffectiveTargetFramerate();
		SubstepAccumulator = 0.0f;
	}

	for (int32 i = 0; i < WarmUpFrames; ++i)
	{
		SimulateModifyBones(Output, ComponentTransform);
	}

	if (bSubstep)
	{
		DeltaTime = SavedDeltaTime;
		SubstepAccumulator = SavedAccumulator;
	}

	// 2.5D Anime Mode: constrain bone movement to a plane
	if (bEnable2DAnimeMode)
	{
		const FVector PlaneNormal = ComponentTransform.TransformVectorNoScale(AnimePlaneNormal).GetSafeNormal();
		for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
		{
			if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
			{
				continue;
			}
			// Project simulated position onto plane relative to pose position
			const FVector Offset = Bone.Location - Bone.PoseLocation;
			const FVector ProjectedOffset = FVector::VectorPlaneProject(Offset, PlaneNormal);
			Bone.Location = Bone.PoseLocation + FMath::Lerp(Offset, ProjectedOffset, AnimePlaneBlend);
		}
	}

	// Transfer Motion: angular velocity of source bones creates external forces
	if (bEnableTransferMotion && TransferMotionSources.Num() > 0)
	{
		const FBoneContainer& TransferBoneContainer = Output.Pose.GetPose().GetBoneContainer();
		for (FKawaiiPhysicsTransferMotionSource& Source : TransferMotionSources)
		{
			if (!Source.SourceBoneRef.IsValidToEvaluate(TransferBoneContainer))
			{
				continue;
			}

			const FCompactPoseBoneIndex CompactIdx = TransferBoneContainer.GetCompactPoseIndexFromSkeletonIndex(
				Source.SourceBoneRef.BoneIndex);
			if (CompactIdx < 0)
			{
				continue;
			}

			const FQuat CurrRot = Output.Pose.GetComponentSpaceTransform(CompactIdx).GetRotation();

			if (!Source.bInitialized)
			{
				Source.PrevRotation = CurrRot;
				Source.bInitialized = true;
				continue;
			}

			// Compute angular velocity
			FQuat DeltaRot = CurrRot * Source.PrevRotation.Inverse();
			FVector Axis;
			float Angle;
			DeltaRot.ToAxisAndAngle(Axis, Angle);
			float AngVel = FMath::RadiansToDegrees(Angle) / FMath::Max(FrameDeltaTime, KINDA_SMALL_NUMBER);
			AngVel = FMath::Min(AngVel, Source.MaxAngularVelocity);

			// Apply as external force to all simulated bones
			const FVector TransferForce = Axis * AngVel * Source.Influence;
			const float DT = GetStepDeltaTime();
			for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
			{
				if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
				{
					continue;
				}
				Bone.Location += TransferForce * DT;
			}

			Source.PrevRotation = CurrRot;
		}
	}

	// Track end time for budget allocator
	if (bEnableBudgetAllocator)
	{
		LastFrameEndTime = FPlatformTime::Seconds() * 1000000.0;
	}

	// Physics Sleep: check if we should sleep next frame
	ApplyPhysicsSleep();
}

// Physics Sleep: freeze simulation when all bones are nearly still
void FAnimNode_KawaiiPhysics::ApplyPhysicsSleep()
{
	if (!bEnablePhysicsSleep)
	{
		return;
	}

	// Check if all bones are below velocity threshold
	bool bAllStill = true;
	for (const FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}
		const float Velocity = (Bone.Location - Bone.PrevLocation).Size() / FMath::Max(GetStepDeltaTime(), KINDA_SMALL_NUMBER);
		if (Velocity > SleepVelocityThreshold)
		{
			bAllStill = false;
			break;
		}
	}

	if (bAllStill)
	{
		SleepFrameCounter++;
		if (SleepFrameCounter >= SleepFrameCount)
		{
			bIsAsleep = true;
		}
	}
	else
	{
		SleepFrameCounter = 0;
		bIsAsleep = false;
	}
}

// CCD (Edge Collision): swept sphere between previous and current bone positions
void FAnimNode_KawaiiPhysics::ApplyCCD()
{
	if (!bEnableCCD || ModifyBones.Num() < 2)
	{
		return;
	}

	const float SphereRadius = CCDSphereRadius;

	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}

		// Swept sphere: check if bone moved through any collision primitive
		const FVector Start = Bone.PrevLocation;
		const FVector End = Bone.Location;
		const FVector SweepDelta = End - Start;
		const float SweepDistSq = SweepDelta.SizeSquared();
		if (SweepDistSq < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// Check against spherical limits
		for (const FSphericalLimit& Sphere : SphericalLimits)
		{
			if (!Sphere.bEnable)
			{
				continue;
			}

			const FVector SphereCenter = Sphere.Location;
			const float SphereRadiusTotal = Sphere.Radius + SphereRadius;

			// Closest point on sweep line to sphere center
			const float t = FMath::Clamp(FVector::DotProduct(SphereCenter - Start, SweepDelta) / SweepDistSq, 0.0f, 1.0f);
			const FVector ClosestPoint = Start + SweepDelta * t;
			const float DistSq = (ClosestPoint - SphereCenter).SizeSquared();

			if (DistSq < SphereRadiusTotal * SphereRadiusTotal)
			{
				// Push bone out of collision
				const float Dist = FMath::Sqrt(DistSq);
				if (Dist > KINDA_SMALL_NUMBER)
				{
					const FVector PushDir = (ClosestPoint - SphereCenter) / Dist;
					Bone.Location = SphereCenter + PushDir * SphereRadiusTotal;
				}
			}
		}

		// Check against capsule limits
		for (const FCapsuleLimit& Capsule : CapsuleLimits)
		{
			if (!Capsule.bEnable)
			{
				continue;
			}

			// Capsule as line segment
			const FVector CapsuleStart = Capsule.Location + Capsule.Rotation.GetUpVector() * (-Capsule.Length * 0.5f);
			const FVector CapsuleEnd = Capsule.Location + Capsule.Rotation.GetUpVector() * (Capsule.Length * 0.5f);
			const float CapsuleRadiusTotal = Capsule.Radius + SphereRadius;

			// Find closest point on sweep line to capsule line (closest-point-on-two-segments)
			const FVector Dir1 = SweepDelta;
			const FVector Dir2 = CapsuleEnd - CapsuleStart;
			const FVector d = Start - CapsuleStart;
			const float a = FVector::DotProduct(Dir1, Dir1); // |Dir1|²
			const float b = FVector::DotProduct(Dir1, Dir2);
			const float c = FVector::DotProduct(Dir2, Dir2); // |Dir2|²
			const float e = FVector::DotProduct(Dir1, d);
			const float f = FVector::DotProduct(Dir2, d);
			const float Denom = a * c - b * b;

			float s, t;
			if (Denom < KINDA_SMALL_NUMBER)
			{
				// Segments nearly parallel
				s = 0.0f;
				t = FMath::Clamp(e / FMath::Max(a, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
			}
			else
			{
				s = FMath::Clamp((b * e - a * f) / Denom, 0.0f, 1.0f);
				t = FMath::Clamp((b * s + e) / FMath::Max(a, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
			}

			const FVector ClosestOnSweep = Start + SweepDelta * t;
			const FVector ClosestOnCapsule = CapsuleStart + Dir2 * s;
			const float DistSq = (ClosestOnSweep - ClosestOnCapsule).SizeSquared();

			if (DistSq < CapsuleRadiusTotal * CapsuleRadiusTotal)
			{
				const float Dist = FMath::Sqrt(DistSq);
				if (Dist > KINDA_SMALL_NUMBER)
				{
					const FVector PushDir = (ClosestOnSweep - ClosestOnCapsule) / Dist;
					Bone.Location = ClosestOnCapsule + PushDir * CapsuleRadiusTotal;
				}
			}
		}
	}
}

// Backstop Collision: per-bone invisible hemisphere preventing bone penetration into character mesh
void FAnimNode_KawaiiPhysics::ApplyBackstopCollision()
{
	if (!bEnableBackstopCollision || ModifyBones.Num() < 2)
	{
		return;
	}

	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		if (Bone.bSkipSimulate || Bone.bDummy || Bone.bBridgeDummy)
		{
			continue;
		}
		if (!Bone.HasParent() || !ModifyBones.IsValidIndex(Bone.ParentIndex))
		{
			continue;
		}

		const FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];

		// Backstop direction: from parent toward bone (the "outward" direction from the body)
		const FVector BackstopDir = (Bone.Location - ParentBone.Location).GetSafeNormal();
		if (BackstopDir.IsNearlyZero())
		{
			continue;
		}

		// The bone should not go BEHIND the parent (into the body)
		// Check if bone has moved behind its parent relative to the rest pose
		const FVector RestDir = (Bone.PoseLocation - ParentBone.PoseLocation).GetSafeNormal();
		const float BehindAmount = FVector::DotProduct(Bone.Location - ParentBone.Location, RestDir);

		// If bone is too close to parent (behind the backstop plane), push it out
		const float MinDist = BackstopRadius * (1.0f - BackstopStrength);
		if (BehindAmount < MinDist)
		{
			const FVector PushOut = RestDir * (MinDist - BehindAmount);
			Bone.Location += PushOut * BackstopStrength;
		}
	}
}

// Convert simulated bone positions back to component space and write to OutBoneTransforms.
// For single-child bones, preserves bone length by projecting simulated position onto the
// original pose direction from parent. This prevents chain collapse when damping/stiffness
// are aggressive.
void FAnimNode_KawaiiPhysics::ApplySimulateResult(FComponentSpacePoseContext& Output,
                                                  const FBoneContainer& BoneContainer,
                                                  TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_ApplySimulateResult);

	for (int32 i = 0; i < ModifyBones.Num(); ++i)
	{
		FTransform PoseTransform = FTransform(ModifyBones[i].PoseRotation, ModifyBones[i].PoseLocation,
		                                      ModifyBones[i].PoseScale);
		// Apply bone scaling (computed by ApplyBoneScaling per-bone when BoneScaleCompensation > 0)
		PoseTransform.SetScale3D(ModifyBones[i].BoneScale);
		PoseTransform =
			ConvertSimulationSpaceTransform(Output, SimulationSpace, EKawaiiPhysicsSimulationSpace::ComponentSpace,
			                                PoseTransform);
		OutBoneTransforms.Add(FBoneTransform(ModifyBones[i].BoneRef.GetCompactPoseIndex(BoneContainer), PoseTransform));
	}


	TSet<int32> ProcessedParents;
	for (int32 i = 0; i < ModifyBones.Num(); ++i)
	{
		FKawaiiPhysicsModifyBone& Bone = ModifyBones[i];
		if (!Bone.HasParent() || !ModifyBones.IsValidIndex(Bone.ParentIndex))
		{
			continue;
		}

		FKawaiiPhysicsModifyBone& ParentBone = ModifyBones[Bone.ParentIndex];

		if (ParentBone.ChildIndices.Num() <= 1 && !ProcessedParents.Contains(Bone.ParentIndex))
		{
			if (ParentBone.BoneRef.BoneIndex >= 0)
			{
				FVector PoseVector = Bone.PoseLocation - ParentBone.PoseLocation;
				FVector SimulateVector = Bone.Location - ParentBone.Location;

				if ((PoseVector.GetSafeNormal() - SimulateVector.GetSafeNormal()).IsNearlyZero(0.01f))
				{
					ProcessedParents.Add(Bone.ParentIndex);
					continue;
				}

				if (BoneForwardAxis == EBoneForwardAxis::X_Negative || BoneForwardAxis == EBoneForwardAxis::Y_Negative
					|| BoneForwardAxis == EBoneForwardAxis::Z_Negative)
				{
					PoseVector *= -1;
					SimulateVector *= -1;
				}

				FQuat SimulateRotation =
					FQuat::FindBetweenVectors(PoseVector, SimulateVector) * ParentBone.PoseRotation;
				ParentBone.PrevRotation = SimulateRotation;

				SimulateRotation =
					ConvertSimulationSpaceRotation(Output, SimulationSpace,
					                               EKawaiiPhysicsSimulationSpace::ComponentSpace, SimulateRotation);
				if (OutBoneTransforms.IsValidIndex(Bone.ParentIndex))
				{
					OutBoneTransforms[Bone.ParentIndex].Transform.SetRotation(SimulateRotation);
				}
				ProcessedParents.Add(Bone.ParentIndex);
			}
		}

		if (Bone.BoneRef.BoneIndex >= 0 && !Bone.bDummy)
		{
			OutBoneTransforms[i].Transform.SetLocation(
				ConvertSimulationSpaceLocation(Output, SimulationSpace, EKawaiiPhysicsSimulationSpace::ComponentSpace,
				                               Bone.Location));
		}
	}

	OutBoneTransforms.RemoveAll([](const FBoneTransform& BoneTransform)
	{
		return BoneTransform.BoneIndex < 0;
	});

	// for check in FCSPose<PoseType>::LocalBlendCSBoneTransforms
	OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

FTransform FAnimNode_KawaiiPhysics::GetBoneTransformInSimSpace(FComponentSpacePoseContext& Output,
                                                               const FCompactPoseBoneIndex& BoneIndex) const
{
	const FSimulationSpaceCache CacheFrom = GetSimulationSpaceCacheFor(
		Output, EKawaiiPhysicsSimulationSpace::ComponentSpace);
	const FSimulationSpaceCache CacheTo = GetSimulationSpaceCacheFor(Output, SimulationSpace);
	return ConvertSimulationSpaceTransformCached(CacheFrom, CacheTo, Output.Pose.GetComponentSpaceTransform(BoneIndex));
}

FAnimNode_KawaiiPhysics::FSimulationSpaceCache FAnimNode_KawaiiPhysics::GetSimulationSpaceCacheFor(
	FComponentSpacePoseContext& Output,
	const EKawaiiPhysicsSimulationSpace Space) const
{
	if (Space == EKawaiiPhysicsSimulationSpace::ComponentSpace)
	{
		return FSimulationSpaceCache();
	}
	if (bHasCurrentEvalSimSpaceCache && Space == SimulationSpace)
	{
		return CurrentEvalSimSpaceCache;
	}
	if (bHasCurrentEvalWorldSpaceCache && Space == EKawaiiPhysicsSimulationSpace::WorldSpace)
	{
		return CurrentEvalWorldSpaceCache;
	}

	const UEnum* EnumPtr = StaticEnum<EKawaiiPhysicsSimulationSpace>();
	UE_LOG(LogKawaiiPhysicsExtension, Verbose, TEXT("Building Simulation Space Cache for %s"),
	       *EnumPtr->GetNameStringByValue(static_cast<int64>(Space)));
	return BuildSimulationSpaceCache(Output, Space);
}

FTransform FAnimNode_KawaiiPhysics::ConvertSimulationSpaceTransformCached(
	const FSimulationSpaceCache& CacheFrom,
	const FSimulationSpaceCache& CacheTo,
	const FTransform& InTransform) const
{
	FTransform ResultTransform = InTransform;
	ResultTransform = ResultTransform * CacheFrom.TargetSpaceToComponent;
	ResultTransform = ResultTransform * CacheTo.ComponentToTargetSpace;
	return ResultTransform;
}

FVector FAnimNode_KawaiiPhysics::ConvertSimulationSpaceVectorCached(
	const FSimulationSpaceCache& CacheFrom,
	const FSimulationSpaceCache& CacheTo,
	const FVector& InVector) const
{
	FVector ResultVector = InVector;
	ResultVector = CacheFrom.TargetSpaceToComponent.TransformVector(ResultVector);
	ResultVector = CacheTo.ComponentToTargetSpace.TransformVector(ResultVector);
	return ResultVector;
}

FVector FAnimNode_KawaiiPhysics::ConvertSimulationSpaceLocationCached(
	const FSimulationSpaceCache& CacheFrom,
	const FSimulationSpaceCache& CacheTo,
	const FVector& InLocation) const
{
	FVector ResultLocation = InLocation;
	ResultLocation = CacheFrom.TargetSpaceToComponent.TransformPosition(ResultLocation);
	ResultLocation = CacheTo.ComponentToTargetSpace.TransformPosition(ResultLocation);
	return ResultLocation;
}

FQuat FAnimNode_KawaiiPhysics::ConvertSimulationSpaceRotationCached(
	const FSimulationSpaceCache& CacheFrom,
	const FSimulationSpaceCache& CacheTo,
	const FQuat& InRotation) const
{
	FQuat ResultRotation = InRotation;
	ResultRotation = CacheFrom.TargetSpaceToComponent.TransformRotation(ResultRotation);
	ResultRotation = CacheTo.ComponentToTargetSpace.TransformRotation(ResultRotation);
	return ResultRotation;
}

void FAnimNode_KawaiiPhysics::ConvertSimulationSpaceCached(
	const FSimulationSpaceCache& CacheFrom,
	const FSimulationSpaceCache& CacheTo,
	EKawaiiPhysicsSimulationSpace From,
	EKawaiiPhysicsSimulationSpace To)
{
	for (FKawaiiPhysicsModifyBone& Bone : ModifyBones)
	{
		Bone.Location = CacheTo.ComponentToTargetSpace.TransformPosition(
			CacheFrom.TargetSpaceToComponent.TransformPosition(Bone.Location));
		Bone.PrevLocation = CacheTo.ComponentToTargetSpace.TransformPosition(
			CacheFrom.TargetSpaceToComponent.TransformPosition(Bone.PrevLocation));

		Bone.PrevRotation = CacheTo.ComponentToTargetSpace.TransformRotation(
			CacheFrom.TargetSpaceToComponent.TransformRotation(Bone.PrevRotation));
	}
}

FTransform FAnimNode_KawaiiPhysics::ConvertSimulationSpaceTransform(FComponentSpacePoseContext& Output,
                                                                    const EKawaiiPhysicsSimulationSpace From,
                                                                    const EKawaiiPhysicsSimulationSpace To,
                                                                    const FTransform& InTransform) const
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_ConvertSimulationSpaceTransform);
	if (From == To)
	{
		return InTransform;
	}

	const FSimulationSpaceCache CacheFrom = GetSimulationSpaceCacheFor(Output, From);
	const FSimulationSpaceCache CacheTo = GetSimulationSpaceCacheFor(Output, To);
	return ConvertSimulationSpaceTransformCached(CacheFrom, CacheTo, InTransform);
}

FVector FAnimNode_KawaiiPhysics::ConvertSimulationSpaceVector(FComponentSpacePoseContext& Output,
                                                              const EKawaiiPhysicsSimulationSpace From,
                                                              const EKawaiiPhysicsSimulationSpace To,
                                                              const FVector& InVector) const
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_ConvertSimulationSpaceVector);
	if (From == To)
	{
		return InVector;
	}

	const FSimulationSpaceCache CacheFrom = GetSimulationSpaceCacheFor(Output, From);
	const FSimulationSpaceCache CacheTo = GetSimulationSpaceCacheFor(Output, To);
	return ConvertSimulationSpaceVectorCached(CacheFrom, CacheTo, InVector);
}

FVector FAnimNode_KawaiiPhysics::ConvertSimulationSpaceLocation(FComponentSpacePoseContext& Output,
                                                                const EKawaiiPhysicsSimulationSpace From,
                                                                const EKawaiiPhysicsSimulationSpace To,
                                                                const FVector& InLocation) const
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_ConvertSimulationSpaceLocation);
	if (From == To)
	{
		return InLocation;
	}

	const FSimulationSpaceCache CacheFrom = GetSimulationSpaceCacheFor(Output, From);
	const FSimulationSpaceCache CacheTo = GetSimulationSpaceCacheFor(Output, To);
	return ConvertSimulationSpaceLocationCached(CacheFrom, CacheTo, InLocation);
}

FQuat FAnimNode_KawaiiPhysics::ConvertSimulationSpaceRotation(FComponentSpacePoseContext& Output,
                                                              const EKawaiiPhysicsSimulationSpace From,
                                                              const EKawaiiPhysicsSimulationSpace To,
                                                              const FQuat& InRotation) const
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_ConvertSimulationSpaceRotation);
	if (From == To)
	{
		return InRotation;
	}

	const FSimulationSpaceCache CacheFrom = GetSimulationSpaceCacheFor(Output, From);
	const FSimulationSpaceCache CacheTo = GetSimulationSpaceCacheFor(Output, To);
	return ConvertSimulationSpaceRotationCached(CacheFrom, CacheTo, InRotation);
}

void FAnimNode_KawaiiPhysics::ConvertSimulationSpace(FComponentSpacePoseContext& Output,
                                                     const EKawaiiPhysicsSimulationSpace From,
                                                     const EKawaiiPhysicsSimulationSpace To)
{
	SCOPE_CYCLE_COUNTER(STAT_KawaiiPhysics_ConvertSimulationSpace);
	if (From == To)
	{
		return;
	}

	const FSimulationSpaceCache CacheFrom = GetSimulationSpaceCacheFor(Output, From);
	const FSimulationSpaceCache CacheTo = GetSimulationSpaceCacheFor(Output, To);
	ConvertSimulationSpaceCached(CacheFrom, CacheTo, From, To);
}

FAnimNode_KawaiiPhysics::FSimulationSpaceCache FAnimNode_KawaiiPhysics::BuildSimulationSpaceCache(
	FComponentSpacePoseContext& Output,
	const EKawaiiPhysicsSimulationSpace SimulationSpaceForCache) const
{
	FSimulationSpaceCache Cache;

	switch (SimulationSpaceForCache)
	{
	default:
	case EKawaiiPhysicsSimulationSpace::ComponentSpace:
		Cache.ComponentToTargetSpace = FTransform::Identity;
		Cache.TargetSpaceToComponent = FTransform::Identity;
		break;

	case EKawaiiPhysicsSimulationSpace::WorldSpace:
		{
			const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
			Cache.ComponentToTargetSpace = ComponentToWorld; // Component -> World
			Cache.TargetSpaceToComponent = ComponentToWorld.Inverse(); // World -> Component
		}
		break;

	case EKawaiiPhysicsSimulationSpace::BaseBoneSpace:

		if (SimulationBaseBone.IsValidToEvaluate())
		{
			const FCompactPoseBoneIndex BaseBoneIndex =
				SimulationBaseBone.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			Cache.TargetSpaceToComponent =
				Output.Pose.GetComponentSpaceTransform(BaseBoneIndex); // Base -> Component
			Cache.ComponentToTargetSpace = Cache.TargetSpaceToComponent.Inverse(); // Component -> Base
		}
		break;
	}

	return Cache;
}

int32 FAnimNode_KawaiiPhysics::AutoGenerateGridConstraints(USkeletalMeshComponent* SkelComp,
                                                           FName RootBoneName,
                                                           TArray<FName> ChainPrefixes)
{
	if (!SkelComp || ChainPrefixes.Num() < 2)
	{
		return 0;
	}

	const USkeleton* Skeleton = SkelComp->GetSkeletalMeshAsset() ? SkelComp->GetSkeletalMeshAsset()->GetSkeleton() : nullptr;
	if (!Skeleton)
	{
		return 0;
	}

	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 NumBones = RefSkeleton.GetNum();

	// Find bones for each chain prefix
	TArray<TArray<int32>> ChainBones;
	ChainBones.SetNum(ChainPrefixes.Num());

	for (int32 PrefixIdx = 0; PrefixIdx < ChainPrefixes.Num(); ++PrefixIdx)
	{
		const FName& Prefix = ChainPrefixes[PrefixIdx];
		for (int32 BoneIdx = 0; BoneIdx < NumBones; ++BoneIdx)
		{
			const FName BoneName = RefSkeleton.GetBoneName(BoneIdx);
			if (BoneName.ToString().StartsWith(Prefix.ToString(), ESearchCase::IgnoreCase))
			{
				ChainBones[PrefixIdx].Add(BoneIdx);
			}
		}

		// Sort chains by depth (closer to root first)
		ChainBones[PrefixIdx].Sort([&RefSkeleton](const int32& A, const int32& B)
		{
			// Calculate depth manually (GetDepth may not exist in UE 5.5)
			int32 DepthA = 0, DepthB = 0;
			for (int32 Parent = RefSkeleton.GetParentIndex(A); Parent >= 0; Parent = RefSkeleton.GetParentIndex(Parent)) { ++DepthA; }
			for (int32 Parent = RefSkeleton.GetParentIndex(B); Parent >= 0; Parent = RefSkeleton.GetParentIndex(Parent)) { ++DepthB; }
			return DepthA < DepthB;
		});
	}

	// Generate horizontal constraints between adjacent chains at the same depth level
	int32 ConstraintsGenerated = 0;
	const int32 MaxDepth = FMath::Min(
		ChainBones.Num() > 0 ? ChainBones[0].Num() : 0,
		ChainBones.Num() > 1 ? ChainBones[1].Num() : 0);

	for (int32 Depth = 0; Depth < MaxDepth; ++Depth)
	{
		for (int32 ChainIdx = 0; ChainIdx < ChainBones.Num() - 1; ++ChainIdx)
		{
			if (ChainBones[ChainIdx].IsValidIndex(Depth) && ChainBones[ChainIdx + 1].IsValidIndex(Depth))
			{
				const int32 BoneIdx1 = ChainBones[ChainIdx][Depth];
				const int32 BoneIdx2 = ChainBones[ChainIdx + 1][Depth];

				FModifyBoneConstraint Constraint;
				Constraint.Bone1.BoneName = RefSkeleton.GetBoneName(BoneIdx1);
				Constraint.Bone2.BoneName = RefSkeleton.GetBoneName(BoneIdx2);
				Constraint.Bone1.Initialize(Skeleton);
				Constraint.Bone2.Initialize(Skeleton);

				// Check if constraint already exists
				bool bExists = false;
				for (const FModifyBoneConstraint& Existing : BoneConstraints)
				{
					if (Existing == Constraint)
					{
						bExists = true;
						break;
					}
				}

				if (!bExists)
				{
					BoneConstraints.Add(Constraint);
					ConstraintsGenerated++;
				}
			}
		}
	}

	// Request reinit to apply new constraints
	if (ConstraintsGenerated > 0)
	{
		RequestModifyBonesReinit();
	}

	return ConstraintsGenerated;
}
