#include "StationTimetable.h"

#include "FGRecipeManager.h"
#include "FGSchematic.h"
#include "Patching/NativeHookManager.h"
#include "Registry/ModContentRegistry.h"
#include "StationTimetableBuildable.h"
#include "StationTimetableContent.h"
#include "StationTimetablePlacementHologram.h"
#include "Hologram/FGStandaloneSignHologram.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Containers/Ticker.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "HAL/CriticalSection.h"
#include "Misc/ScopeLock.h"
#include "UObject/UObjectArray.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(LogStationTimetable);

namespace
{
    TSubclassOf<UFGRecipe> GetVanillaTrainStationRecipe()
    {
        return LoadClass<UFGRecipe>(
            nullptr,
            TEXT("/Game/FactoryGame/Buildable/Factory/Train/Station/Recipe_TrainStation.Recipe_TrainStation_C"));
    }

    void AddRecipeToVanillaTrainSchematic()
    {
        UClass* TrainSchematicClass = LoadClass<UFGSchematic>(
            nullptr,
            TEXT("/Game/FactoryGame/Schematics/Progression/Schematic_6-3.Schematic_6-3_C"));
        if (!TrainSchematicClass)
        {
            UE_LOG(LogStationTimetable, Error, TEXT("Could not load the Vanilla train schematic Schematic_6-3"));
            return;
        }

        const FArrayProperty* RecipesProperty = FindFProperty<FArrayProperty>(UFGUnlockRecipe::StaticClass(), TEXT("mRecipes"));
        const FObjectPropertyBase* RecipeClassProperty = RecipesProperty
            ? CastField<FObjectPropertyBase>(RecipesProperty->Inner)
            : nullptr;
        if (!RecipesProperty || !RecipeClassProperty)
        {
            UE_LOG(LogStationTimetable, Error, TEXT("Could not access UFGUnlockRecipe.mRecipes"));
            return;
        }

        for (UFGUnlock* Unlock : UFGSchematic::GetUnlocks(TrainSchematicClass))
        {
            UFGUnlockRecipe* RecipeUnlock = Cast<UFGUnlockRecipe>(Unlock);
            if (!RecipeUnlock) continue;

            FScriptArrayHelper Recipes(RecipesProperty, RecipesProperty->ContainerPtrToValuePtr<void>(RecipeUnlock));
            for (int32 Index = 0; Index < Recipes.Num(); ++Index)
            {
                if (RecipeClassProperty->GetObjectPropertyValue(Recipes.GetRawPtr(Index)) == UStationTimetableRecipe::StaticClass())
                {
                    return;
                }
            }

            const int32 NewIndex = Recipes.AddValue();
            RecipeClassProperty->SetObjectPropertyValue(Recipes.GetRawPtr(NewIndex), UStationTimetableRecipe::StaticClass());
            STATION_TIMETABLE_DEV_LOG(Display, TEXT("Attached Station Timetable recipe to Vanilla train schematic Schematic_6-3"));
            return;
        }

        UE_LOG(LogStationTimetable, Error, TEXT("Vanilla train schematic Schematic_6-3 has no recipe unlock"));
    }

    constexpr int32 MaximumUObjectSlots = 2162688;
    class FStationTimetableUObjectCreationListener final : public FUObjectArray::FUObjectCreateListener
    {
    public:
        void Start()
        {
            GUObjectArray.AddUObjectCreateListener(this);
            bRegistered = true;
            TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateRaw(this, &FStationTimetableUObjectCreationListener::Report), 60.0f);
#if !UE_SERVER
            LastLiveObjectCount = GUObjectArray.GetObjectArrayNumMinusAvailable();
            HudTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateRaw(this, &FStationTimetableUObjectCreationListener::UpdateHud), 1.0f);
            DrawDebugDelegateHandle = UDebugDrawService::Register(
                TEXT("Game"),
                FDebugDrawDelegate::CreateRaw(this, &FStationTimetableUObjectCreationListener::DrawHud));
#endif
        }

        void Stop()
        {
            if (TickerHandle.IsValid())
            {
                FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
                TickerHandle.Reset();
            }
#if !UE_SERVER
            if (HudTickerHandle.IsValid())
            {
                FTSTicker::GetCoreTicker().RemoveTicker(HudTickerHandle);
                HudTickerHandle.Reset();
            }
            if (DrawDebugDelegateHandle.IsValid())
            {
                UDebugDrawService::Unregister(DrawDebugDelegateHandle);
                DrawDebugDelegateHandle.Reset();
            }
#endif
            if (bRegistered)
            {
                GUObjectArray.RemoveUObjectCreateListener(this);
                bRegistered = false;
            }
        }

        virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override
        {
            if (!Object || !Object->GetClass()) return;
            const UClass* ObjectClass = Object->GetClass();
            const FName ClassName = ObjectClass->GetFName();
            const FString ClassPackage = ObjectClass->GetOutermost()->GetName();
            FString SourcePackage;
            FString SourceClass;
            for (const UObject* Outer = static_cast<const UObject*>(Object); Outer; Outer = Outer->GetOuter())
            {
                const UClass* OuterClass = Outer->GetClass();
                if (!OuterClass) continue;
                const FString OuterClassPackage = OuterClass->GetOutermost()->GetName();
                if (!OuterClassPackage.StartsWith(TEXT("/Script/")) &&
                    !OuterClassPackage.StartsWith(TEXT("/Engine/Transient")))
                {
                    SourcePackage = OuterClassPackage;
                    SourceClass = OuterClass->GetName();
                    break;
                }
            }
            if (SourcePackage.IsEmpty()) SourcePackage = ClassPackage;
            if (SourceClass.IsEmpty()) SourceClass = ObjectClass->GetName();
            const FString SourceKey = FString::Printf(TEXT("%s<-%s"), *ClassName.ToString(), *SourceClass);
            FScopeLock Lock(&Mutex);
            ++CreatedByClass.FindOrAdd(ClassName);
            ++CreatedBySourcePackage.FindOrAdd(SourcePackage);
            ++CreatedBySourceClass.FindOrAdd(SourceKey);
            ++CreatedTotal;
            ++CreatedSinceLastHudUpdate;
            if (SourcePackage.Contains(TEXT("StationTimetable")) || ClassPackage.Contains(TEXT("StationTimetable"))) ++CreatedStationTimetable;
            else if (SourcePackage.Contains(TEXT("FicsItCam")) || SourcePackage.Contains(TEXT("CineFactory")) ||
                ClassPackage.Contains(TEXT("FicsItCam")) || ClassPackage.Contains(TEXT("CineFactory"))) ++CreatedCineFactory;
            else if (SourcePackage.Contains(TEXT("FactoryGame")) || ClassPackage.Contains(TEXT("FactoryGame"))) ++CreatedFactoryGame;
            else if (ClassPackage.Contains(TEXT("UMG"))) ++CreatedUMG;
            else ++CreatedOther;
        }

        virtual void OnUObjectArrayShutdown() override
        {
            if (bRegistered)
            {
                GUObjectArray.RemoveUObjectCreateListener(this);
                bRegistered = false;
            }
        }

    private:
#if !UE_SERVER
        bool UpdateHud(float DeltaTime)
        {
            const int32 LiveObjectCount = GUObjectArray.GetObjectArrayNumMinusAvailable();
            const int32 ObjectSlots = GUObjectArray.GetObjectArrayNum();
            const int32 LiveDelta = LiveObjectCount - LastLiveObjectCount;
            LastLiveObjectCount = LiveObjectCount;

            int64 CreatedThisSecond = 0;
            {
                FScopeLock Lock(&Mutex);
                CreatedThisSecond = CreatedSinceLastHudUpdate;
                CreatedSinceLastHudUpdate = 0;
            }

            const float UsagePercent = 100.0f * static_cast<float>(LiveObjectCount) /
                static_cast<float>(MaximumUObjectSlots);
            const FColor DisplayColor = FColor::White;
            const FString Message = FString::Printf(
                TEXT("Live UObjects: %d / %d (%.1f%%) | Live %+d/s | Slots %d | Created %lld/s"),
                LiveObjectCount,
                MaximumUObjectSlots,
                UsagePercent,
                LiveDelta,
                ObjectSlots,
                CreatedThisSecond);
            {
                FScopeLock Lock(&Mutex);
                HudMessage = Message;
                HudColor = DisplayColor;
            }
            return true;
        }

        void DrawHud(UCanvas* Canvas, APlayerController* PlayerController)
        {
            if (!Canvas || !GEngine) return;

            FString Message;
            FColor DisplayColor;
            {
                FScopeLock Lock(&Mutex);
                Message = HudMessage;
                DisplayColor = HudColor;
            }
            if (Message.IsEmpty()) return;

            Canvas->SetDrawColor(DisplayColor);
            Canvas->DrawText(GEngine->GetSmallFont(), FStringView(Message), 20.0f, 55.0f);
        }
#endif

        bool Report(float DeltaTime)
        {
            TArray<TPair<FName, int64>> Snapshot;
            TArray<TPair<FString, int64>> SourcePackageSnapshot;
            TArray<TPair<FString, int64>> SourceClassSnapshot;
            int64 Total = 0;
            int64 StationTimetable = 0;
            int64 CineFactory = 0;
            int64 FactoryGame = 0;
            int64 UMG = 0;
            int64 Other = 0;
            {
                FScopeLock Lock(&Mutex);
                Snapshot.Reset(CreatedByClass.Num());
                for (const TPair<FName, int64>& Entry : CreatedByClass) Snapshot.Add(Entry);
                SourcePackageSnapshot.Reserve(CreatedBySourcePackage.Num());
                for (const TPair<FString, int64>& Entry : CreatedBySourcePackage) SourcePackageSnapshot.Add(Entry);
                SourceClassSnapshot.Reserve(CreatedBySourceClass.Num());
                for (const TPair<FString, int64>& Entry : CreatedBySourceClass) SourceClassSnapshot.Add(Entry);
                Total = CreatedTotal;
                StationTimetable = CreatedStationTimetable;
                CineFactory = CreatedCineFactory;
                FactoryGame = CreatedFactoryGame;
                UMG = CreatedUMG;
                Other = CreatedOther;
                CreatedTotal = 0;
                CreatedStationTimetable = 0;
                CreatedCineFactory = 0;
                CreatedFactoryGame = 0;
                CreatedUMG = 0;
                CreatedOther = 0;
                CreatedByClass.Reset();
                CreatedBySourcePackage.Reset();
                CreatedBySourceClass.Reset();
            }
            Snapshot.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });
            SourcePackageSnapshot.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });
            SourceClassSnapshot.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });
            FString TopClasses;
            for (int32 Index = 0; Index < FMath::Min(15, Snapshot.Num()); ++Index)
            {
                if (!TopClasses.IsEmpty()) TopClasses += TEXT(", ");
                TopClasses += FString::Printf(TEXT("%s=%lld"), *Snapshot[Index].Key.ToString(), Snapshot[Index].Value);
            }
            FString TopSourcePackages;
            for (int32 Index = 0; Index < FMath::Min(10, SourcePackageSnapshot.Num()); ++Index)
            {
                if (!TopSourcePackages.IsEmpty()) TopSourcePackages += TEXT(", ");
                TopSourcePackages += FString::Printf(TEXT("%s=%lld"), *SourcePackageSnapshot[Index].Key, SourcePackageSnapshot[Index].Value);
            }
            FString TopSourceClasses;
            for (int32 Index = 0; Index < FMath::Min(15, SourceClassSnapshot.Num()); ++Index)
            {
                if (!TopSourceClasses.IsEmpty()) TopSourceClasses += TEXT(", ");
                TopSourceClasses += FString::Printf(TEXT("%s=%lld"), *SourceClassSnapshot[Index].Key, SourceClassSnapshot[Index].Value);
            }
            UE_LOG(LogStationTimetable, Warning,
                TEXT("UObject creations/60s total=%lld StationTimetable=%lld CineFactory=%lld FactoryGame=%lld UMG=%lld Other=%lld top=[%s] sources=[%s] owners=[%s]"),
                Total, StationTimetable, CineFactory, FactoryGame, UMG, Other, *TopClasses, *TopSourcePackages, *TopSourceClasses);
            return true;
        }

        FCriticalSection Mutex;
        TMap<FName, int64> CreatedByClass;
        TMap<FString, int64> CreatedBySourcePackage;
        TMap<FString, int64> CreatedBySourceClass;
        int64 CreatedTotal = 0;
        int64 CreatedStationTimetable = 0;
        int64 CreatedCineFactory = 0;
        int64 CreatedFactoryGame = 0;
        int64 CreatedUMG = 0;
        int64 CreatedOther = 0;
        int64 CreatedSinceLastHudUpdate = 0;
        bool bRegistered = false;
        FTSTicker::FDelegateHandle TickerHandle;
#if !UE_SERVER
        int32 LastLiveObjectCount = 0;
        FString HudMessage;
        FColor HudColor = FColor::Green;
        FTSTicker::FDelegateHandle HudTickerHandle;
        FDelegateHandle DrawDebugDelegateHandle;
#endif
    };

    TUniquePtr<FStationTimetableUObjectCreationListener> UObjectCreationListener;

#if UE_SERVER
    FTSTicker::FDelegateHandle DedicatedServerUObjectTickerHandle;
    int32 LastDedicatedServerUObjectCount = 0;

    bool ReportDedicatedServerUObjects(float DeltaTime)
    {
        const int32 LiveObjectCount = GUObjectArray.GetObjectArrayNumMinusAvailable();
        const int32 ObjectSlots = GUObjectArray.GetObjectArrayNum();
        const int32 ObjectDelta = LiveObjectCount - LastDedicatedServerUObjectCount;
        LastDedicatedServerUObjectCount = LiveObjectCount;
        UE_LOG(LogStationTimetable, Display,
            TEXT("Dedicated server UObjects/5min live=%d delta=%+d slots=%d available=%d"),
            LiveObjectCount,
            ObjectDelta,
            ObjectSlots,
            GUObjectArray.GetObjectArrayEstimatedAvailable());
        return true;
    }
#endif
}

void FStationTimetableModule::StartupModule()
{
    if (IsRunningCommandlet())
    {
        return;
    }

    STATION_TIMETABLE_DEV_LOG(Display, TEXT("StationTimetable %s (internal build %d) loading"), StationTimetableVersion, StationTimetableBuildNumber);
    if constexpr (StationTimetableDevelopmentDiagnostics)
    {
        UObjectCreationListener = MakeUnique<FStationTimetableUObjectCreationListener>();
        UObjectCreationListener->Start();
    }
#if UE_SERVER
    if constexpr (StationTimetableDevelopmentDiagnostics)
    {
        LastDedicatedServerUObjectCount = GUObjectArray.GetObjectArrayNumMinusAvailable();
        DedicatedServerUObjectTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateStatic(&ReportDedicatedServerUObjects), 300.0f);
    }
#endif

#if !WITH_EDITOR && !UE_SERVER
    UClass* VanillaHologramClass = LoadClass<AFGStandaloneSignHologram>(
        nullptr,
        TEXT("/Game/FactoryGame/Buildable/Factory/-Shared/Holo_StandaloneSign_Large.Holo_StandaloneSign_Large_C")
    );
    if (VanillaHologramClass)
    {
        FStationTimetablePlacementHook::Install(
            VanillaHologramClass->GetDefaultObject<AFGStandaloneSignHologram>()
        );
    }
    else
    {
        UE_LOG(LogStationTimetable, Error, TEXT("Could not load Vanilla large standalone sign hologram for station proximity validation"));
    }
#elif WITH_EDITOR
    STATION_TIMETABLE_DEV_LOG(Display, TEXT("Station proximity placement hook disabled in FactoryEditor because FactoryGame SDK functions are stubs"));
#else
    STATION_TIMETABLE_DEV_LOG(Display, TEXT("Client-only station proximity placement hook disabled on dedicated server"));
#endif

    SUBSCRIBE_UOBJECT_METHOD_AFTER(UModContentRegistry, Initialize, [](UModContentRegistry* Registry, FSubsystemCollectionBase& Collection)
    {
        Registry->RegisterRecipe(TEXT("StationTimetable"), UStationTimetableRecipe::StaticClass());
        AddRecipeToVanillaTrainSchematic();
    });

    SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGRecipeManager, BeginPlay, [](AFGRecipeManager* RecipeManager)
    {
        const TSubclassOf<UFGRecipe> TrainStationRecipe = GetVanillaTrainStationRecipe();
        if (TrainStationRecipe && RecipeManager->IsRecipeAvailable(TrainStationRecipe))
        {
            RecipeManager->AddAvailableRecipe(UStationTimetableRecipe::StaticClass());
        }
        STATION_TIMETABLE_DEV_LOG(Display, TEXT("Registered build recipe: available=%s, building=%s"),
            RecipeManager->IsRecipeAvailable(UStationTimetableRecipe::StaticClass()) ? TEXT("true") : TEXT("false"),
            RecipeManager->IsBuildingAvailable(AStationTimetableBuildable::StaticClass()) ? TEXT("true") : TEXT("false"));
    });
}

void FStationTimetableModule::ShutdownModule()
{
#if UE_SERVER
    if (DedicatedServerUObjectTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(DedicatedServerUObjectTickerHandle);
        DedicatedServerUObjectTickerHandle.Reset();
    }
#endif
    if (UObjectCreationListener)
    {
        UObjectCreationListener->Stop();
        UObjectCreationListener.Reset();
    }
}

IMPLEMENT_MODULE(FStationTimetableModule, StationTimetable)
