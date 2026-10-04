# Changelog

## 0.2.4

- Fixed timetable displays not refreshing after their initial placement.
- Fixed continuous StationTimetable widget and UObject creation during timetable updates.
- Added tested Windows and Linux dedicated-server support.
- Added server-authoritative timetable calculation and reliable replicated display updates.
- Added bounded Vanilla sign refreshes for existing and previously placed displays.
- Added localized `This station` and `Dieser Bahnhof` destination text for trains approaching the connected station.
- Disabled UObject development diagnostics in public Alpha builds.

## 0.2.2

- Added Windows and Linux dedicated-server build support.
- Added server-authoritative timetable calculation and replicated client display data.
- Added reliable timetable refreshes for dormant and previously placed displays.
- Added dedicated-server UObject diagnostics for Alpha and Beta testing.
- Fixed the missing server-side hologram class when entering the build state.

## 0.2.1

Initial public Alpha release.

- Added a station-bound timetable based on the Vanilla small billboard.
- Added next-train and additional-service information with status icons and estimated arrival times.
- Added Vanilla foreground, auxiliary, and background color support.
- Added emissive and glossiness settings.
- Added blueprint-safe station linking and multiplayer replication.
- Added Windows client support for Steam and Epic Games Store.
- Dedicated-server packaging remains disabled until practical server testing is complete.
- Changed runtime timetable updates to reuse existing widgets and avoid continuous UObject growth.
- Added English as native language and prepared German localization.
