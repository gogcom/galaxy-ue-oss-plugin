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

	private string BinariesPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", "Binaries", "ThirdParty","GalaxySDK")); }
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
		PublicLibraryPaths.Add(LibrariesPath);

		string galaxyDLLName;
		if(Target.Platform == UnrealTargetPlatform.Win32)
		{
			galaxyDLLName = "Galaxy.dll";
			PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, "Galaxy.lib"));
		}
		else if(Target.Platform == UnrealTargetPlatform.Win64)
		{
			galaxyDLLName = "Galaxy64.dll";
			PublicAdditionalLibraries.Add("Galaxy64.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			galaxyDLLName = "libGalaxy.dylib";
			PublicAdditionalShadowFiles.Add(Path.Combine(BinariesPath, galaxyDLLName));
			PublicLibraryPaths.Add(BinariesPath);
		}
		else
		{
			string Err = string.Format("Unsupported platform: {0}", Target.Platform);
			System.Console.WriteLine(Err);
			throw new BuildException(Err);
		}

		PublicDefinitions.Add("GALAXY_DLL_NAME=" + galaxyDLLName);
		PublicDelayLoadDLLs.Add(Path.Combine(BinariesPath, galaxyDLLName));
		RuntimeDependencies.Add(Path.Combine(BinariesPath, galaxyDLLName));
	}
}
