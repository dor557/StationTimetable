#pragma once

#include "Hologram/FGStandaloneSignHologram.h"

class FStationTimetablePlacementHook final : private AFGStandaloneSignHologram
{
public:
    static void Install(AFGStandaloneSignHologram* VanillaHologramDefault);
};
