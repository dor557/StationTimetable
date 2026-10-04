#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableWidgetSign.h"
#include "StationTimetableBuildable.generated.h"

class AFGBuildableRailroadStation;
class UFGSignPrefabWidget;
class UFGSignTypeDescriptor;
class UStaticMeshComponent;

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
    bool UpdateRenderedTimetable(const FPrefabSignData& SignData);
    void SetRendererDiagnosticState(uint8 NewState, const TCHAR* Description);
#endif

    UFUNCTION()
    void OnRep_LinkedStation();

    UPROPERTY()
    TSoftClassPtr<UFGSignPrefabWidget> TimetableWidgetLayout;

    UPROPERTY()
    TSubclassOf<UFGSignTypeDescriptor> TimetableSignTypeClass;

    UPROPERTY(VisibleDefaultsOnly)
    TObjectPtr<UStaticMeshComponent> TimetableFrameMesh;

    UPROPERTY(SaveGame, ReplicatedUsing = OnRep_LinkedStation)
    TObjectPtr<AFGBuildableRailroadStation> LinkedStation;

    TObjectPtr<AFGBuildableRailroadStation> LinkedStationBeforeBlueprintSerialization;

    TMap<FString, FString> LastPublishedTimetableData;
    bool bHasPublishedTimetableData = false;
    bool bRefreshPausedForInteraction = false;
#if !UE_SERVER
    uint8 RendererDiagnosticState = MAX_uint8;
#endif
    FTimerHandle RefreshTimer;
};
