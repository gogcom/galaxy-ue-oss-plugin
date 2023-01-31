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
#if UE_4_19_OR_LATER
		RuntimeDependencies.Add(dllPath);
#else
		RuntimeDependencies.Add(new RuntimeDependency(dllPath));
#endif
	}

	private void AddPublicDefinition(string definition)
	{
#if UE_4_19_OR_LATER
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
		if(Target.Platform == UnrealTargetPlatform.Win64)
		{
			galaxyDLLName = "Galaxy64.dll";
			PublicDelayLoadDLLs.Add(galaxyDLLName);
			AddPublicLibrary(LibrariesPath, "Galaxy64.lib");
		}
#if ! UE_5_0_OR_LATER
		else if(Target.Platform == UnrealTargetPlatform.Win32)
		{
			galaxyDLLName = "Galaxy.dll";
			PublicDelayLoadDLLs.Add(galaxyDLLName);
			AddPublicLibrary(LibrariesPath, "Galaxy.lib");
		}
#endif
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			galaxyDLLName = "libGalaxy64.dylib";
			string galaxyDLLPath = Path.Combine(LibrariesPath, galaxyDLLName);
			PublicDelayLoadDLLs.Add(galaxyDLLPath);
#if ! UE_4_22_OR_LATER
			PublicAdditionalShadowFiles.Add(galaxyDLLPath);
			AdditionalBundleResources.Add(new UEBuildBundleResource(galaxyDLLPath, "MacOS"));
#endif
		}
#if ! UE_5_0_OR_LATER
		else if (Target.Platform == UnrealTargetPlatform.PS4)
		{
			galaxyDLLName = "libGalaxy64.prx";
			AddPublicLibrary(LibrariesPath, "Galaxy64_stub_weak");
		}
#endif
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
