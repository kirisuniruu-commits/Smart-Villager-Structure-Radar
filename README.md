# Smart Villager & Structure Radar

**Smart Villager & Structure Radar created by ChrisNiruu**

A native (`.so`) preloader module for **LeviLauncher Android** (Minecraft Bedrock) that draws villager and
structure overlays on the HUD. GitHub Actions compiles the library and packages a ready-to-install
`.levipack` on every commit.

## Features

| Group | Option |
| --- | --- |
| Master | Module enable/disable toggle (takes effect instantly, no restart) |
| Villager Radar | Profession badges + tier icons, trade restock timer, zombie villager highlight, unclaimed workstation pointer |
| Structure Radar | Village boundary ring, fortress / outpost bounding boxes, ocean monument / stronghold distance vectors |
| Tuning | Render distance 16-128 blocks, overlay opacity, overlay scale |
| Presets | **Performance** (flat 2D icons, basic lines) / **Quality** (glow, perspective-scaled icons, animated rings) |
| Diagnostics | **Demo mode**: synthetic scene to test the menu and both presets on-device |

## Status (read this first)

| Part | State |
| --- | --- |
| Menu, settings, master switch, presets | Implemented |
| Projection math, overlay rendering for every feature | Implemented, unit-tested on a PC (`tests/test_radar.cpp`) |
| Demo mode | Implemented |
| **Live game data** (reading villagers / structures from Minecraft) | **Not implemented yet** - needs byte signatures and struct offsets for your exact Minecraft build, see [docs/SIGNATURES.md](docs/SIGNATURES.md). Until then the native bridge fails closed: nothing is hooked, nothing is drawn. |

**Probe mode** (Diagnostics): shows raw game memory as hex on the HUD so struct offsets can be worked out from a screenshot. Hooks for `LevelRendererCamera::render`, `InGamePlayScreen::_renderLevelPrep` and `DataDrivenRenderer::render` were located in Minecraft 1.26.52.03 and are verified unique in that build.

Nothing in this repo has been run inside the game yet. Android-side code was only syntax-checked against stub headers.

## Install

1. Push to GitHub, open **Actions**, open the latest run, download the `SmartVillagerStructureRadar-levipack` artifact.
2. Unzip the artifact and import `SmartVillagerStructureRadar.levipack` in LeviLauncher.
3. Open the Mod Menu, find **Smart Villager & Structure Radar**, switch on **Demo Mode** to verify the overlay.

## Build locally

```bash
# PC unit tests
cmake -S . -B build-host -DRADAR_BUILD_TESTS=ON && cmake --build build-host && ctest --test-dir build-host

# Android (NDK r25+)
cmake -S . -B build-android -G Ninja -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DCMAKE_BUILD_TYPE=Release
cmake --build build-android --target villager_radar
```

If the Mod Menu shows duplicated controls, configure with `-DRADAR_USE_SCHEMA=OFF` (credits then show in the module description only).

## Layout

```
include/radar/   Config, Math (projection), Model (snapshot types), Renderer, Demo   <- pure C++, tested
src/RadarMod.cpp Lifecycle, Mod Menu registration, overlay thread
src/GameBridge.* Demo source + native source (fails closed until signatures exist)
src/Signatures.hpp  Signatures / offsets to fill in per Minecraft build
third_party/preloader-android/include   Preloader headers (from the preloader-android source you supplied)
```

## Fair use

This is a client-side convenience overlay. Structure and villager locators can break the rules of multiplayer
servers and Realms - use it in your own worlds or where the server allows it.

## Third-party code

`third_party/preloader-android/include` is copied from the preloader-android sources provided with this project;
check its license before publishing the repository publicly.

## Credits

Smart Villager & Structure Radar created by ChrisNiruu.
