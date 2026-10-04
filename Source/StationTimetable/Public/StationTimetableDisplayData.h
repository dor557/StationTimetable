#pragma once

#include "CoreMinimal.h"
#include "FGSignTypes.h"
#include "StationTimetableDisplayData.generated.h"

UENUM(BlueprintType)
enum class EStationTimetableTrainStatus : uint8
{
    Unknown,
    Driving,
    AtSignal,
    AtStation
};

USTRUCT(BlueprintType)
struct STATIONTIMETABLE_API FStationTimetableTrainDisplayData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Station Timetable")
    FText TrainName;

    UPROPERTY(BlueprintReadOnly, Category = "Station Timetable")
    FText Destination;

    UPROPERTY(BlueprintReadOnly, Category = "Station Timetable")
    FText Eta;

    UPROPERTY(BlueprintReadOnly, Category = "Station Timetable")
    EStationTimetableTrainStatus Status = EStationTimetableTrainStatus::Unknown;
};

USTRUCT(BlueprintType)
struct STATIONTIMETABLE_API FStationTimetableDisplayData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Station Timetable")
    FText StationName;

    UPROPERTY(BlueprintReadOnly, Category = "Station Timetable")
    bool bStationConnected = false;

    UPROPERTY(BlueprintReadOnly, Category = "Station Timetable")
    TArray<FStationTimetableTrainDisplayData> Trains;

    void WriteToSignData(FPrefabSignData& SignData) const;
    static FStationTimetableDisplayData FromSignData(const FPrefabSignData& SignData);
};
