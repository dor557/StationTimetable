#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStationTimetable, Log, All);

enum class EStationTimetableReleaseStage : uint8
{
    Development,
    Alpha,
    Beta,
    Release
};

inline constexpr EStationTimetableReleaseStage StationTimetableReleaseStage = EStationTimetableReleaseStage::Alpha;
inline constexpr bool StationTimetableDevelopmentDiagnostics =
    StationTimetableReleaseStage == EStationTimetableReleaseStage::Development;
inline constexpr int32 StationTimetableBuildNumber = 123;
inline constexpr const TCHAR* StationTimetableVersion = TEXT("0.2.1");

#define STATION_TIMETABLE_DEV_LOG(Verbosity, Format, ...) \
    do { if constexpr (StationTimetableDevelopmentDiagnostics) { \
        UE_LOG(LogStationTimetable, Verbosity, Format, ##__VA_ARGS__); \
    } } while (false)

class FStationTimetableModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
