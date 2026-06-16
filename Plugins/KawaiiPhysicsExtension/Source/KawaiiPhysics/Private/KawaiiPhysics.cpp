// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "KawaiiPhysics.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FKawaiiPhysicsExtensionModule"

void FKawaiiPhysicsExtensionModule::StartupModule()
{
	UE_LOG(LogKawaiiPhysicsExtension, Log, TEXT("KawaiiPhysicsExtension module started (v1.31.1)"));
}

void FKawaiiPhysicsExtensionModule::ShutdownModule()
{
	UE_LOG(LogKawaiiPhysicsExtension, Log, TEXT("KawaiiPhysicsExtension module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKawaiiPhysicsExtensionModule, KawaiiPhysicsExtension)
