// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "AnimNotifies/AnimNotify_KawaiiPhysicsImpulse.h"
#include "AnimNode_KawaiiPhysics.h"
#include "KawaiiPhysics.h"
#include "KawaiiPhysicsLibrary.h"
#include "KawaiiPhysicsTypes.h"

UAnimNotify_KawaiiPhysicsImpulse::UAnimNotify_KawaiiPhysicsImpulse(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FString UAnimNotify_KawaiiPhysicsImpulse::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("KawaiiPhysics Impulse: %s (%.0f)"), *BoneName.ToString(), Strength);
}

void UAnimNotify_KawaiiPhysicsImpulse::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	// Find KawaiiPhysics AnimNode through the AnimInstance
	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// Try to find KawaiiPhysics node in linked anim instances
	TArray<UAnimInstance*> LinkedInstances;
	for (UAnimInstance* Instance : const_cast<const USkeletalMeshComponent*>(MeshComp)->GetLinkedAnimInstances())
	{
		LinkedInstances.Add(Instance);
	}

	// Also check the primary AnimInstance itself
	TArray<UAnimInstance*> AllInstances = LinkedInstances;
	AllInstances.AddUnique(AnimInstance);

	for (UAnimInstance* LinkedInstance : AllInstances)
	{
		if (!LinkedInstance)
		{
			continue;
		}

		// Check if this instance has a KawaiiPhysics node
		const UClass* InstanceClass = LinkedInstance->GetClass();
		if (!InstanceClass)
		{
			continue;
		}

		// Look for the AnimNode property
		FProperty* AnimNodeProperty = InstanceClass->FindPropertyByName(FName("KawaiiPhysics"));
		if (!AnimNodeProperty)
		{
			continue;
		}

		// Get the AnimNode struct
		const FStructProperty* StructProperty = CastField<FStructProperty>(AnimNodeProperty);
		if (!StructProperty)
		{
			continue;
		}

		const UScriptStruct* StructType = StructProperty->Struct;
		if (!StructType || !StructType->IsChildOf(FAnimNode_KawaiiPhysics::StaticStruct()))
		{
			continue;
		}

		// Get the AnimNode instance
		void* StructInstance = StructProperty->ContainerPtrToValuePtr<void>(LinkedInstance);
		if (!StructInstance)
		{
			continue;
		}

		FAnimNode_KawaiiPhysics* AnimNode = static_cast<FAnimNode_KawaiiPhysics*>(StructInstance);

		// Apply impulse to the bone
		FVector ImpulseDirection = Direction.GetSafeNormal();
		float CurrentStrength = Strength;

		// Apply to entire chain if requested
		TArray<int32> BonesToImpulse;
		if (bApplyToEntireChain)
		{
			// Find the starting bone index
			for (int32 i = 0; i < AnimNode->ModifyBones.Num(); ++i)
			{
				if (AnimNode->ModifyBones[i].BoneRef.BoneName == BoneName)
				{
					BonesToImpulse.Add(i);

					// Add all child bones in the chain
					for (int32 j = i + 1; j < AnimNode->ModifyBones.Num(); ++j)
					{
						if (AnimNode->ModifyBones[j].ParentIndex == BonesToImpulse.Last())
						{
							BonesToImpulse.Add(j);
						}
					}
					break;
				}
			}
		}
		else
		{
			// Find only the specified bone
			for (int32 i = 0; i < AnimNode->ModifyBones.Num(); ++i)
			{
				if (AnimNode->ModifyBones[i].BoneRef.BoneName == BoneName)
				{
					BonesToImpulse.Add(i);
					break;
				}
			}
		}

		// Apply impulses — queue for thread-safe application during simulation
		for (int32 i = 0; i < BonesToImpulse.Num(); ++i)
		{
			const int32 BoneIndex = BonesToImpulse[i];

			// Calculate strength for this bone (with decay for chain)
			float BoneStrength = CurrentStrength;
			if (bApplyToEntireChain && i > 0)
			{
				BoneStrength = CurrentStrength * FMath::Pow(DecayMultiplier, i);
			}

			// Convert direction to component space if needed
			FVector FinalDirection = ImpulseDirection;
			if (bWorldSpaceDirection)
			{
				const FTransform ComponentTransform = MeshComp->GetComponentTransform();
				FinalDirection = ComponentTransform.InverseTransformVector(ImpulseDirection);
			}

			// Queue impulse for thread-safe application
			FScopeLock Lock(&AnimNode->GetImpulseMutex());
			AnimNode->PendingImpulses.Add({BoneIndex, FinalDirection, BoneStrength});
		}
	}
}

#if WITH_EDITOR
void UAnimNotify_KawaiiPhysicsImpulse::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();

	if (BoneName.IsNone())
	{
		UE_LOG(LogKawaiiPhysicsExtension, Warning, TEXT("KawaiiPhysics Impulse: BoneName is empty"));
	}

	if (FMath::IsNearlyZero(Strength))
	{
		UE_LOG(LogKawaiiPhysicsExtension, Warning, TEXT("KawaiiPhysics Impulse: Strength is zero, notify will have no effect"));
	}
}
#endif
