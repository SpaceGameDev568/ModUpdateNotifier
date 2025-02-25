// Copyright 2024 Jesse Hodgson.

#include "MUNMenuModule.h"

#include "AkAcousticTextureSetComponent.h"
#include "FGBlueprintFunctionLibrary.h"
#include "ModUpdateNotifier.h"
#include "Http.h"
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

		// Create the array of mod info for checking later
		TArray<FModUpdateNotifierInfo> ModList;

		// Check all loaded mods
		for (auto& CurrentLoadedMod : LoadedMods)
		{
			// If we find a module for the currently loaded mod, continue
			if (WorldModuleManager->FindModule(FName(*CurrentLoadedMod.Name)))
			{
				// Create a variable to hold the mod module
				UWorldModule* Mod = WorldModuleManager->FindModule(FName(*CurrentLoadedMod.Name));

				// Based on this post: https://forums.unrealengine.com/t/how-to-get-a-string-property-by-name/266008
				//
				// Check if the module has an SMR_ID property
				if (Mod->GetClass()->FindPropertyByName("ModUpdateNotifier_SMR_ID")->IsValidLowLevel())
				{
					// Get a reference to the SMR_ID property and assign its value to a variable
					FProperty* Property = Mod->GetClass()->FindPropertyByName("ModUpdateNotifier_SMR_ID");

					FStrProperty* StringProperty = CastField<FStrProperty>(Property);

					void* PropertyAddress = Property->ContainerPtrToValuePtr<void>(Mod);

					FString OutValue = StringProperty->GetPropertyValue(PropertyAddress);

					// Assign the info from the mod along with its SMR ID to a struct, then add it to the list of mods to check for updates
					FModUpdateNotifierInfo MUNInfo = {
						CurrentLoadedMod.FriendlyName,
						CurrentLoadedMod.Name,
						OutValue
					};

					ModList.Add(MUNInfo);
				}
				// DEBUG: else {
				// 	UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Could not find SMR_ID field for mod: %s"), *CurrentLoadedMod.FriendlyName);
				// }
			}
		}

		// List out the info for each mod manually (NOT USED CURRENTLY)

		// const TArray<FModNotifierInfo> ModList = {
		// 	{ "Better Grass", "BetterGrass", "4S2xwMEFdMKymS" },// Better Grass
		// 	{ "SatisWHACKtory", "ObstacleMod", "8XYLMRNbnfzc2G" }, // SatisWHACKtory
		// 	{ "Remove All Annoyances", "RemoveAllAnnoyances", "FGnDVTV2ygmANY" }, // Remove All Annoyances
		// 	{ "Factory Props", "Factory_Prop_Mod", "8ivr6Mvuv4sCkX" }, // Factory Props
		// 	{ "Discord Rich Presence", "FG_DiscordRP", "2t2nCEBqMdUt1n" }, // Discord Rich Presence
		// 	{ "2m Walls", "TwoMeterWalls", "7NEYeWC3Mf5Rqa" }, // 2m Walls
		// 	{ "More Players", "MorePlayers", "CMA7t3H6L1dkWT" }, // More Players
		// 	{ "Mod Update Notifier", "ModUpdateNotifier", "8KzYMxowiUmKLn" } // Mod Update Notifier
		// };

		// Get all loaded mod versions and put them into an array
		for(int32 Index = 0; Index != ModList.Num(); ++Index)
		{
			const FString CurrentModName = ModList[Index].ModName;

			if(ModLoadingLibrary->IsModLoaded(CurrentModName))
			{
				FModInfo ModInfo;
				ModLoadingLibrary->GetLoadedModInfo(CurrentModName, ModInfo);

				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Added %s to mod version list."), *ModInfo.FriendlyName);

				APIVersions.Add("Unfulfilled");
				InstalledMods.Add(CurrentModName);
				InstalledModVersions.Add(ModInfo.Version);
				InstalledModIDs.Add(ModList[Index].ModID);
				InstalledModFriendlyNames.Add(ModList[Index].ModFriendlyName);
			}
		}

		// Create an HTTP request to send to the ficsit.app REST API to get the latest version(s) for each mod
		for(auto& CurrentModID : InstalledModIDs)
		{
			// Create an HTTP GET request
			FHttpRequestRef Request =  FHttpModule::Get().CreateRequest();
			TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
			Request->OnProcessRequestComplete().BindUObject(this, &UMUNMenuModule::OnResponseReceived);
			Request->SetURL("https://api.ficsit.app/v1/mod/" + CurrentModID + "/latest-versions");
			Request->SetVerb("GET");

			Request->ProcessRequest();
		}
	}
}

// Parse the HTTP response to extract the data we want: "mod_id" and "version"
void UMUNMenuModule::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
	if(bWasSuccessful){

		// Create our JSON object
		TSharedPtr<FJsonObject> ResponseObj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		FJsonSerializer::Deserialize(Reader, ResponseObj);

		if(ResponseObj && ResponseObj->HasField("data"))
		{
			// Create more objects to narrow down the field to a valid release, beta, or alpha version.
			const TSharedPtr<FJsonObject> DataObj = ResponseObj->GetObjectField("data");
			TSharedPtr<FJsonObject> ReleaseObj;

			// Check if there is a release of the mod available. If there is not, fall back to a beta or alpha release.
			if (DataObj->HasField("release"))
			{
				ReleaseObj = DataObj->GetObjectField("release");
			}
			else if (DataObj->HasField("beta"))
			{
				ReleaseObj = DataObj->GetObjectField("beta");
			}
			else if (DataObj->HasField("alpha"))
			{
				ReleaseObj = DataObj->GetObjectField("alpha");
			}
			else
			{
				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Response does not contain a release of any kind."));
			}

			// Put the response info into variables
			const FString ResponseID = ReleaseObj->GetStringField("mod_id");
			const FString ResponseVersion = ReleaseObj->GetStringField("version");

			// Add the version response to the API version array at the corresponding index of the mod ID for future retrieval
			APIVersions[InstalledModIDs.Find(ResponseID)] = ResponseVersion;
		}
		else
		{
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Invalid response: no field \"data\" found."));
		}

		// If the API version list is complete, compare it to the known version list
		if(!APIVersions.Contains("Unfulfilled"))
		{
			for (auto& CurrentAPIVersionString : APIVersions)
			{
				bool IsModOutOfDate = false;

				// Create an index of the current version in the array that we are processing
				const int Index = APIVersions.Find(CurrentAPIVersionString);

				// Create variables for the major, minor, and patch version numbers from the API response to be compared against the known version
				FString MajorVersionOut;
				FString MinorVersionOut;
				FString PatchVersionOut;
				CurrentAPIVersionString.Split(".", &MajorVersionOut,&MinorVersionOut, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				MinorVersionOut.Split(".", &MinorVersionOut, &PatchVersionOut, ESearchCase::IgnoreCase, ESearchDir::FromStart);

				// Check though the major, minor, and patch numbers from the API response sequentially to see if it is greater than the known version.
				if(UKismetStringLibrary::Conv_StringToInt(MajorVersionOut) > InstalledModVersions[Index].Major)
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("There is a newer version of this mod available since the major version is lower than the retrieved one. %s"), *InstalledMods[Index]);

					IsModOutOfDate = true;
				} // If the remote Minor version is greater AND the remote Major version is greater than or equal to the local version
				else if (UKismetStringLibrary::Conv_StringToInt(MinorVersionOut) > InstalledModVersions[Index].Minor && UKismetStringLibrary::Conv_StringToInt(MajorVersionOut) >= InstalledModVersions[Index].Major)
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("There is a newer version of this mod available since the minor version is lower than the retrieved one. %s"), *InstalledMods[Index]);

					IsModOutOfDate = true;
				} // If the remote Patch version is greater AND the remote Minor version is greater than or equal to the local version
				else if (UKismetStringLibrary::Conv_StringToInt(PatchVersionOut) > InstalledModVersions[Index].Patch && UKismetStringLibrary::Conv_StringToInt(MinorVersionOut) >= InstalledModVersions[Index].Minor)
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("There is a newer version of this mod available since the patch version is lower than the retrieved one. %s"), *InstalledMods[Index]);

					IsModOutOfDate = true;
				}
				else
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("The installed mod is up to date or newer than the available versions on SMR. %s"), *InstalledMods[Index]);
				}

				// If a mod has updates available and the list is not empty, add it to the list on a new line
				if(ModUpdates.IsEmpty() && IsModOutOfDate == true)
				{
					ModUpdates = InstalledModFriendlyNames[Index] + " " + InstalledModVersions[Index].ToString() + " -> " + APIVersions[Index];
				}
				else if (IsModOutOfDate == true)
				{
					ModUpdates = ModUpdates + ",\n" + InstalledModFriendlyNames[Index] + " " + InstalledModVersions[Index].ToString() + " -> " + APIVersions[Index];
				}
			}

			// If there are out of date mods in the list, create the menu widget. Also check if we are running on a server and not display the menu widget.
			if (!ModUpdates.IsEmpty() && this->GetWorld()->GetNetMode() != NM_DedicatedServer)
			{
				// Add the popup in the Main Menu
				const FPopupClosed CloseDelegate;

				UFGBlueprintFunctionLibrary::AddPopupWithCloseDelegate(this->GetWorld()->GetGameInstance()->GetFirstLocalPlayerController(), FText::FromString("Mod Update Notifier"), FText::FromString("Body Text"), CloseDelegate, PID_NONE, MenuWidgetClass, this, false);
			}
			else
			{
				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("All mods are up to date, not displaying a notification."));
			}
		}
	}
	else {
		UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Unable to connect to API, user may be offline."));
	}

}

void UMUNMenuModule::GetAvailableUpdates(FString& AvailableUpdates)
{
	AvailableUpdates = ModUpdates;
}