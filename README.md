### Supported baseline:
- UnrealEngine 4.16, 4.17, 4.18, 4.19, 4.20, 4.21, 4.22, 4.23, 4.24, 4.25, 4.26, 4.27, 5.0, 5.1
- Galaxy SDK 1.148.3
- Compiler:
	- Windows: Visual Studio 15 Update 3 or later
	- Mac: Xcode 9.4 or later

# Known issues and limitations:
- GalaxySDK may be initialized only once per process, so each player window must be spawned in a separate process
- Dedicated servers are not supported yet

## UE4.20
Multiplayer is broken for UE4.20 due to `FUniqueNetIdRepl` serialization bug in Engine.
You can either cherry-pick the fix done by Michael.Kirzinger@epicgames.com from UnrealEngine repo(commit d2c61c90e1c203bd89d868950b552e5af7e0fe20), or apply diff-patch with these changes(Source/ue4.20-unique-net-id-serialization-fix.patch).

# Installing plugin:

- Copy the plugin folder to your **"GameFolder/Plugins"**
- Unpack content of the [Galaxy SDK](https://devportal.gog.com/galaxy/components/sdk "Galaxy SDK") to **"GameFolder/Plugins/OnlineSubsystemGOG/Source/ThirdParty/GalaxySDK"**
- Enable the plugin:
	* either via UE4 Editor interface:
		* **Settings** -> **Plugins** -> **Online** -> **OnlineSubsystemGOG**
		* check **"Enabled"**
		* restart UnrealEditor

	* or modifying **&#42;.uproject** file manually as follows:
```
"Plugins":
[
	{
		"Name": "OnlineSubsystem",
		"Enabled": true
	},
	{
		"Name": "OnlineSubsystemGOG",
		"Enabled": true
	}
]
```
- Update default engine configuration file (**"GameFolder/Config/DefaultEngine.ini"**) providing values without angle brackets:

```
[OnlineSubsystem]
DefaultPlatformService=GOG

[OnlineSubsystemGOG]
ClientID=<CLIENT_ID>
ClientSecret=<CLIENT_SECRET>
; The local port used to communicate with GOG Galaxy Multiplayer server and other players. Chosen automatically if not specified. Can also be overriden with -port=<port>
; Port=<LOCAL_PORT>
; The local IP address the game would bind to. Chosen automatically if not specified. Can also be overriden with -multihome=<host>
; Host=<LOCAL_HOST>
; When authorizing with Galaxy, this flag controls whether user can play offline (using local profile from the GalaxyClient), or has to be logged on to Galaxy backend services
; When offline, achievements, stats and other data are stored locally until user is online, and some functionalities are unavailable, e.g. friends, multiplayer, rich presence
bRequireBackendAuthorization=<IS_BACKEND_AUTH_REQUIRED>

[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemGOG.NetDriverGOG",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[/Script/Engine.GameEngine]
!NetDriverDefinitions=ClearArray
NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemGOG.NetDriverGOG",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[/Script/OnlineSubsystemGOG.NetDriverGOG]
NetConnectionClassName="/Script/OnlineSubsystemGOG.NetConnectionGOG"
```
 Please contact your GOG.com™ tech representative for more info on how to obtain **ClientID** and **ClientSecret**

## XBOX cross platform support:
Make sure your engine is compiled with the support for the XBOX. Please contact Epic Games™ representatives for more details.

Make sure both Microsoft XDP and GOG.com™ backend services are configured to use XBOX authentication for Galaxy. Please contact your GOG.com™ tech representatives for more information on how to configure this.

To compile this plugin for the XBOX, you will need binaries from both Win64 and XBOX versions of the [Galaxy SDK](https://devportal.gog.com/galaxy/components/sdk "Galaxy SDK") unpacked to "GameFolder/Plugins/OnlineSubsystemGOG/Source/ThirdParty/GalaxySDK"

Update XBOX engine configuration file (**"GameFolder/Config/XboxOne/XboxOneEngine.ini"**), adding XBOX **TitleId** and **PrimaryServiceConfigId**:
```
[OnlineSubsystem]
DefaultPlatformService=GOG

[/Script/XboxOnePlatformEditor.XboxOneTargetSettings]
TitleId=<XBOX TITLE ID>
PrimaryServiceConfigId=<XBOX SERVICE CONFIG ID>
```
Rest of the config file is similar to the default engine configuration file described above.

# Authentication:
In order to use online functionalities user must be signed in GOG Galaxy Client after which game must be authorized using one of the above methods:

- Using C++, `IOnlineIdentity::Login()` method is provided:

```
auto onlineIdentityInterface = Online::GetIdentityInterface(TEXT("GOG"));
auto onLoginCompleteDelegateHandle = onlineIdentityInterface->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateRaw(this, &OnLoginComplete));
FOnlineAccountCredentials accountCredentials;
onlineIdentityInterface->Login(0, accountCredentials);

void OnLoginComplete(int32, bool, const FUniqueNetId&, const FString&)
{
	Online::GetIdentityInterface(TEXT("GOG"))->ClearOnLoginCompleteDelegate_Handle(0, onLoginCompleteDelegateHandle);
	...
	// Proceed with online features initialization
}
```

- Using Blueprints, you can find **Login** method under the **Online** category

## Steam-based authentication
Before authenticating using Steam **EncryptedAppTicket** both **AppID** and **PrivateKey** have to be configured first in [GOG Devportal](https://devportal.gog.com "GOG Devportal"). Please contact your GOG.com™ tech representatives for more information on how to configure them.

Since Steam **EncryptedAppTicket** constains binary data it has to be first converted to a hex string, then passed to the **Login** method along with Steam user name:

```
FOnlineAccountCredentials accountCredentials;
accountCredentials.Type = TEXT("steam");

uint8 rgubTicket[1024];
uint32 cubTicket;
SteamUser()->GetEncryptedAppTicket(rgubTicket, sizeof(rgubTicket), &cubTicket);
accountCredentials.Token = BytesToHex(rgubTicket, cubTicket);

accountCredentials.Id = TEXT(SteamFriends()->GetPersonaName());

Online::GetIdentityInterface(TEXT("GOG"))->Login(0, accountCredentials);
```

## XBOX-based authentication
To authorize in Galaxy using XBOX authentication method, **XboxUserId** has to be passed to the **Login** method:
```
FOnlineAccountCredentials accountCredentials;
accountCredentials.Type = TEXT("xbox");
accountCredentials.Id = <XboxUserId>

Online::GetIdentityInterface(TEXT("GOG"))->Login(0, accountCredentials);
```

# Using the Achievements:
Prior to using the achievements:
* Achievements must be defined in [GOG Devportal](https://devportal.gog.com/panel/games "GOG Devportal")
* Achievements **API Key**s as defined in Devportal should be provided to engine configuration file:

```
[OnlineSubsystemGOG]

+Achievements=ACHIEVEMENT_KEY_1
+Achievements=ACHIEVEMENT_KEY_2
... e.t.c.
```
