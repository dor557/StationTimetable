#include "StationTimetablePlacementHologram.h"

#if !UE_SERVER
#include "StationTimetableBuildable.h"
#include "StationTimetableContent.h"
#include "Patching/NativeHookManager.h"

void FStationTimetablePlacementHook::Install(
    AFGStandaloneSignHologram* VanillaHologramDefault)
{
    if (!VanillaHologramDefault) return;

    SUBSCRIBE_METHOD_VIRTUAL_AFTER(
        AFGHologram::CheckValidPlacement,
        VanillaHologramDefault,
        [](AFGHologram* Hologram)
        {
            const TSubclassOf<AActor> BuildClass = Hologram->GetBuildClass();
            if (!BuildClass || !BuildClass->IsChildOf(AStationTimetableBuildable::StaticClass())) return;
            if (Hologram->GetBlueprintDesigner()) return;

            if (!AStationTimetableBuildable::FindNearestStation(Hologram, Hologram->GetActorLocation()))
            {
                Hologram->AddConstructDisqualifier(UStationTimetableNoStationDisqualifier::StaticClass());
            }
        }
    );
}
#else
void FStationTimetablePlacementHook::Install(AFGStandaloneSignHologram* VanillaHologramDefault)
{
}
#endif
