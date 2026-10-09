#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableWidgetSign.h"
#include "StationTimetableDisplayData.h"
#include "StationTimetableBuildable.generated.h"

class AFGBuildableRailroadStation;
class UFGSignPrefabWidget;
class UFGSignTypeDescriptor;
class UStaticMeshComponent;
class UWidgetComponent;

UCLASS()
class STATIONTIMETABLE_API AStationTimetableBuildable final : public AFGBuildableWidgetSign
{
    GENERATED_BODY()
public:
    AStationTimetableBuildable();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostLoadGame_Implementation(int32 SaveVersion, int32 GameVersion) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsUseable_Implementation() const override;
    virtual void OnUse_Implementation(AFGCharacterPlayer* ByCharacter, const FUseState& State) override;
    virtual UFGFactoryClipboardSettings* CopySettings_Implementation() override;
    virtual bool PasteSettings_Implementation(UFGFactoryClipboardSettings* Settings, AFGPlayerController* Player) override;
    virtual void PreSerializedToBlueprint() override;
    virtual void PostSerializedToBlueprint() override;
    virtual void PostSerializedFromBlueprint(bool IsBlueprintWorld = false) override;

    AFGBuildableRailroadStation* GetLinkedStation() const { return LinkedStation; }
    static AFGBuildableRailroadStation* FindNearestStation(const UObject* WorldContext, const FVector& Location, float MaxDistanceFromStationEdge = 500.0f);

private:
    void RefreshTimetableData();
    bool IsSignEditorOpen() const;
#if !UE_SERVER
    bool UpdateRuntimeWidget();
#endif

    UFUNCTION()
    void OnRep_LinkedStation();

    UFUNCTION()
    void OnRep_TimetableRevision();

    UPROPERTY()
    TSoftClassPtr<UFGSignPrefabWidget> TimetableWidgetLayout;

    UPROPERTY()
    TSubclassOf<UFGSignTypeDescriptor> TimetableSignTypeClass;

    UPROPERTY(VisibleDefaultsOnly)
    TObjectPtr<UStaticMeshComponent> TimetableFrameMesh;

    UPROPERTY(SaveGame, ReplicatedUsing = OnRep_LinkedStation)
    TObjectPtr<AFGBuildableRailroadStation> LinkedStation;

    UPROPERTY(Replicated)
    FStationTimetableDisplayData ReplicatedTimetableData;

    UPROPERTY(ReplicatedUsing = OnRep_TimetableRevision)
    uint32 TimetableRevision = 0;

    TObjectPtr<AFGBuildableRailroadStation> LinkedStationBeforeBlueprintSerialization;

    bool bHasPublishedTimetableData = false;
    bool bRefreshPausedForInteraction = false;
#if !UE_SERVER
    uint32 LastObservedClientRevision = MAX_uint32;
    uint32 LastAppliedRuntimeRevision = MAX_uint32;
    uint32 LastRuntimeDiagnosticRevision = MAX_uint32;
    TWeakObjectPtr<UWidgetComponent> LastRuntimeWidgetComponent;
#endif
    FTimerHandle RefreshTimer;
};
