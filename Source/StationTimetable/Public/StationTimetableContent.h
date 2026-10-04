#pragma once

#include "CoreMinimal.h"
#include "FGConstructDisqualifier.h"
#include "FGRecipe.h"
#include "FGSignTypes.h"
#include "Resources/FGBuildingDescriptor.h"
#include "StationTimetableContent.generated.h"

UCLASS()
class STATIONTIMETABLE_API UStationTimetableDescriptor final : public UFGBuildingDescriptor
{
    GENERATED_BODY()
public:
    UStationTimetableDescriptor();
};

UCLASS()
class STATIONTIMETABLE_API UStationTimetableRecipe final : public UFGRecipe
{
    GENERATED_BODY()
public:
    UStationTimetableRecipe();
};

UCLASS()
class STATIONTIMETABLE_API UStationTimetableNoStationDisqualifier final : public UFGConstructDisqualifier
{
    GENERATED_BODY()
public:
    UStationTimetableNoStationDisqualifier();
};

UCLASS()
class STATIONTIMETABLE_API UStationTimetableSignTypeDescriptor final : public UFGSignTypeDescriptor
{
    GENERATED_BODY()
public:
    UStationTimetableSignTypeDescriptor();
};
