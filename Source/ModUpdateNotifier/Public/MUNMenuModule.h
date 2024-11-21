// Copyright 2024 Jesse Hodgson.

#pragma once

#include "CoreMinimal.h"
#include "Module/MenuWorldModule.h"
#include "Util/SemVersion.h"
#include "Http.h"
#include "ModUpdateNotifier_ConfigStruct.h"
#include "Blueprint/UserWidget.h"
#include "MUNMenuModule.generated.h"

USTRUCT(BlueprintType)
struct FModUpdateNotifierInfo
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModFriendlyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModID;
};

UCLASS()
class MODUPDATENOTIFIER_API UMUNMenuModule : public UMenuWorldModule
{
	GENERATED_BODY()

public:

	UMUNMenuModule();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UUserWidget> MenuWidgetClass; // The notification widget class to show on the main menu

	UPROPERTY(BlueprintReadOnly)
	UUserWidget* MenuWidget; // The main menu widget

	UPROPERTY(EditAnywhere)
	TArray<FString> InstalledModFriendlyNames; // Human-readable names of installed mods

	UPROPERTY(EditAnywhere)
	TArray<FString> InstalledMods; // Mods that are installed

	UPROPERTY(EditAnywhere)
	TArray<FString> InstalledModIDs; // Satisfactory Mod Repository (https://ficsit.app) Mod IDs used in the REST API

	UPROPERTY(EditAnywhere)
	TArray<FVersion> InstalledModVersions; // Known versions of installed mods

	UPROPERTY(EditAnywhere)
	TArray<FString> APIVersions; // Remote versions of installed mods from the Satisfactory Mod Repository (https://ficsit.app)

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString ModUpdates; // List of available mod updates to be shown to the player in the notification

	FModUpdateNotifier_ConfigStruct ModNotifierConfig; // Our global mod configuration structure

	bool bDisableNotifications; // Whether the player has chosen to opt out of receiving notifications

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier")
	void Init(TArray<FModUpdateNotifierInfo> ModInfoList); // Initialize the module in subclasses

	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful); // Begin processing the response from the Satisfactory Mod Repository (https://ficsit.app) REST API

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void FinishedProcessingUpdates(); // Triggered when we are finished processing updates. Implemented in subclasses to send data about updates to the menu widget via a blueprint interface
};
