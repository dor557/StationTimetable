# Station Timetable

Station Timetable adds a station-bound small billboard that displays the next arriving train and up to four additional train services.

## Short Description

**English:** Adds customizable station displays showing upcoming trains, destinations, status information, and estimated arrival times.

**Deutsch:** Fügt anpassbare Bahnhofsanzeigen hinzu, die kommende Züge, Ziele, Statusinformationen und geschätzte Ankunftszeiten darstellen.

## Author and Support

- Author: dor557
- Ficsit.app profile: https://ficsit.app/user/SykzxA5sax2zK
- Website and documentation: https://www.the-microborgs.de/mods/
- Source code: https://github.com/dor557/StationTimetable
- Contact: development@the-microborgs.de

## License

Copyright (C) 2026 dor557. Licensed under the GNU General Public License v3.0 only (`GPL-3.0-only`).

## Features

- Uses the Vanilla small-billboard mesh, placement behavior, sign materials and editor.
- Links to the nearest railroad station within 5 m of its outer boundary.
- Displays station, train, destination, status and estimated arrival information.
- Supports three Vanilla sign colors, emissive intensity and matte/glossy material settings.
- Updates the existing rendered widget in place without continuously creating sign presets.
- Supports blueprint placement without copying a station reference.
- Supports multiplayer and dedicated-server targets.
- Unlocks with the same Tier 6 train milestone that unlocks the Vanilla Train Station.
- Uses the Vanilla billboard recipe ingredients and construction cost.

## Build Targets

Alpakit is configured for `Windows`, `WindowsServer` and `LinuxServer`.

Do not add a `GameFeature` field to the source `.uplugin`. Alpakit adds it to packaged releases and installs the mod below `FactoryGame/Mods/GameFeatures/StationTimetable`.

## Release Stage

The internal stage is configured in `Source/StationTimetable/Public/StationTimetable.h`:

```cpp
inline constexpr EStationTimetableReleaseStage StationTimetableReleaseStage =
    EStationTimetableReleaseStage::Development;
```

Available stages are `Development`, `Alpha`, `Beta` and `Release`.

The UObject HUD, creation listener and verbose development logs are active only in `Development`. Before creating an Alpha, Beta or Release package, change this constant to the matching stage. Errors required for troubleshooting remain logged.

The internal build number is independent of the public semantic version and is not shown outside Development builds.

## Localization

The native language is English. The plugin localization target `StationTimetable` is configured for English (`en`) and German (`de`) in `Config/PluginLocalization.ini` and registered in `StationTimetable.uplugin`.

To finish localization in Unreal Editor:

1. Open `Tools` → `Localization Dashboard`.
2. Select `StationTimetable` under `Plugin Targets`.
3. Verify that native culture is `English (en)` and German (`de`) is enabled.
4. Open `WBP_StationTimetableRenderer` and make every source text English before gathering: `Next train`, `Destination`, `Arrival in`, `Additional trains`, and `No trains available`.
5. Save and compile the widget asset.
6. Run `Gather Text` for `StationTimetable`.
7. Enter the German equivalents: `Nächster Zug`, `Ziel`, `Ankunft in`, `Weitere Züge`, and `Keine Züge verfügbar`.
8. Translate the gathered C++ entries such as display name, description, placement error, `NO STATION CONNECTED`, and `Unknown`.
9. Run `Count Words`, then `Compile Text`, and save all generated localization files.

Do not create a Game localization target. Plugin localization targets are packaged with the mod.

## Publication Checklist

1. Set the internal release stage to `Alpha`, `Beta`, or `Release`.
2. Update `Version`, `VersionName`, and `SemVersion` consistently.
3. Update `GameVersion` and the SML dependency to the versions actually tested.
4. Compile and test Steam, Epic, Windows dedicated server, and Linux dedicated server.
5. Test host/client color editing, blueprint copy/paste, and save/load behavior.
6. Gather and compile localization after the final asset text changes.
7. Package all targets with Alpakit Release.
8. Upload the combined multi-target archive to ficsit.app and test installation through Satisfactory Mod Manager.

## Current Public Version

- Semantic version: `0.2.1`
- Minimum game build: `502094`
- Required SML range: `^3.12.0`
- Multiplayer: required on host and client
