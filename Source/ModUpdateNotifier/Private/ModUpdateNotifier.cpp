// Copyright 2024 Jesse Hodgson.

#include "ModUpdateNotifier.h"

#define LOCTEXT_NAMESPACE "FModUpdateNotifierModule"

void FModUpdateNotifierModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FModUpdateNotifierModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

DEFINE_LOG_CATEGORY(LogModUpdateNotifier);

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FModUpdateNotifierModule, ModUpdateNotifier)