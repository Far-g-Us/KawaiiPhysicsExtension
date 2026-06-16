// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#include "KawaiiPhysicsEd.h"
#include "Modules/ModuleManager.h"
#include "Textures/SlateIcon.h"
#include "KawaiiPhysicsEditMode.h"

#define LOCTEXT_NAMESPACE "FKawaiiPhysicsExtensionModuleEd"


void FKawaiiPhysicsExtensionEdModule::StartupModule()
{
	FEditorModeRegistry::Get().RegisterMode<FKawaiiPhysicsEditMode>("AnimGraph.SkeletalControl.KawaiiPhysicsExtension",
	                                                                LOCTEXT("FKawaiiPhysicsExtensionEditMode", "Kawaii Physics"),
	                                                                FSlateIcon(), false);
}


void FKawaiiPhysicsExtensionEdModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode("AnimGraph.SkeletalControl.KawaiiPhysicsExtension");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FKawaiiPhysicsExtensionEdModule, KawaiiPhysicsExtensionEd)
//IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, KawaiiPhysicsExtensionEd, "KawaiiPhysicsExtensionEd" );
