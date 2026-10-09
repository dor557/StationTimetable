#include "StationTimetableMarquee.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"


void UStationTimetableMarquee::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    ClipBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ClipBox"));
    ClipBox->SetClipping(EWidgetClipping::ClipToBounds);
    WidgetTree->RootWidget = ClipBox;

    TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text"));
    TextBlock->SetAutoWrapText(false);
    ClipBox->SetContent(TextBlock);
}

void UStationTimetableMarquee::UpdateFontSize(float AvailableHeight)
{
    if (!TextBlock || AvailableHeight <= 1.0f)
    {
        return;
    }

    const float TargetHeight =
        FMath::Max(1.0f, AvailableHeight - (VerticalPadding * 2.0f));
    const int32 EstimatedFontSize = FMath::RoundToInt(TargetHeight * 0.75f);
    const int32 BestFontSize = FMath::Clamp(((EstimatedFontSize + 1) / 2) * 2, 8, 128);

    if (BestFontSize != AppliedFontSize)
    {
        FSlateFontInfo Font = TextBlock->GetFont();
        Font.Size = BestFontSize;

        TextBlock->SetFont(Font);
        AppliedFontSize = BestFontSize;
    }
}

void UStationTimetableMarquee::Configure(const FText& Text, const FLinearColor& Color, bool Bold)
{
    if (bHasConfiguration && ConfiguredText.EqualTo(Text) && ConfiguredColor.Equals(Color) && bConfiguredBold == Bold)
    {
        return;
    }

    ConfiguredText = Text;
    ConfiguredColor = Color;
    bConfiguredBold = Bold;
    bHasConfiguration = true;
    Elapsed = 0.0f;
    if (!ClipBox || !TextBlock)
    {
        return;
    }
    ClipBox->ClearWidthOverride();
    TextBlock->SetText(Text);
    TextBlock->SetColorAndOpacity(FSlateColor(Color));
    FSlateFontInfo Font = TextBlock->GetFont();
    Font.TypefaceFontName = Bold ? TEXT("Bold") : TEXT("Regular");
    TextBlock->SetFont(Font);
    TextBlock->SetRenderTranslation(FVector2D::ZeroVector);
    AppliedFontSize = INDEX_NONE;
    LastMeasuredHeight = -1.0f;
}

void UStationTimetableMarquee::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!TextBlock)
    {
        return;
    }

    const FVector2D LocalSize = MyGeometry.GetLocalSize();
    if (LocalSize.X > 1.0f)
    {
        FieldWidth = FMath::Max(FieldWidth, LocalSize.X);
    }
    if (LocalSize.Y > 1.0f)
    {
        StableHeight = FMath::Max(StableHeight, LocalSize.Y);
        if (!FMath::IsNearlyEqual(StableHeight, LastMeasuredHeight, 0.5f))
        {
            LastMeasuredHeight = StableHeight;
            UpdateFontSize(StableHeight);
        }
    }

    Elapsed += InDeltaTime;
    const float Overflow = FMath::Max(0.0f, TextBlock->GetDesiredSize().X - FieldWidth);
    if (Overflow <= 1.0f)
    {
        TextBlock->SetRenderTranslation(FVector2D::ZeroVector);
        return;
    }

    constexpr float Pause = 1.5f;
    constexpr float Speed = 45.0f;
    const float TravelTime = Overflow / Speed;
    const float Cycle = Pause + TravelTime + Pause;
    const float Phase = FMath::Fmod(Elapsed, Cycle);
    const float Offset = Phase <= Pause ? 0.0f : Phase < Pause + TravelTime ? (Phase - Pause) * Speed : Overflow;
    TextBlock->SetRenderTranslation(FVector2D(-Offset, 0.0f));
}
