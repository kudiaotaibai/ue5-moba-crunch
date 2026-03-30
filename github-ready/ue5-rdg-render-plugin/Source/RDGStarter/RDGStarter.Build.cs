using System.IO;
using UnrealBuildTool;

public class RDGStarter : ModuleRules
{
	public RDGStarter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			});

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Projects",
				"RenderCore",
				"Renderer",
				"RHI",
				"Slate",
				"SlateCore"
			});
	}
}
