#pragma once

#include "CoreMinimal.h"
#include "FGSignTypes.h"
#include "StationTimetableDisplayData.h"
#include "StationTimetableWidget.generated.h"

class UBorder;
class UImage;
class UPanelWidget;
class UStationTimetableMarquee;
class UTextBlock;
class UTexture2D;

UCLASS(Blueprintable)
class STATIONTIMETABLE_API UStationTimetableWidget : public UFGSignPrefabWidget
{
    GENERATED_BODY()

public:
    UStationTimetableWidget();

    UFUNCTION(BlueprintCallable, Category = "Station Timetable")
    void SetTimetableData(const FPrefabSignData& InData);

    UFUNCTION(BlueprintCallable, Category = "Station Timetable")
    void UpdateTimetable(const FStationTimetableDisplayData& Data);

    UFUNCTION(BlueprintImplementableEvent, Category = "Station Timetable", meta = (DisplayName = "Timetable Updated"))
    void OnTimetableUpdated(const FStationTimetableDisplayData& Data);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void ApplyColors();
    void ApplyTrainRow(int32 Index, const FStationTimetableTrainDisplayData* Train);
    UTexture2D* GetStatusTexture(EStationTimetableTrainStatus Status) const;
    UFGSignPrefabWidget* FindOuterPrefabWidget() const;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UStationTimetableWidget> TimetableRenderer;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> HeaderStationIcon;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> DefaultHeaderIcon;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> StatusAtStationIcon;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> StatusAtSignalIcon;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> StatusDrivingIcon;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> StatusUnknownIcon;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UStationTimetableMarquee> StationName;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> NextSectionTitleText;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> NextStatusBackground;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> NextStatusIcon;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UStationTimetableMarquee> NextTrainName;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> NextDestinationCaption;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> NextDestinationText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> NextEtaCaption;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> NextEtaText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> OtherSectionTitleText;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> EmptyState;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EmptyStateMessage;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> OtherRow1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> OtherStatus1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UStationTimetableMarquee> OtherTrainName1;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> OtherEtaText1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> OtherRow2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> OtherStatus2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UStationTimetableMarquee> OtherTrainName2;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> OtherEtaText2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> OtherRow3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> OtherStatus3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UStationTimetableMarquee> OtherTrainName3;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> OtherEtaText3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> OtherRow4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> OtherStatus4;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UStationTimetableMarquee> OtherTrainName4;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> OtherEtaText4;
};
