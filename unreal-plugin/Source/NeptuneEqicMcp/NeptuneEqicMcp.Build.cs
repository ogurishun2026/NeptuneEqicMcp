using UnrealBuildTool;

public class NeptuneEqicMcp : ModuleRules
{
    public NeptuneEqicMcp(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore",
            "Json", "JsonUtilities", "WebSockets", "Networking"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
    }
}
