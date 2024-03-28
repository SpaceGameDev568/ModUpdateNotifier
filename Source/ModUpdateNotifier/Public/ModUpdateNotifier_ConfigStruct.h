#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "ModUpdateNotifier_ConfigStruct.generated.h"

/* Struct generated from Mod Configuration Asset '/ModUpdateNotifier/ModUpdateNotifier_Config' */
USTRUCT(BlueprintType)
struct FModUpdateNotifier_ConfigStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    bool bDisableNotifications{};

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FModUpdateNotifier_ConfigStruct GetActiveConfig(UObject* WorldContext) {
        FModUpdateNotifier_ConfigStruct ConfigStruct{};
        FConfigId ConfigId{"ModUpdateNotifier", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FModUpdateNotifier_ConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

