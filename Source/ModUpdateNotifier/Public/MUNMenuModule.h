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

USTRUCT(BlueprintType)
struct FAvailableUpdateInfo
{
	GENERATED_BODY()

public:

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
	TArray<FString> InstalledModFriendlyNames; // Human-readable names of installed mods

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> InstalledMods; // Mods that are installed

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> InstalledModIDs; // Satisfactory Mod Repository (https://ficsit.app) Mod IDs used in the REST API

	UPROPERTY(BlueprintReadOnly)
	TArray<FVersion> InstalledModVersions; // Known versions of installed mods

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> ModChangelogs;

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> APIVersions; // Remote versions of installed mods from the Satisfactory Mod Repository (https://ficsit.app)

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> ModAuthors;

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> SupportURLs;

	UPROPERTY(BlueprintReadOnly)
	TArray<bool> HasSupportURLs;

	UPROPERTY(BlueprintReadOnly)
	TArray<FAvailableUpdateInfo> AvailableUpdates; // Known versions of installed mods

	FModUpdateNotifier_ConfigStruct ModNotifierConfig; // Our global mod configuration structure

	bool bDisableNotifications; // Whether the player has chosen to opt out of receiving notifications

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier")
	void Init(); // Initialize the module in subclasses

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier")
	void GetAvailableUpdates(TArray<FAvailableUpdateInfo>& OutAvailableUpdates) const; // Allows the widget to retrieve update information after it has been created. ONLY CALL THIS FROM Widget_MUN_Notification

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier")
	void GetChangelog(FString ModReference); // Allows the widget to retrieve a specific mod's changelog

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ChangelogProcessed(const FString& Changelog); // Allows the widget to retrieve a specific mod's changelog

	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful); // Triggered when we receive a response from the Satisfactory Mod Repository (https://api.ficsit.app/v1/) REST API

	void OnChangelogReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);
};