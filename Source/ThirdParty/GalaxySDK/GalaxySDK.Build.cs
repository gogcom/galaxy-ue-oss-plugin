using System;
using System.IO;
using UnrealBuildTool;

public class GalaxySDK : ModuleRules
{

	private string IncludePath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "Include")); }
	}

	private string LibrariesPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "Libraries")); }
	}

	private void AddRuntimeDependency(string dllPath)
	{
#if UE_4_18_OR_LATER
		RuntimeDependencies.Add(dllPath);
#else
		RuntimeDependencies.Add(new RuntimeDependency(dllPath));
#endif
	}

	private void AddPublicDefinition(string definition)
	{
#if UE_4_18_OR_LATER
		PublicDefinitions.Add(definition);
#else
		Definitions.Add(definition);
#endif
	}

	private void AddPublicLibrary(string libraryPath, string libraryName)
	{
		PublicAdditionalLibraries.Add(
#if UE_4_24_OR_LATER
		Path.Combine(libraryPath, libraryName)
#else
		libraryName
#endif
		);
	}

	public GalaxySDK(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (!Directory.Exists(ModuleDirectory))
		{
			string Err = string.Format("GalaxySDK not found in {0}", ModuleDirectory);
			System.Console.WriteLine(Err);
			throw new BuildException(Err);
		}

		if (!Directory.Exists(IncludePath))
		{
			string Err = string.Format("Galaxy 'Include' folder not found: {0}", IncludePath);
			System.Console.WriteLine(Err);
			throw new BuildException(Err);
		}
		PublicIncludePaths.Add(IncludePath);

		if (!Directory.Exists(LibrariesPath))
		{
			string Err = string.Format(" Galaxy'Libraries' folder not found: {0}", LibrariesPath);
			System.Console.WriteLine(Err);
			throw new BuildException(Err);
		}
#if ! UE_4_24_OR_LATER
		PublicLibraryPaths.Add(LibrariesPath);
#endif
		string galaxyDLLName;
		if(Target.Platform == UnrealTargetPlatform.Win32)
		{
			galaxyDLLName = "Galaxy.dll";
			PublicDelayLoadDLLs.Add(galaxyDLLName);
			AddPublicLibrary(LibrariesPath, "Galaxy.lib");
		}
		else if(Target.Platform == UnrealTargetPlatform.Win64)
		{
			galaxyDLLName = "Galaxy64.dll";
			PublicDelayLoadDLLs.Add(galaxyDLLName);
			AddPublicLibrary(LibrariesPath, "Galaxy64.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			galaxyDLLName = "libGalaxy.dylib";
			string galaxyDLLPath = Path.Combine(LibrariesPath, galaxyDLLName);
			PublicDelayLoadDLLs.Add(galaxyDLLPath);
#if ! UE_4_22_OR_LATER
			PublicAdditionalShadowFiles.Add(galaxyDLLPath);
			AdditionalBundleResources.Add(new UEBuildBundleResource(galaxyDLLPath, "MacOS"));
#endif
		}
		else if (Target.Platform == UnrealTargetPlatform.XboxOne)
		{
			galaxyDLLName = "Galaxy64_Durango.dll";
			AddPublicLibrary(LibrariesPath, "Galaxy64_Durango.lib");
			// In order to compile this for XBOX, please copy 'delayimp.lib' binary from:
			// from: c:/Program Files (x86)/Microsoft Visual Studio/<version>/<Professional | Community>/VC/Tools/MSVC/<MS Tools version>/lib/x64/delayimp.lib
			// to: <game repository>/Plugins/OnlineSubsystemGOG/Source/ThirdParty/GalaxySDK/Libraries/
			AddPublicLibrary(LibrariesPath, "delayimp.lib");
			PublicDelayLoadDLLs.Add(galaxyDLLName);
		}
		else if (Target.Platform == UnrealTargetPlatform.PS4)
		{
			galaxyDLLName = "libGalaxy64.prx";
			AddPublicLibrary(LibrariesPath, "Galaxy64_stub_weak");
		}
		else
		{
			string Err = string.Format("Unsupported platform: {0}", Target.Platform);
			System.Console.WriteLine(Err);
			throw new BuildException(Err);
		}

		AddPublicDefinition("GALAXY_DLL_NAME=" + galaxyDLLName);
		AddRuntimeDependency(Path.Combine(LibrariesPath, galaxyDLLName));
	}
}
