#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StationTimetableMarquee.generated.h"

class USizeBox;
class UTextBlock;

UCLASS()
class STATIONTIMETABLE_API UStationTimetableMarquee final : public UUserWidget
{
    GENERATED_BODY()

public:
    void Configure(const FText& Text, const FLinearColor& Color, bool Bold = false);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void UpdateFontSize(float AvailableHeight);

    UPROPERTY()
    TObjectPtr<USizeBox> ClipBox;

    UPROPERTY()
    TObjectPtr<UTextBlock> TextBlock;

    float FieldWidth = 100.0f;
    float StableHeight = 0.0f;
    float Elapsed = 0.0f;
    FText ConfiguredText;
    FLinearColor ConfiguredColor = FLinearColor::Transparent;
    bool bConfiguredBold = false;
    bool bHasConfiguration = false;

    int32 AppliedFontSize = INDEX_NONE;

    // Sicherheitsabstand oben/unten, damit Descender wie g, p, q, y
    // nicht direkt an der Clip-Grenze liegen.
    float VerticalPadding = 4.0f;

    // Verhindert, dass wir bei jedem Tick neu messen.
    float LastMeasuredHeight = -1.0f;
};
