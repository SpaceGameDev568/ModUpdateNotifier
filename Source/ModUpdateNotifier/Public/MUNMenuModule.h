// Copyright 2024 Jesse Hodgson.

#pragma once

#include "CoreMinimal.h"
#include "Module/MenuWorldModule.h"
#include "Util/SemVersion.h"
#include "HTTP.h"
#include "ModUpdateNotifier_ConfigStruct.h"
#include "Blueprint/UserWidget.h"
#include "MUNMenuModule.generated.h"

UCLASS()
class MODUPDATENOTIFIER_API UMUNMenuModule : public UMenuWorldModule
{
	GENERATED_BODY()

public:

	UMUNMenuModule();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UUserWidget> MenuWidgetClass; // The notification widget class to show on the main menu

	UPROPERTY(BlueprintReadOnly)
	UUserWidget* MenuWidget; // A reference to the widget on the main menu, set after it is created at runtime

	UPROPERTY(EditAnywhere)
	TArray<FString> ModFriendlyNames; // Array of mod human-readable names

	UPROPERTY(EditAnywhere)
	TArray<FString> InstalledModFriendlyNames; // Array of mod human-readable names, only populated at runtime with mods that are installed

	UPROPERTY(EditAnywhere)
	TArray<FString> ModNames; // Array of mod references

	UPROPERTY(EditAnywhere)
	TArray<FString> InstalledMods; // Array of mod references, only populated at runtime with mods that are installed

	UPROPERTY(EditAnywhere)
	TArray<FString> ModIDs; // Array of SMR mod IDs

	UPROPERTY(EditAnywhere)
	TArray<FString> InstalledModIDs; // Array of SMR mod IDs, only populated at runtime with mods that are installed

	UPROPERTY(EditAnywhere)
	TArray<FVersion> ModVersions; // Array of versions for installed mods

	UPROPERTY(EditAnywhere)
	TArray<FString> APIVersionStrings; // Array of mod versions retrieved from the SMR API

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString OutputList; // String list of mods that need to be updated and the version difference information

	FModUpdateNotifier_ConfigStruct MUNConfig; // A pointer to the configuration structure

	bool bDisableNotifications; // Boolean that is true if the user does not want to be notified of updates

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier")
	void Init(); // Function called manually by subclasses to initialize the module

	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful); // Function called when the response is received from the API

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void FinishedProcessing(); // Event triggered when we are finished processing data. Implemented in subclasses to send data about updates to the menu widget via a blueprint interface
};
