#include "StationTimetableWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "StationTimetableMarquee.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void SetText(UTextBlock* Widget, const FText& Text) { if (Widget) Widget->SetText(Text); }
    void SetTextColor(UTextBlock* Widget, const FLinearColor& Color) { if (Widget) Widget->SetColorAndOpacity(FSlateColor(Color)); }
    void SetVisible(UWidget* Widget, bool bVisible)
    {
        if (Widget) Widget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }

    void SetImageTexture(UImage* Widget, UTexture2D* Texture)
    {
        if (Widget && Texture && Widget->GetBrush().GetResourceObject() != Texture)
        {
            Widget->SetBrushFromTexture(Texture, true);
        }
    }
}

UStationTimetableWidget::UStationTimetableWidget()
{
    bContainsIcon = true;
    bContainsOtherIcon = false;
#if !UE_SERVER
    static ConstructorHelpers::FObjectFinder<UTexture2D> TrainIcon(
        TEXT("/Game/FactoryGame/Interface/UI/Assets/MonochromeIcons/TXUI_MIcon_Train.TXUI_MIcon_Train"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> AtStationIcon(
        TEXT("/StationTimetable/UI/Icons/T_ST_Status_AtStation.T_ST_Status_AtStation"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> AtSignalIcon(
        TEXT("/StationTimetable/UI/Icons/T_ST_Status_AtSignal.T_ST_Status_AtSignal"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> DrivingIcon(
        TEXT("/StationTimetable/UI/Icons/T_ST_Status_Driving.T_ST_Status_Driving"));
    static ConstructorHelpers::FObjectFinder<UTexture2D> UnknownIcon(
        TEXT("/StationTimetable/UI/Icons/T_ST_Status_Unknown.T_ST_Status_Unknown"));
    DefaultHeaderIcon = TrainIcon.Object;
    StatusAtStationIcon = AtStationIcon.Object;
    StatusAtSignalIcon = AtSignalIcon.Object;
    StatusDrivingIcon = DrivingIcon.Object;
    StatusUnknownIcon = UnknownIcon.Object;
#endif
}

void UStationTimetableWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    bContainsIcon = true;
    bContainsOtherIcon = false;
}

void UStationTimetableWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (HeaderStationIcon)
    {
        SetImageTexture(HeaderStationIcon, DefaultHeaderIcon);
        TMap<FString, UImage*> IconWidgets;
        IconWidgets.Add(TEXT("Icon"), HeaderStationIcon);
        SetNameToIconWidgetMap(IconWidgets);
    }
    NextSectionTitleText = Cast<UTextBlock>(GetWidgetFromName(TEXT("NextSectionTitle0")));
    EmptyStateMessage = Cast<UTextBlock>(GetWidgetFromName(TEXT("EmptyStateText0")));
    NextDestinationCaption = Cast<UTextBlock>(GetWidgetFromName(TEXT("NextDestinationLabel0")));
    NextDestinationText = Cast<UTextBlock>(GetWidgetFromName(TEXT("NextDestination0")));
    NextEtaCaption = Cast<UTextBlock>(GetWidgetFromName(TEXT("NextEtaLabel0")));
    NextEtaText = Cast<UTextBlock>(GetWidgetFromName(TEXT("NextEta0")));
    OtherSectionTitleText = Cast<UTextBlock>(GetWidgetFromName(TEXT("OtherSectionTitle0")));
    OtherEtaText1 = Cast<UTextBlock>(GetWidgetFromName(TEXT("OtherEta1_")));
    OtherEtaText2 = Cast<UTextBlock>(GetWidgetFromName(TEXT("OtherEta2_")));
    OtherEtaText3 = Cast<UTextBlock>(GetWidgetFromName(TEXT("OtherEta3_")));
    OtherEtaText4 = Cast<UTextBlock>(GetWidgetFromName(TEXT("OtherEta4_")));
    SetColorAndOpacity(FLinearColor::White);
    SetRenderOpacity(1.0f);
    if (!mPrefabSignData.TextElementData.IsEmpty()) SetTimetableData(mPrefabSignData);
}

void UStationTimetableWidget::SetTimetableData(const FPrefabSignData& InData)
{
    const bool bUnchanged =
        InData.TextElementData.OrderIndependentCompareEqual(mPrefabSignData.TextElementData) &&
        InData.IconElementData.OrderIndependentCompareEqual(mPrefabSignData.IconElementData) &&
        InData.ForegroundColor.Equals(mPrefabSignData.ForegroundColor) &&
        InData.BackgroundColor.Equals(mPrefabSignData.BackgroundColor) &&
        InData.AuxiliaryColor.Equals(mPrefabSignData.AuxiliaryColor) &&
        FMath::IsNearlyEqual(InData.Emissive, mPrefabSignData.Emissive) &&
        FMath::IsNearlyEqual(InData.Glossiness, mPrefabSignData.Glossiness);

    mPrefabSignData = InData;
    if (TimetableRenderer && TimetableRenderer != this)
    {
        TimetableRenderer->SetTimetableData(InData);
        return;
    }
    if (!bUnchanged || !StationName) UpdateTimetable(FStationTimetableDisplayData::FromSignData(InData));
}

void UStationTimetableWidget::UpdateTimetable(const FStationTimetableDisplayData& Data)
{
    if (TimetableRenderer && TimetableRenderer != this)
    {
        TimetableRenderer->mPrefabSignData = mPrefabSignData;
        TimetableRenderer->UpdateTimetable(Data);
        return;
    }

    if (StationName) StationName->Configure(Data.StationName, mPrefabSignData.ForegroundColor, true);
    const FStationTimetableTrainDisplayData* NextTrain = Data.Trains.IsValidIndex(0) ? &Data.Trains[0] : nullptr;
    SetVisible(EmptyState, !NextTrain);
    SetVisible(NextStatusBackground, NextTrain != nullptr);
    SetVisible(NextTrainName, NextTrain != nullptr);
    SetVisible(NextDestinationCaption, NextTrain != nullptr);
    SetVisible(NextDestinationText, NextTrain != nullptr);
    SetVisible(NextEtaCaption, NextTrain != nullptr);
    SetVisible(NextEtaText, NextTrain != nullptr);
    SetVisible(OtherSectionTitleText, Data.Trains.Num() > 1);

    if (NextTrain)
    {
        SetImageTexture(NextStatusIcon, GetStatusTexture(NextTrain->Status));
        if (NextTrainName) NextTrainName->Configure(NextTrain->TrainName, mPrefabSignData.ForegroundColor, true);
        SetText(NextDestinationText, NextTrain->Destination);
        SetText(NextEtaText, NextTrain->Eta);
    }

    for (int32 Index = 1; Index < 5; ++Index)
    {
        ApplyTrainRow(Index, Data.Trains.IsValidIndex(Index) ? &Data.Trains[Index] : nullptr);
    }
    ApplyColors();
    OnTimetableUpdated(Data);
}

void UStationTimetableWidget::ApplyTrainRow(int32 Index, const FStationTimetableTrainDisplayData* Train)
{
    UPanelWidget* Row = Index == 1 ? OtherRow1 : Index == 2 ? OtherRow2 : Index == 3 ? OtherRow3 : OtherRow4;
    UImage* Status = Index == 1 ? OtherStatus1 : Index == 2 ? OtherStatus2 : Index == 3 ? OtherStatus3 : OtherStatus4;
    UStationTimetableMarquee* Name = Index == 1 ? OtherTrainName1 : Index == 2 ? OtherTrainName2 : Index == 3 ? OtherTrainName3 : OtherTrainName4;
    UTextBlock* Eta = Index == 1 ? OtherEtaText1 : Index == 2 ? OtherEtaText2 : Index == 3 ? OtherEtaText3 : OtherEtaText4;
    SetVisible(Row, Train != nullptr);
    if (!Train) return;
    SetImageTexture(Status, GetStatusTexture(Train->Status));
    if (Name) Name->Configure(Train->TrainName, mPrefabSignData.ForegroundColor);
    SetText(Eta, Train->Eta);
}

void UStationTimetableWidget::ApplyColors()
{
    const FLinearColor Foreground = mPrefabSignData.ForegroundColor;
    const FLinearColor Auxiliary = mPrefabSignData.AuxiliaryColor;
    if (HeaderStationIcon) HeaderStationIcon->SetColorAndOpacity(Foreground);
    SetTextColor(NextSectionTitleText, Foreground);
    SetTextColor(NextDestinationCaption, Auxiliary);
    SetTextColor(NextDestinationText, Auxiliary);
    SetTextColor(NextEtaCaption, Foreground);
    SetTextColor(NextEtaText, Auxiliary);
    SetTextColor(OtherSectionTitleText, Foreground);
    SetTextColor(EmptyStateMessage, Foreground);
    SetTextColor(OtherEtaText1, Auxiliary);
    SetTextColor(OtherEtaText2, Auxiliary);
    SetTextColor(OtherEtaText3, Auxiliary);
    SetTextColor(OtherEtaText4, Auxiliary);
    if (NextStatusIcon) NextStatusIcon->SetColorAndOpacity(Auxiliary);
    if (OtherStatus1) OtherStatus1->SetColorAndOpacity(Auxiliary);
    if (OtherStatus2) OtherStatus2->SetColorAndOpacity(Auxiliary);
    if (OtherStatus3) OtherStatus3->SetColorAndOpacity(Auxiliary);
    if (OtherStatus4) OtherStatus4->SetColorAndOpacity(Auxiliary);
    if (NextStatusBackground) NextStatusBackground->SetBrushColor(FLinearColor(Auxiliary.R, Auxiliary.G, Auxiliary.B, 0.18f));
}

UTexture2D* UStationTimetableWidget::GetStatusTexture(EStationTimetableTrainStatus Status) const
{
    if (Status == EStationTimetableTrainStatus::Driving) return StatusDrivingIcon;
    if (Status == EStationTimetableTrainStatus::AtSignal) return StatusAtSignalIcon;
    if (Status == EStationTimetableTrainStatus::AtStation) return StatusAtStationIcon;
    return StatusUnknownIcon;
}

void UStationTimetableWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (TimetableRenderer && TimetableRenderer != this)
    {
        if (!mPrefabSignData.TextElementData.IsEmpty()) TimetableRenderer->SetTimetableData(mPrefabSignData);
        return;
    }
    UFGSignPrefabWidget* OuterPrefab = FindOuterPrefabWidget();
    if (!OuterPrefab) return;
    const FPrefabSignData& OuterData = OuterPrefab->mPrefabSignData;
    if (!OuterData.TextElementData.OrderIndependentCompareEqual(mPrefabSignData.TextElementData) ||
        !OuterData.IconElementData.OrderIndependentCompareEqual(mPrefabSignData.IconElementData) ||
        !OuterData.ForegroundColor.Equals(mPrefabSignData.ForegroundColor) ||
        !OuterData.BackgroundColor.Equals(mPrefabSignData.BackgroundColor) ||
        !OuterData.AuxiliaryColor.Equals(mPrefabSignData.AuxiliaryColor) ||
        !FMath::IsNearlyEqual(OuterData.Emissive, mPrefabSignData.Emissive) ||
        !FMath::IsNearlyEqual(OuterData.Glossiness, mPrefabSignData.Glossiness)) SetTimetableData(OuterData);
}

UFGSignPrefabWidget* UStationTimetableWidget::FindOuterPrefabWidget() const
{
    for (UObject* Outer = GetOuter(); Outer; Outer = Outer->GetOuter())
    {
        if (UFGSignPrefabWidget* Prefab = Cast<UFGSignPrefabWidget>(Outer); Prefab && Prefab != this) return Prefab;
    }
    const UWidget* CurrentWidget = this;
    while (const UPanelWidget* Parent = CurrentWidget->GetParent())
    {
        if (UFGSignPrefabWidget* Prefab = Parent->GetTypedOuter<UFGSignPrefabWidget>(); Prefab && Prefab != this) return Prefab;
        CurrentWidget = Parent;
    }
    return nullptr;
}
