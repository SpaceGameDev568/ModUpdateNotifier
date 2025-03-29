// Copyright 2024 Jesse Hodgson.

#include "MUNMenuModule.h"

#include "AkAcousticTextureSetComponent.h"
#include "FGBlueprintFunctionLibrary.h"
#include "ModUpdateNotifier.h"
#include "Http.h"
#include "JsonSerializer.h"
#include "Kismet/KismetStringLibrary.h"
#include "ModLoading/ModLoadingLibrary.h"
#include "WorldModuleManager.h"

UMUNMenuModule::UMUNMenuModule()
{
	// Initialize variables from config
	{
	#if WITH_EDITOR
	UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Cannot retrieve config data while in editor."));
	#else
	ModNotifierConfig = FModUpdateNotifier_ConfigStruct::GetActiveConfig(GetWorld());
	#endif
	}

	bDisableNotifications = ModNotifierConfig.bDisableNotifications;

	APIIndex = 0;
	APIIndexRetrieved = 0;
}

void UMUNMenuModule::Init()
{
	// Log metadata from ModUpdateNotifier for debug purposes
	UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Loaded ModUpdateNotifier Menu Module."));

	// Get an instance of the ModLoadingLibrary to use for retrieving mod info
	UModLoadingLibrary *ModLoadingLibrary = GetWorld()->GetGameInstance()->GetSubsystem<UModLoadingLibrary>();

	// Log some build info and debug information about MUN
	FModInfo ModNotifierMetaInfo;
	ModLoadingLibrary->GetLoadedModInfo("ModUpdateNotifier", ModNotifierMetaInfo);
	UE_LOG(LogModUpdateNotifier, Verbose, TEXT("%s"), *ModNotifierMetaInfo.FriendlyName.Append(", " + ModNotifierMetaInfo.Version.ToString()));
	UE_LOG(LogModUpdateNotifier, Display, TEXT("Build Date: %s %s"), ANSI_TO_TCHAR(__DATE__), ANSI_TO_TCHAR(__TIME__));

	// Should we check for updates?
	if (!bDisableNotifications && this->GetWorld()->GetNetMode() != NM_DedicatedServer) // Temporarily disable checking on dedicated servers due to elevated number of SIGSEGV errors
	{
		// Get a reference to the World Module Manager, used later for check for the SMR_ID property on mod World Modules
		UWorldModuleManager *WorldModuleManager = GetWorld()->GetSubsystem<UWorldModuleManager>();
		// Get a list of all loaded mods, we loop through this to check if each mod has the SMR_ID property
		TArray<FModInfo> LoadedMods = ModLoadingLibrary->GetLoadedMods();

		// Get all loaded mod versions and put them into an array
		for(int32 Index = 0; Index != LoadedMods.Num(); ++Index)
		{
			const FString CurrentModName = LoadedMods[Index].Name;

			if(ModLoadingLibrary->IsModLoaded(CurrentModName))
			{
				bool OptedOut = false;
				FModInfo ModInfo;
				ModLoadingLibrary->GetLoadedModInfo(CurrentModName, ModInfo);

				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Detected mod: %s."), *ModInfo.FriendlyName);

				// If we find a module for the currently loaded mod, continue
				if (WorldModuleManager->FindModule(FName(*LoadedMods[Index].Name)))
				{
					// Create a variable to hold the mod module
					UWorldModule* Mod = WorldModuleManager->FindModule(FName(*LoadedMods[Index].Name));

					// Based on this post: https://forums.unrealengine.com/t/how-to-get-a-string-property-by-name/266008

					if (Mod->GetClass()->FindPropertyByName("ModUpdateNotifier_OptOut")->IsValidLowLevel())
					{
						// Get a reference to the OptOut property and assign its value to a variable
						FProperty* Property = Mod->GetClass()->FindPropertyByName("ModUpdateNotifier_OptOut");

						FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property);

						void* PropertyAddress = Property->ContainerPtrToValuePtr<void>(Mod);

						OptedOut = BoolProperty->GetPropertyValue(PropertyAddress);
					}
					if (!OptedOut)
					{
						// Check if the module has a Support_URL property
						if (Mod->GetClass()->FindPropertyByName("ModUpdateNotifier_Support_URL")->IsValidLowLevel())
						{
							// Get a reference to the Support_URL property and assign its value to a variable
							FProperty* Property = Mod->GetClass()->FindPropertyByName("ModUpdateNotifier_Support_URL");

							FStrProperty* StringProperty = CastField<FStrProperty>(Property);

							void* PropertyAddress = Property->ContainerPtrToValuePtr<void>(Mod);

							FString OutValue = StringProperty->GetPropertyValue(PropertyAddress);

							SupportURLs.Add(OutValue);
							HasSupportURLs.Add(true);
						}
						else {
							UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Could not find Support_URL field for mod: %s"), *LoadedMods[Index].FriendlyName);

							SupportURLs.Add("none");
							HasSupportURLs.Add(false);
						}
					}
				}
				else
				{
					if (!OptedOut)
					{
						SupportURLs.Add("none");
						HasSupportURLs.Add(false);
					}
				}

				if (!OptedOut)
				{
					APIVersions.Add({0,0,0});
					APIIndex++;
					ModChangelogs.Add("Unfulfilled");
					InstalledMods.Add(CurrentModName);
					InstalledModVersions.Add(ModInfo.Version);
					InstalledModFriendlyNames.Add(LoadedMods[Index].FriendlyName);
					ModAuthors.Add(LoadedMods[Index].CreatedBy);
				}
				else
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Mod opted-out of update checking: %s"), *LoadedMods[Index].FriendlyName);
				}
			}
		}

		// Create an HTTP request to send to the ficsit.app REST API to get the latest version(s) for each mod
		for(auto& CurrentModReference : InstalledMods)
		{
			// Create an HTTP GET request
			FHttpRequestRef Request =  FHttpModule::Get().CreateRequest();
			TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
			Request->OnProcessRequestComplete().BindUObject(this, &UMUNMenuModule::OnResponseReceived);
			Request->SetURL("https://api.ficsit.app/v1/mod/" + CurrentModReference + "/versions/all");
			Request->SetVerb("GET");

			Request->ProcessRequest();
		}
	}
}

// Parse the HTTP response to extract the data we want: "mod_reference" and "version"
void UMUNMenuModule::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
	if(bWasSuccessful){

		FString LeftStringOut;
		FString RightStringOut;
		FString ModReference;
		Request->GetURL().Split("https://api.ficsit.app/v1/mod/", &LeftStringOut, &RightStringOut, ESearchCase::IgnoreCase, ESearchDir::FromStart);
		RightStringOut.Split("/versions/all", &ModReference, &RightStringOut, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		// Create our JSON object
		TSharedPtr<FJsonObject> ResponseObj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		FJsonSerializer::Deserialize(Reader, ResponseObj);

		// https://forums.unrealengine.com/t/parse-json-file-with-array-of-objects-without-converting-to-ustruct/650839/7
		if(ResponseObj.IsValid() && ResponseObj->HasField("data"))
		{
			// Find the highest version from the API
			const TArray<TSharedPtr<FJsonValue>> DataArrayObj = ResponseObj->GetArrayField(ANSI_TO_TCHAR("data"));

			FVersion HighestVersion = {0,0,0};

			for (auto ArrayItem : DataArrayObj)
			{
				TSharedPtr<FJsonObject> JsonObject = ArrayItem->AsObject();

				FString Version = JsonObject->GetStringField(ANSI_TO_TCHAR("version"));

				if (!Version.Contains("-"))
				{
					FString MajorVersionOut;
					FString MinorVersionOut;
					FString PatchVersionOut;
					Version.Split(".", &MajorVersionOut,&MinorVersionOut, ESearchCase::IgnoreCase, ESearchDir::FromStart);
					MinorVersionOut.Split(".", &MinorVersionOut, &PatchVersionOut, ESearchCase::IgnoreCase, ESearchDir::FromStart);



					FVersion OutVersion = {
						UKismetStringLibrary::Conv_StringToInt64(MajorVersionOut),
						UKismetStringLibrary::Conv_StringToInt64(MinorVersionOut),
						UKismetStringLibrary::Conv_StringToInt64(PatchVersionOut),
					};

					if (OutVersion.Compare(HighestVersion) == 1)
					{
						HighestVersion = OutVersion;
					}
				}
				else
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Version %s contains a `-`, excluding."), *Version);
				}


			}

			APIVersions[InstalledMods.Find(ModReference)] = HighestVersion;

			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("%s: %s"), *ModReference, *HighestVersion.ToString());
		}
		else
		{
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Invalid response: no field \"data\" found."));
		}

		APIIndexRetrieved++;

		// If the API version list is as long as the list of mods we've asked for, compare it to the known version list
		if(APIIndex == APIIndexRetrieved)
		{
			for (int Index = 0; Index < APIVersions.Num(); Index++)
			{
				bool IsModOutOfDate = false;

				// Create an index of the current version in the array that we are processing
				FVersion CurrentAPIVersion = APIVersions[Index];

				if (CurrentAPIVersion.Compare(InstalledModVersions[Index]) == 1)
				{
					IsModOutOfDate = true;
				}
				else
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("The installed mod is up to date or newer than the available versions on SMR. %s"), *InstalledMods[Index]);
				}

				if (IsModOutOfDate)
				{
					FAvailableUpdateInfo ModAvailableUpdate = {
						InstalledModFriendlyNames[Index],
						InstalledMods[Index],
						InstalledModVersions[Index].ToString(),
						APIVersions[Index].ToString(),
						ModChangelogs[Index],
						SupportURLs[Index],
						HasSupportURLs[Index],
						ModAuthors[Index],
					};

					AvailableUpdates.Add(ModAvailableUpdate);
				}
			}

			// If there are out of date mods in the list, create the menu widget. Also check if we are running on a server and not display the menu widget.
			if (!AvailableUpdates.IsEmpty() && this->GetWorld()->GetNetMode() != NM_DedicatedServer)
			{
				// Add the popup in the Main Menu
				const FPopupClosed CloseDelegate;

				UFGBlueprintFunctionLibrary::AddPopupWithCloseDelegate(this->GetWorld()->GetFirstPlayerController(), FText::FromString("Mod Update Notifier"), FText::FromString("Body Text"), CloseDelegate, PID_NONE, MenuWidgetClass, this, false);
			}
			else
			{
				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("All mods are up to date, not displaying a notification."));
			}
		}
	}
	else {
		UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Unable to connect to the API, user may be offline."));
	}

}

void UMUNMenuModule::GetAvailableUpdates(TArray<FAvailableUpdateInfo>& OutAvailableUpdates) const
{
	OutAvailableUpdates = AvailableUpdates;
}

void UMUNMenuModule::GetChangelog(const FString ModReference)
{
	if (ModChangelogs[InstalledMods.Find(ModReference)] != "Unfulfilled")
	{
		ChangelogProcessed(ModChangelogs[InstalledMods.Find(ModReference)]);
	}
	else
	{
		// Create an HTTP GET request
		const FHttpRequestRef Request =  FHttpModule::Get().CreateRequest();
		TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
		Request->OnProcessRequestComplete().BindUObject(this, &UMUNMenuModule::OnChangelogReceived);
		Request->SetURL("https://api.ficsit.app/v2/query");
		Request->SetContentAsString("{\"query\": \"{ getModByReference(modReference:" + ModReference + ") { version(version: \\\"" + APIVersions[InstalledMods.Find(ModReference)].ToString() + "\\\") { changelog mod { mod_reference } } } }\"}");

		Request->SetVerb("POST");
		Request->SetHeader(TEXT("User-Agent"), "X-UE5-ModUpdateNotifier-Agent");
		Request->SetHeader("Content-Type", TEXT("application/json"));

		Request->ProcessRequest();
	}
}

void UMUNMenuModule::OnChangelogReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
	if(bWasSuccessful)
	{
		// Create our JSON object
		TSharedPtr<FJsonObject> ResponseObj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		FJsonSerializer::Deserialize(Reader, ResponseObj);

		if(ResponseObj && ResponseObj->HasField("data"))
		{
			if(const TSharedPtr<FJsonObject> DataObj = ResponseObj->GetObjectField("data"); DataObj && DataObj->HasField("getModByReference"))
			{
				const TSharedPtr<FJsonObject> VersionObj = ResponseObj->GetObjectField("data")->GetObjectField("getModByReference")->GetObjectField("version");

				const FString ModReference = VersionObj->GetObjectField("mod")->GetStringField("mod_reference");

				const FString Changelog = VersionObj->GetStringField("changelog");

				ModChangelogs[InstalledMods.Find(ModReference)] = Changelog;

				ChangelogProcessed(Changelog);
			}
		}
	}
	else {
		UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Unable to connect to the API, user may be offline."));
	}
}
