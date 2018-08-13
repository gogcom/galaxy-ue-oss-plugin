using System.IO;
using UnrealBuildTool;

public class OnlineSubsystemGOG : ModuleRules
{
	public OnlineSubsystemGOG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
#if ! UE_4_20_OR_LATER
		PublicIncludePaths.AddRange(
			new string[] {
				"OnlineSubsystemGOG/Public"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"OnlineSubsystemGOG/Private"
			}
		);
#endif
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"OnlineSubsystemUtils"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Projects",
				"Engine",
				"Sockets",
				"OnlineSubsystem",
				"Json",
				"GalaxySDK",
				"PacketHandler"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
