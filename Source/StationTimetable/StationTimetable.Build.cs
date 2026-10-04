using UnrealBuildTool;

public class StationTimetable : ModuleRules
{
    public StationTimetable(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UMG", "Slate", "SlateCore",
            "FactoryGame", "SML"
        });

    }
}
