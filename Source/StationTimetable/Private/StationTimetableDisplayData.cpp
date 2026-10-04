#include "StationTimetableDisplayData.h"

namespace
{
    FString StatusToString(EStationTimetableTrainStatus Status)
    {
        switch (Status)
        {
        case EStationTimetableTrainStatus::Driving: return TEXT("Driving");
        case EStationTimetableTrainStatus::AtSignal: return TEXT("AtSignal");
        case EStationTimetableTrainStatus::AtStation: return TEXT("AtStation");
        default: return TEXT("Unknown");
        }
    }

    EStationTimetableTrainStatus StatusFromString(const FString& Status)
    {
        if (Status == TEXT("Driving")) return EStationTimetableTrainStatus::Driving;
        if (Status == TEXT("AtSignal")) return EStationTimetableTrainStatus::AtSignal;
        if (Status == TEXT("AtStation")) return EStationTimetableTrainStatus::AtStation;
        return EStationTimetableTrainStatus::Unknown;
    }
}

void FStationTimetableDisplayData::WriteToSignData(FPrefabSignData& SignData) const
{
    SignData.TextElementData.Reset();
    SignData.TextElementData.Add(TEXT("StationName"), StationName.ToString());
    SignData.TextElementData.Add(TEXT("StationConnected"), bStationConnected ? TEXT("1") : TEXT("0"));
    SignData.TextElementData.Add(TEXT("ServiceCount"), FString::FromInt(Trains.Num()));

    for (int32 Index = 0; Index < FMath::Min(Trains.Num(), 5); ++Index)
    {
        const FString Prefix = FString::Printf(TEXT("Train%d"), Index);
        const FStationTimetableTrainDisplayData& Train = Trains[Index];
        SignData.TextElementData.Add(Prefix + TEXT("Name"), Train.TrainName.ToString());
        SignData.TextElementData.Add(Prefix + TEXT("Destination"), Train.Destination.ToString());
        SignData.TextElementData.Add(Prefix + TEXT("Eta"), Train.Eta.ToString());
        SignData.TextElementData.Add(Prefix + TEXT("Symbol"), StatusToString(Train.Status));
    }
}

FStationTimetableDisplayData FStationTimetableDisplayData::FromSignData(const FPrefabSignData& SignData)
{
    FStationTimetableDisplayData Result;
    const TMap<FString, FString>& Data = SignData.TextElementData;
    Result.StationName = FText::FromString(Data.FindRef(TEXT("StationName")));
    Result.bStationConnected = Data.FindRef(TEXT("StationConnected")) == TEXT("1");
    const int32 Count = FMath::Clamp(FCString::Atoi(*Data.FindRef(TEXT("ServiceCount"))), 0, 5);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FString Prefix = FString::Printf(TEXT("Train%d"), Index);
        FStationTimetableTrainDisplayData& Train = Result.Trains.AddDefaulted_GetRef();
        Train.TrainName = FText::FromString(Data.FindRef(Prefix + TEXT("Name")));
        Train.Destination = FText::FromString(Data.FindRef(Prefix + TEXT("Destination")));
        Train.Eta = FText::FromString(Data.FindRef(Prefix + TEXT("Eta")));
        Train.Status = StatusFromString(Data.FindRef(Prefix + TEXT("Symbol")));
    }

    return Result;
}
