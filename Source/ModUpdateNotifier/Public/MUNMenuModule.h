// Copyright 2024 - 2026 Jesse Hodgson.

#pragma once

#include "CoreMinimal.h"
#include "Module/MenuWorldModule.h"
#include "Util/SemVersion.h"
#include "Http.h"
#include "ModUpdateNotifier_ConfigStruct.h"
#include "Blueprint/UserWidget.h"
#include "MUNMenuModule.generated.h"

USTRUCT(BlueprintType)
struct FAvailableUpdateInfo // Struct containing all info from the arrays for each mod
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModFriendlyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModExistingVersion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModAvailableVersion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModChangelog;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModDonationURL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bModHasDonationURL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ModAuthor;
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
	int APIIndex; // Index of API Versions we've asked for

	UPROPERTY(BlueprintReadOnly)
	int APIIndexRetrieved; // Index of API Versions we've received

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> InstalledModFriendlyNames; // Human-readable names of installed mods

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> InstalledMods; // Mods that are installed

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> InstalledModIDs; // Satisfactory Mod Repository (https://ficsit.app) Mod IDs used in the REST API

	UPROPERTY(BlueprintReadOnly)
	TArray<FVersion> InstalledModVersions; // Known versions of installed mods

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> ModChangelogs; // Previously fetched mod changelogs

	UPROPERTY(BlueprintReadOnly)
	TArray<FVersion> APIVersions; // Remote versions of installed mods from the Satisfactory Mod Repository (https://ficsit.app)

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> ModAuthors; // Mod Authors

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> SupportURLs; // Supplied URLs for mod donation platforms. Filled with "none" if no URL is supplied.

	UPROPERTY(BlueprintReadOnly)
	TArray<bool> HasSupportURLs; // Whether a mod has supplied a support URL

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> LockedDependencies;

	UPROPERTY(BlueprintReadOnly)
	TArray<FAvailableUpdateInfo> AvailableUpdates; // Known versions of installed mods

	FModUpdateNotifier_ConfigStruct ModNotifierConfig; // Our global mod configuration structure

	bool bShowNotifications; // Whether the player has chosen to opt out of receiving notifications

	bool bDebugLogging; // Opt-in for debug logging to not spam the log during normal operation

	bool bDisableNotifications; // Legacy thingy dont touch

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier", Exec)
	void CheckForModUpdates(); // Initialize the module in subclasses

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier")
	void GetAvailableUpdates(TArray<FAvailableUpdateInfo>& OutAvailableUpdates) const; // Allows the widget to retrieve update information after it has been created. ONLY CALL THIS FROM Widget_MUN_Notification

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier")
	void GetChangelog(FString ModReference); // Allows the widget to retrieve a specific mod's changelog

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ChangelogProcessed(const FString& Changelog); // Allows the widget to retrieve a specific mod's changelog

	// Triggered when we receive a response from the Satisfactory Mod Repository (https://api.ficsit.app/v1/) REST API for mod updates
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);

	// Triggered when we receive a response containing a mod changelog
	void OnChangelogReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);

protected:
	virtual void BeginDestroy() override;

private:
	void CancelPendingRequests();
	TArray<FHttpRequestPtr> PendingRequests;
};
