#include "StationTimetableBuildable.h"

#include "Buildables/FGBuildableRailroadStation.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "FGRailroadSubsystem.h"
#include "FGRailroadTimeTable.h"
#include "FGTrain.h"
#include "FGTrainStationIdentifier.h"
#include "FGColoredInstanceMeshProxy.h"
#include "FGHUD.h"
#include "FGSignSubsystem.h"
#include "UI/FGGameUI.h"
#include "UI/FGInteractWidget.h"
#include "Hologram/FGStandaloneSignHologram.h"
#include "Net/UnrealNetwork.h"
#include "Materials/Material.h"
#include "StationTimetable.h"
#include "StationTimetableContent.h"
#include "StationTimetableDisplayData.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace
{
    const FLinearColor DefaultForegroundColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E59244FF")));
    const FLinearColor DefaultAuxiliaryColor = FLinearColor::White;
    const FLinearColor DefaultBackgroundColor = FLinearColor::Black;

    struct FServiceRow
    {
        AFGTrain* Train = nullptr;
        bool DockedHere = false;
        bool HeadsHere = false;
        bool DestinationIsConnectedStation = false;
        float EstimatedSeconds = TNumericLimits<float>::Max();
        FString NextStop;
    };

    constexpr float EstimatedTravelSpeed = 1200.0f;
    constexpr float RouteDistanceFactor = 1.15f;
    constexpr float StandardStationWaitSeconds = 30.0f;

    bool AreDisplayDataEqual(const FStationTimetableDisplayData& A, const FStationTimetableDisplayData& B)
    {
        if (A.bStationConnected != B.bStationConnected ||
            !A.StationName.EqualTo(B.StationName) ||
            A.Trains.Num() != B.Trains.Num())
        {
            return false;
        }

        for (int32 Index = 0; Index < A.Trains.Num(); ++Index)
        {
            const FStationTimetableTrainDisplayData& Left = A.Trains[Index];
            const FStationTimetableTrainDisplayData& Right = B.Trains[Index];
            if (!Left.TrainName.EqualTo(Right.TrainName) ||
                !Left.Destination.EqualTo(Right.Destination) ||
                !Left.Eta.EqualTo(Right.Eta) ||
                Left.Status != Right.Status)
            {
                return false;
            }
        }
        return true;
    }

    int32 FindStopIndex(const TArray<FTimeTableStop>& Stops, int32 StartIndex, const AFGTrainStationIdentifier* Station)
    {
        if (Stops.IsEmpty() || !Station) return INDEX_NONE;
        const int32 NormalizedStart = (StartIndex % Stops.Num() + Stops.Num()) % Stops.Num();
        for (int32 Offset = 0; Offset < Stops.Num(); ++Offset)
        {
            const int32 Index = (NormalizedStart + Offset) % Stops.Num();
            if (Stops[Index].Station == Station) return Index;
        }
        return INDEX_NONE;
    }

    float EstimateArrivalSeconds(AFGTrain* Train, const TArray<FTimeTableStop>& Stops, int32 CurrentStopIndex,
        const AFGTrainStationIdentifier* Destination)
    {
        if (!Train || Stops.IsEmpty() || !Destination) return TNumericLimits<float>::Max();
        if (Train->IsDocked() && Train->mDockedAtStation == Destination->GetStation()) return 0.0f;

        int32 StartIndex = (CurrentStopIndex % Stops.Num() + Stops.Num()) % Stops.Num();
        FVector SegmentStart = Train->GetRealActorLocation();
        float Seconds = 0.0f;

        if (Train->IsDocked() && IsValid(Train->mDockedAtStation))
        {
            AFGTrainStationIdentifier* DockedIdentifier = Train->mDockedAtStation->GetStationIdentifier();
            const int32 DockedIndex = FindStopIndex(Stops, StartIndex, DockedIdentifier);
            if (DockedIndex != INDEX_NONE) StartIndex = (DockedIndex + 1) % Stops.Num();
            SegmentStart = Train->mDockedAtStation->GetActorLocation();
            Seconds += FMath::Max(0.0f,
                StandardStationWaitSeconds - Train->mDockedAtStation->GetCurrentDockForDuration());
        }

        for (int32 Offset = 0; Offset < Stops.Num(); ++Offset)
        {
            const int32 StopIndex = (StartIndex + Offset) % Stops.Num();
            AFGTrainStationIdentifier* StopIdentifier = Stops[StopIndex].Station;
            AFGBuildableRailroadStation* StopStation = StopIdentifier ? StopIdentifier->GetStation() : nullptr;
            if (!IsValid(StopStation)) continue;

            Seconds += FVector::Distance(SegmentStart, StopStation->GetActorLocation()) * RouteDistanceFactor / EstimatedTravelSpeed;
            if (StopIdentifier == Destination) return Seconds;

            Seconds += StandardStationWaitSeconds;
            SegmentStart = StopStation->GetActorLocation();
        }
        return TNumericLimits<float>::Max();
    }

    FString EtaText(const FServiceRow& Row)
    {
        if (!Row.Train || !FMath::IsFinite(Row.EstimatedSeconds)) return TEXT("—");
        const float Seconds = Row.EstimatedSeconds;
        return Seconds < 60.0f ? TEXT("< 1 MIN") : FString::Printf(TEXT("%d MIN"), FMath::CeilToInt(Seconds / 60.0f));
    }

    EStationTimetableTrainStatus TrainStatus(const FServiceRow& Row)
    {
        if (Row.Train && Row.Train->IsDocked()) return EStationTimetableTrainStatus::AtStation;
        if (!Row.Train || !Row.Train->IsSelfDrivingEnabled()) return EStationTimetableTrainStatus::Unknown;
        if (Row.Train->mSelfDrivingData.TimeWaitingAtSignal > KINDA_SMALL_NUMBER) return EStationTimetableTrainStatus::AtSignal;
        return Row.HeadsHere ? EStationTimetableTrainStatus::Driving : EStationTimetableTrainStatus::Unknown;
    }
}

AStationTimetableBuildable::AStationTimetableBuildable()
{
    bReplicates = true;
    TimetableFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationTimetableFrame"));
    TimetableFrameMesh->SetMobility(EComponentMobility::Static);
    TimetableFrameMesh->SetupAttachment(RootComponent);
    static ConstructorHelpers::FClassFinder<AFGStandaloneSignHologram> VanillaSignHologram(
        TEXT("/Game/FactoryGame/Buildable/Factory/-Shared/Holo_StandaloneSign_Large")
    );
    if (VanillaSignHologram.Succeeded()) mHologramClass = VanillaSignHologram.Class;
    mDisplayName = NSLOCTEXT("StationTimetable", "BuildableName", "Station Timetable");
    mDescription = NSLOCTEXT("StationTimetable", "BuildableDescription", "Displays the train services of the nearest station.");

    static ConstructorHelpers::FClassFinder<AFGBuildableWidgetSign> VanillaSign(TEXT("/Game/FactoryGame/Buildable/Factory/SignDigital/Build_StandaloneWidgetSign_Large"));
    if (VanillaSign.Succeeded())
    {
        const AFGBuildableWidgetSign* Source = VanillaSign.Class->GetDefaultObject<AFGBuildableWidgetSign>();
        static const FName SignPropertyNames[] = {
            TEXT("mWorldDimensions"), TEXT("mPoleOffset"), TEXT("mPoleScale"), TEXT("mSignToSignOffset"),
            TEXT("mSignDrawSize"), TEXT("mGainSignificanceDistance")
        };
        for (const FName PropertyName : SignPropertyNames)
        {
            if (FProperty* Property = VanillaSign.Class->FindPropertyByName(PropertyName)) Property->CopyCompleteValue_InContainer(this, Source);
        }
#if !UE_SERVER
        if (FProperty* WidgetClassProperty = VanillaSign.Class->FindPropertyByName(TEXT("mWidgetClass")))
        {
            WidgetClassProperty->CopyCompleteValue_InContainer(this, Source);
        }
#endif
        TInlineComponentArray<UStaticMeshComponent*> Components(Source);
        const auto CopyMeshComponent = [](UStaticMeshComponent* Target, const UStaticMeshComponent* SourceComponent)
        {
            if (!Target || !SourceComponent) return;
            Target->SetStaticMesh(SourceComponent->GetStaticMesh());
            Target->SetRelativeTransform(SourceComponent->GetRelativeTransform());
            for (int32 MaterialIndex = 0; MaterialIndex < SourceComponent->GetNumMaterials(); ++MaterialIndex)
            {
                Target->SetMaterial(MaterialIndex, SourceComponent->GetMaterial(MaterialIndex));
            }
        };
        if (const UStaticMeshComponent* const* SourceProxy = Components.FindByPredicate([](const UStaticMeshComponent* Component)
        {
            return Component && Component->GetName() == TEXT("ProxyMesh");
        }))
        {
            CopyMeshComponent(mSignProxyPlane, *SourceProxy);
        }
        if (const UStaticMeshComponent* const* SourceFrame = Components.FindByPredicate([](const UStaticMeshComponent* Component)
        {
            return Component && Component->GetName() == TEXT("SignMeshProxy");
        }))
        {
            CopyMeshComponent(TimetableFrameMesh, *SourceFrame);
        }
    }

    if (FProperty* InteractWidgetProperty =
        AFGBuildable::StaticClass()->FindPropertyByName(TEXT("mInteractWidgetSoftClass")))
    {
        if (TSoftClassPtr<UFGInteractWidget>* InteractWidgetClass =
            InteractWidgetProperty->ContainerPtrToValuePtr<TSoftClassPtr<UFGInteractWidget>>(this))
        {
            *InteractWidgetClass = TSoftClassPtr<UFGInteractWidget>(FSoftObjectPath(
                TEXT("/Game/FactoryGame/Interface/UI/InGame/Signs/BPW_SignInteractWidget.BPW_SignInteractWidget_C")));
        }
    }

#if !UE_SERVER
    static ConstructorHelpers::FObjectFinder<UMaterial> WidgetMaterial(TEXT("/Game/FactoryGame/Buildable/Factory/SignDigital/Material/MM_SignWidgetMaterial.MM_SignWidgetMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterial> EmissiveMaterial(TEXT("/Game/FactoryGame/Buildable/Factory/SignDigital/Material/MM_SignEmissive.MM_SignEmissive"));
    static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Game/FactoryGame/Buildable/Factory/SignDigital/Material/MM_SignRT.MM_SignRT"));
    mWidgetMaterial = WidgetMaterial.Object;
    mEmissiveOnlySignMaterial = EmissiveMaterial.Object;
    mDefaultSignMaterial = DefaultMaterial.Object;
    static ConstructorHelpers::FClassFinder<UUserWidget> SignLayoutManagerBlueprint(
        TEXT("/Game/FactoryGame/Interface/UI/InGame/Signs/BPW_SignLayoutManager"));
#endif
    TimetableWidgetLayout = TSoftClassPtr<UFGSignPrefabWidget>(FSoftObjectPath(
        TEXT("/StationTimetable/Widgets/WBP_StationTimetable.WBP_StationTimetable_C")));
    TimetableSignTypeClass = UStationTimetableSignTypeDescriptor::StaticClass();
    UStationTimetableSignTypeDescriptor* SignTypeDefaults = TimetableSignTypeClass->GetDefaultObject<UStationTimetableSignTypeDescriptor>();
    SignTypeDefaults->mPrefabArray.Reset();
    SignTypeDefaults->mPrefabArray.Add(TimetableWidgetLayout);
    mSignTypeDescriptor = TimetableSignTypeClass;
    mSoftActivePrefabLayout = TimetableWidgetLayout;
#if !UE_SERVER
    if (SignLayoutManagerBlueprint.Succeeded()) mWidgetClass = SignLayoutManagerBlueprint.Class;
#endif
    mForegroundColor = DefaultForegroundColor;
    mAuxilaryColor = DefaultAuxiliaryColor;
    mBackgroundColor = DefaultBackgroundColor;
    mEmissive = 1.0f;
    mGlossiness = 1.0f;
}

bool AStationTimetableBuildable::IsUseable_Implementation() const
{
    return true;
}

void AStationTimetableBuildable::OnUse_Implementation(AFGCharacterPlayer* ByCharacter, const FUseState& State)
{
    Super::OnUse_Implementation(ByCharacter, State);
}

UFGFactoryClipboardSettings* AStationTimetableBuildable::CopySettings_Implementation()
{
    UFGFactoryClipboardSettings* Settings = Super::CopySettings_Implementation();
    if (UFGSignClipboardSettings* SignSettings = Cast<UFGSignClipboardSettings>(Settings))
    {
        SignSettings->mPrefabSignData.TextElementData.Reset();
        SignSettings->mPrefabSignData.IconElementData.Reset();
        SignSettings->mPrefabSignData.PrefabLayout = TimetableWidgetLayout;
        SignSettings->mPrefabSignData.SignTypeDesc = TimetableSignTypeClass;
    }
    return Settings;
}

bool AStationTimetableBuildable::PasteSettings_Implementation(UFGFactoryClipboardSettings* Settings, AFGPlayerController* Player)
{
    const UFGSignClipboardSettings* SourceSettings = Cast<UFGSignClipboardSettings>(Settings);
    if (!SourceSettings) return false;

    UFGSignClipboardSettings* SanitizedSettings = DuplicateObject<UFGSignClipboardSettings>(SourceSettings, GetTransientPackage());
    FPrefabSignData CurrentData;
    GetSignPrefabData(CurrentData);
    SanitizedSettings->mPrefabSignData.TextElementData = MoveTemp(CurrentData.TextElementData);
    SanitizedSettings->mPrefabSignData.IconElementData = MoveTemp(CurrentData.IconElementData);
    SanitizedSettings->mPrefabSignData.PrefabLayout = TimetableWidgetLayout;
    SanitizedSettings->mPrefabSignData.SignTypeDesc = TimetableSignTypeClass;
    return Super::PasteSettings_Implementation(SanitizedSettings, Player);
}

void AStationTimetableBuildable::PreSerializedToBlueprint()
{
    Super::PreSerializedToBlueprint();
    LinkedStationBeforeBlueprintSerialization = LinkedStation;
    LinkedStation = nullptr;
}

void AStationTimetableBuildable::PostSerializedToBlueprint()
{
    LinkedStation = LinkedStationBeforeBlueprintSerialization;
    LinkedStationBeforeBlueprintSerialization = nullptr;
    Super::PostSerializedToBlueprint();
}

void AStationTimetableBuildable::PostSerializedFromBlueprint(bool IsBlueprintWorld)
{
    LinkedStation = nullptr;
    LinkedStationBeforeBlueprintSerialization = nullptr;
    Super::PostSerializedFromBlueprint(IsBlueprintWorld);
}

void AStationTimetableBuildable::BeginPlay()
{
    Super::BeginPlay();
    STATION_TIMETABLE_DEV_LOG(Display,
        TEXT("Sign setup %s manager=%s layout=%s descriptor=%s interact=%s"),
        *GetName(),
        *GetNameSafe(mWidgetClass),
        *mSoftActivePrefabLayout.ToString(),
        *GetNameSafe(mSignTypeDescriptor),
        *GetNameSafe(GetInteractWidgetClass()));
    if (HasAuthority())
    {
        if (IsBuildableInsideBlueprintDesigner())
        {
            LinkedStation = nullptr;
        }
        else if (!IsValid(LinkedStation))
        {
            LinkedStation = FindNearestStation(this, GetActorLocation());
        }
    }
    RefreshTimetableData();
    GetWorldTimerManager().SetTimer(
        RefreshTimer,
        this,
        &AStationTimetableBuildable::RefreshTimetableData,
        1.0f,
        true,
        0.25f);
    STATION_TIMETABLE_DEV_LOG(Display, TEXT("Display %s linkedStation=%s uses Vanilla sign renderer"), *GetName(), *GetNameSafe(LinkedStation));
}

void AStationTimetableBuildable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(RefreshTimer);
    Super::EndPlay(EndPlayReason);
}

void AStationTimetableBuildable::PostLoadGame_Implementation(int32 SaveVersion, int32 GameVersion)
{
    Super::PostLoadGame_Implementation(SaveVersion, GameVersion);
}

void AStationTimetableBuildable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AStationTimetableBuildable, LinkedStation);
    DOREPLIFETIME(AStationTimetableBuildable, ReplicatedTimetableData);
    DOREPLIFETIME(AStationTimetableBuildable, TimetableRevision);
}

void AStationTimetableBuildable::OnRep_LinkedStation()
{
    STATION_TIMETABLE_DEV_LOG(Verbose, TEXT("Display %s received linked station %s"), *GetName(), *GetNameSafe(LinkedStation));
}

void AStationTimetableBuildable::OnRep_TimetableRevision()
{
#if !UE_SERVER
    const bool bNewRevision = LastObservedClientRevision != TimetableRevision;
    LastObservedClientRevision = TimetableRevision;
    if (bNewRevision)
    {
        STATION_TIMETABLE_DEV_LOG(Display,
            TEXT("Client observed timetable revision=%u sign=%s station=%s services=%d guid=%u"),
            TimetableRevision,
            *GetName(),
            *ReplicatedTimetableData.StationName.ToString(),
            ReplicatedTimetableData.Trains.Num(),
            mCachedGUID);
    }
    UpdateRuntimeWidget();
#endif
}

#if !UE_SERVER
bool AStationTimetableBuildable::UpdateRuntimeWidget()
{
    const auto LogFailureOnce = [this](const TCHAR* Reason)
    {
        if (LastRuntimeDiagnosticRevision != TimetableRevision)
        {
            LastRuntimeDiagnosticRevision = TimetableRevision;
            STATION_TIMETABLE_DEV_LOG(Warning,
                TEXT("Runtime timetable update deferred revision=%u sign=%s guid=%u reason=%s"),
                TimetableRevision, *GetName(), mCachedGUID, Reason);
        }
        return false;
    };

    if (!GetWorld() || mCachedGUID == 0)
    {
        return LogFailureOnce(TEXT("world-or-guid-unavailable"));
    }

    AFGSignSubsystem* SignSubsystem = AFGSignSubsystem::Get(GetWorld());
    UWidgetComponent* WidgetComponent = SignSubsystem ? SignSubsystem->GetWidgetByGUID(mCachedGUID) : nullptr;
    if (!IsValid(WidgetComponent))
    {
        return LogFailureOnce(TEXT("vanilla-widget-component-unavailable"));
    }

    if (LastRuntimeWidgetComponent.Get() == WidgetComponent && LastAppliedRuntimeRevision == TimetableRevision)
    {
        return true;
    }

    UUserWidget* RootWidget = WidgetComponent->GetUserWidgetObject();
    if (!IsValid(RootWidget))
    {
        return LogFailureOnce(TEXT("root-widget-unavailable"));
    }

    TArray<UStationTimetableWidget*> TimetableWidgets;
    if (UStationTimetableWidget* RootTimetableWidget = Cast<UStationTimetableWidget>(RootWidget))
    {
        TimetableWidgets.Add(RootTimetableWidget);
    }
    if (UStationTimetableWidget* NamedRenderer =
        Cast<UStationTimetableWidget>(RootWidget->GetWidgetFromName(TEXT("TimetableRenderer"))))
    {
        TimetableWidgets.AddUnique(NamedRenderer);
    }
    if (FObjectProperty* RendererProperty = FindFProperty<FObjectProperty>(
        RootWidget->GetClass(), TEXT("TimetableRenderer")))
    {
        if (UStationTimetableWidget* PropertyRenderer = Cast<UStationTimetableWidget>(
            RendererProperty->GetObjectPropertyValue_InContainer(RootWidget)))
        {
            TimetableWidgets.AddUnique(PropertyRenderer);
        }
    }
    if (RootWidget->WidgetTree)
    {
        RootWidget->WidgetTree->ForEachWidgetAndDescendants([&TimetableWidgets](UWidget* Widget)
        {
            if (UStationTimetableWidget* TimetableWidget = Cast<UStationTimetableWidget>(Widget))
            {
                TimetableWidgets.AddUnique(TimetableWidget);
            }
        });
    }
    ForEachObjectWithOuter(WidgetComponent, [&TimetableWidgets](UObject* Object)
    {
        if (UStationTimetableWidget* TimetableWidget = Cast<UStationTimetableWidget>(Object))
        {
            TimetableWidgets.AddUnique(TimetableWidget);
        }
    }, true, RF_ClassDefaultObject, EInternalObjectFlags::Garbage);
    if (TimetableWidgets.IsEmpty())
    {
        const FString FailureReason = FString::Printf(
            TEXT("station-timetable-widget-not-found-root-%s"),
            *GetNameSafe(RootWidget->GetClass()));
        return LogFailureOnce(*FailureReason);
    }

    FPrefabSignData SignData;
    GetSignPrefabData(SignData);
    SignData.PrefabLayout = TimetableWidgetLayout;
    SignData.SignTypeDesc = TimetableSignTypeClass;
    ReplicatedTimetableData.WriteToSignData(SignData);
    for (UStationTimetableWidget* TimetableWidget : TimetableWidgets)
    {
        if (IsValid(TimetableWidget))
        {
            TimetableWidget->SetRuntimeTimetableData(SignData);
            TimetableWidget->InvalidateLayoutAndVolatility();
        }
    }
    RootWidget->InvalidateLayoutAndVolatility();
    RootWidget->ForceLayoutPrepass();
    WidgetComponent->RequestRedraw();
    WidgetComponent->TickComponent(GetWorld()->GetDeltaSeconds(), LEVELTICK_All, nullptr);
    LastRuntimeWidgetComponent = WidgetComponent;
    LastAppliedRuntimeRevision = TimetableRevision;
    LastRuntimeDiagnosticRevision = TimetableRevision;
    STATION_TIMETABLE_DEV_LOG(Display,
        TEXT("Updated %d persistent timetable widgets revision=%u sign=%s guid=%u component=%s root=%s eta=%s"),
        TimetableWidgets.Num(),
        TimetableRevision,
        *GetName(),
        mCachedGUID,
        *GetNameSafe(WidgetComponent),
        *GetNameSafe(RootWidget),
        ReplicatedTimetableData.Trains.IsEmpty() ? TEXT("-") : *ReplicatedTimetableData.Trains[0].Eta.ToString());
    return true;
}
#endif

bool AStationTimetableBuildable::IsSignEditorOpen() const
{
    if (!GetInteractingPlayers().IsEmpty()) return true;
#if !UE_SERVER
    if (!GetWorld()) return false;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PlayerController = It->Get();
        const AFGHUD* Hud = PlayerController ? PlayerController->GetHUD<AFGHUD>() : nullptr;
        if (!Hud || !Hud->GetGameUI()) continue;
        TArray<UFGInteractWidget*> OpenWidgets;
        Hud->GetGameUI()->GetInteractWidgetsOfInteractObject(const_cast<AStationTimetableBuildable*>(this), OpenWidgets);
        if (!OpenWidgets.IsEmpty()) return true;
    }
#endif
    return false;
}

AFGBuildableRailroadStation* AStationTimetableBuildable::FindNearestStation(const UObject* WorldContext, const FVector& Location, float MaxDistanceFromStationEdge)
{
    if (!WorldContext || !WorldContext->GetWorld()) return nullptr;
    AFGBuildableRailroadStation* Nearest = nullptr;
    float BestDistanceSquared = FMath::Square(MaxDistanceFromStationEdge);
    for (TActorIterator<AFGBuildableRailroadStation> It(WorldContext->GetWorld()); It; ++It)
    {
        const float DistanceSquared = It->GetComponentsBoundingBox(true).ComputeSquaredDistanceToPoint(Location);
        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Nearest = *It;
        }
    }
    return Nearest;
}

void AStationTimetableBuildable::RefreshTimetableData()
{
    if (!HasAuthority() && IsSignEditorOpen())
    {
        if (!bRefreshPausedForInteraction)
        {
            bRefreshPausedForInteraction = true;
            STATION_TIMETABLE_DEV_LOG(Display,
                TEXT("Paused timetable updates while sign editor is open: %s"), *GetName());
        }
        return;
    }
    if (bRefreshPausedForInteraction)
    {
        bRefreshPausedForInteraction = false;
        STATION_TIMETABLE_DEV_LOG(Display,
            TEXT("Resumed timetable updates after sign editor closed: %s"), *GetName());
    }

    if (!HasAuthority())
    {
        OnRep_TimetableRevision();
        return;
    }

    if (IsBuildableInsideBlueprintDesigner())
    {
        LinkedStation = nullptr;
    }
    else if (!IsValid(LinkedStation))
    {
        LinkedStation = FindNearestStation(this, GetActorLocation());
    }
    FStationTimetableDisplayData DisplayData;
    AFGTrainStationIdentifier* Identifier = LinkedStation ? LinkedStation->GetStationIdentifier() : nullptr;
    DisplayData.bStationConnected = Identifier != nullptr;
    DisplayData.StationName = FText::FromString(
        Identifier ? Identifier->GetStationName().ToString().ToUpper() :
            NSLOCTEXT("StationTimetable", "NoStationConnected", "NO STATION CONNECTED").ToString());
    TArray<FServiceRow> Services;
    if (Identifier)
    {
        TArray<AFGTrain*> Trains;
        if (AFGRailroadSubsystem* Railroad = AFGRailroadSubsystem::Get(this)) Railroad->GetAllTrains(Trains);
        for (AFGTrain* Train : Trains)
        {
            AFGRailroadTimeTable* TimeTable = Train ? Train->GetTimeTable() : nullptr;
            if (!TimeTable) continue;
            TArray<FTimeTableStop> Stops;
            TimeTable->GetStops(Stops);
            if (!Stops.ContainsByPredicate([Identifier, this](const FTimeTableStop& Stop)
                {
                    return Stop.Station == Identifier ||
                        (Stop.Station && Stop.Station->GetStation() == LinkedStation);
                })) continue;
            FServiceRow& Row = Services.AddDefaulted_GetRef();
            Row.Train = Train;
            const int32 CurrentStopIndex = TimeTable->GetCurrentStop();
            const FTimeTableStop CurrentStop = TimeTable->GetStop(CurrentStopIndex);
            Row.DockedHere = Train->IsDocked() && Train->mDockedAtStation == LinkedStation;
            Row.HeadsHere = CurrentStop.Station == Identifier;
            Row.DestinationIsConnectedStation = CurrentStop.Station &&
                (CurrentStop.Station == Identifier || CurrentStop.Station->GetStation() == LinkedStation);
            Row.NextStop = CurrentStop.Station ? CurrentStop.Station->GetStationName().ToString() :
                NSLOCTEXT("StationTimetable", "UnknownDestination", "Unknown").ToString();
            Row.EstimatedSeconds = EstimateArrivalSeconds(Train, Stops, CurrentStopIndex, Identifier);
        }
    }

    TArray<FServiceRow> OrderedServices;
    if (!Services.IsEmpty())
    {
        int32 PrimaryIndex = 0;
        for (int32 Index = 1; Index < Services.Num(); ++Index)
        {
            const FServiceRow& Candidate = Services[Index];
            const FServiceRow& Primary = Services[PrimaryIndex];
            if ((Candidate.DockedHere && !Primary.DockedHere) ||
                (Candidate.DockedHere == Primary.DockedHere && Candidate.HeadsHere && !Primary.HeadsHere) ||
                (Candidate.DockedHere == Primary.DockedHere && Candidate.HeadsHere == Primary.HeadsHere &&
                    Candidate.EstimatedSeconds < Primary.EstimatedSeconds))
            {
                PrimaryIndex = Index;
            }
        }
        OrderedServices.Add(Services[PrimaryIndex]);
        Services.RemoveAt(PrimaryIndex);
        Services.Sort([](const FServiceRow& A, const FServiceRow& B)
        {
            return A.EstimatedSeconds < B.EstimatedSeconds;
        });
        OrderedServices.Append(Services);
    }

    for (int32 Index = 0; Index < FMath::Min(OrderedServices.Num(), 5); ++Index)
    {
        const FServiceRow& Service = OrderedServices[Index];
        FStationTimetableTrainDisplayData& Train = DisplayData.Trains.AddDefaulted_GetRef();
        Train.TrainName = Service.Train->GetTrainName();
        Train.Destination = Service.DestinationIsConnectedStation
            ? FText::FromString(TEXT("{THIS_STATION}"))
            : FText::FromString(Service.NextStop);
        Train.Eta = FText::FromString(EtaText(Service));
        Train.Status = Index == 0
            ? TrainStatus(Service)
            : (Service.Train->mSelfDrivingData.TimeWaitingAtSignal > KINDA_SMALL_NUMBER
                ? EStationTimetableTrainStatus::AtSignal
                : EStationTimetableTrainStatus::Driving);
    }

    const bool bInitialPublish = !bHasPublishedTimetableData;
    if (bInitialPublish)
    {
        FPrefabSignData SignData;
        GetSignPrefabData(SignData);
        SignData.PrefabLayout = TimetableWidgetLayout;
        SignData.SignTypeDesc = TimetableSignTypeClass;
        const int32 ExistingServiceCount = FCString::Atoi(*SignData.TextElementData.FindRef(TEXT("ServiceCount")));
        const bool bPreserveLoadedServices = DisplayData.Trains.IsEmpty() && ExistingServiceCount > 0;
        if (bPreserveLoadedServices)
        {
            STATION_TIMETABLE_DEV_LOG(Display,
                TEXT("Preserving %d loaded timetable services for %s until railroad data is ready"),
                ExistingServiceCount, *GetName());
            DisplayData = FStationTimetableDisplayData::FromSignData(SignData);
        }
        else
        {
            DisplayData.WriteToSignData(SignData);
        }
        bHasPublishedTimetableData = true;
        STATION_TIMETABLE_DEV_LOG(Display,
            TEXT("Initializing persistent timetable preset %s station=%s entries=%d services=%s previousGuid=%u"),
            *GetName(),
            *GetNameSafe(LinkedStation),
            SignData.TextElementData.Num(),
            *SignData.TextElementData.FindRef(TEXT("ServiceCount")),
            mCachedGUID);
        SetPrefabSignData(SignData, false);
    }

    if (!AreDisplayDataEqual(DisplayData, ReplicatedTimetableData) || TimetableRevision == 0)
    {
        const FString PreviousEta = ReplicatedTimetableData.Trains.IsEmpty()
            ? TEXT("-")
            : ReplicatedTimetableData.Trains[0].Eta.ToString();
        ReplicatedTimetableData = DisplayData;
        ++TimetableRevision;
        FlushNetDormancy();
        ForceNetUpdate();
        STATION_TIMETABLE_DEV_LOG(Display,
            TEXT("Timetable data changed revision=%u sign=%s station=%s services=%d eta=%s previousEta=%s"),
            TimetableRevision,
            *GetName(),
            *DisplayData.StationName.ToString(),
            DisplayData.Trains.Num(),
            DisplayData.Trains.IsEmpty() ? TEXT("-") : *DisplayData.Trains[0].Eta.ToString(),
            *PreviousEta);
#if UE_SERVER
        if constexpr (StationTimetableDevelopmentDiagnostics)
        {
            UE_LOG(LogStationTimetable, Display,
                TEXT("Replicated timetable revision=%u sign=%s station=%s services=%d nextEta=%s"),
                TimetableRevision,
                *GetName(),
                *DisplayData.StationName.ToString(),
                DisplayData.Trains.Num(),
                DisplayData.Trains.IsEmpty() ? TEXT("-") : *DisplayData.Trains[0].Eta.ToString());
        }
#endif
    }

#if !UE_SERVER
    UpdateRuntimeWidget();
#endif
}
