#include "StationTimetableContent.h"

#include "StationTimetableBuildable.h"
#include "StationTimetableWidget.h"
#include "Equipment/FGBuildGun.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace
{
    void CopyDescriptorProperty(const FName PropertyName, UFGBuildingDescriptor* Target, const UFGBuildingDescriptor* Source)
    {
        if (FProperty* Property = UFGBuildingDescriptor::StaticClass()->FindPropertyByName(PropertyName))
        {
            Property->CopyCompleteValue_InContainer(Target, Source);
        }
    }
}

UStationTimetableDescriptor::UStationTimetableDescriptor()
{
    static ConstructorHelpers::FClassFinder<AStationTimetableBuildable> TimetableBuildableBlueprint(
        TEXT("/StationTimetable/Blueprints/BP_StationTimetable"));
    mBuildableClass = AStationTimetableBuildable::StaticClass();
    if (TimetableBuildableBlueprint.Succeeded()) mBuildableClass = TimetableBuildableBlueprint.Class;
    mDisplayName = NSLOCTEXT("StationTimetable", "DisplayName", "Station Timetable");
    mDescription = NSLOCTEXT("StationTimetable", "Description", "A small billboard displaying the train services linked to a station.");

    static ConstructorHelpers::FClassFinder<UFGBuildingDescriptor> VanillaDescriptor(
        TEXT("/Game/FactoryGame/Buildable/Factory/SignDigital/Desc_StandaloneWidgetSign_Large"));
    if (VanillaDescriptor.Succeeded())
    {
        const UFGBuildingDescriptor* Source = VanillaDescriptor.Class->GetDefaultObject<UFGBuildingDescriptor>();
        mCategory = Source->GetCategoryFromInstance();
        mSmallIcon = UFGItemDescriptor::GetSmallIcon(VanillaDescriptor.Class);
        mPersistentBigIcon = UFGItemDescriptor::GetBigIcon(VanillaDescriptor.Class);
        CopyDescriptorProperty(TEXT("mSubCategories"), this, Source);
        CopyDescriptorProperty(TEXT("mQuickSwitchGroup"), this, Source);
    }

    static ConstructorHelpers::FClassFinder<UFGBuildingDescriptor> TrainStationDescriptor(
        TEXT("/Game/FactoryGame/Buildable/Factory/Train/Station/Desc_TrainStation"));
    if (TrainStationDescriptor.Succeeded())
    {
        const UFGBuildingDescriptor* Source = TrainStationDescriptor.Class->GetDefaultObject<UFGBuildingDescriptor>();
        mCategory = Source->GetCategoryFromInstance();
        CopyDescriptorProperty(TEXT("mSubCategories"), this, Source);
    }
    mMenuPriority = 50.0f;
}

UStationTimetableRecipe::UStationTimetableRecipe()
{
    mDisplayNameOverride = true;
    mDisplayName = NSLOCTEXT("StationTimetable", "RecipeName", "Station Timetable");
    mProduct.Add(FItemAmount(UStationTimetableDescriptor::StaticClass(), 1));
    mManufactoringDuration = 1.0f;
    mManufacturingMenuPriority = 50.0f;
    mProducedIn.Add(AFGBuildGun::StaticClass());

    static ConstructorHelpers::FClassFinder<UFGRecipe> VanillaSignRecipe(
        TEXT("/Game/FactoryGame/Recipes/Buildings/Recipe_StandaloneWidgetSign_Large"));
    if (VanillaSignRecipe.Succeeded())
    {
        mIngredients = UFGRecipe::GetIngredients(GetTransientPackage(), VanillaSignRecipe.Class);
    }

    static ConstructorHelpers::FClassFinder<UFGRecipe> TrainStationRecipe(
        TEXT("/Game/FactoryGame/Buildable/Factory/Train/Station/Recipe_TrainStation"));
    if (TrainStationRecipe.Succeeded())
    {
        mOverriddenCategory = UFGRecipe::GetCategory(TrainStationRecipe.Class);
    }
}

UStationTimetableNoStationDisqualifier::UStationTimetableNoStationDisqualifier()
{
    mDisqfualifyingText = NSLOCTEXT("StationTimetable", "NoStation", "Must be placed within 5 m of a station boundary");
}

UStationTimetableSignTypeDescriptor::UStationTimetableSignTypeDescriptor()
{
    static ConstructorHelpers::FObjectFinder<UObject> TrainStationIcon(
        TEXT("/Game/FactoryGame/Buildable/Factory/Train/Station/UI/IconDesc_TrainStation_256.IconDesc_TrainStation_256"));
    static ConstructorHelpers::FClassFinder<UFGSignTypeDescriptor> Vanilla2x1Descriptor(
        TEXT("/Game/FactoryGame/Buildable/Factory/-Shared/SignTypes/SignTypeDesc_2x1"));
    mSignCanvasDimensions = FVector2D(800.0f, 400.0f);
    mDefaultForegroundColor = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E59244FF")));
    mDefaultAuxiliaryColor = FLinearColor::White;
    mDefaultBackgroundColor = FLinearColor::Black;
    mTextElementNameMap.Add(TEXT("StationName"), TEXT("Station"));
    if (TrainStationIcon.Succeeded())
    {
        mIconElementNameMap.Add(TEXT("Icon"), TrainStationIcon.Object);
    }
    if (Vanilla2x1Descriptor.Succeeded())
    {
        const UFGSignTypeDescriptor* VanillaDefaults = Vanilla2x1Descriptor.Class->GetDefaultObject<UFGSignTypeDescriptor>();
        if (const TObjectPtr<UObject>* Background = VanillaDefaults->mIconElementNameMap.Find(TEXT("{BG}Background")))
        {
            mIconElementNameMap.Add(TEXT("{BG}Background"), *Background);
        }
    }
    mPrefabArray.Add(UStationTimetableWidget::StaticClass());
}
