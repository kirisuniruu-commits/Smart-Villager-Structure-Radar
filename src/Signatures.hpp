#pragma once
// Smart Villager & Structure Radar - created by ChrisNiruu
//
// Byte signatures / offsets for the game hooks. They are specific to ONE Minecraft Bedrock
// build and must be filled in from libminecraftpe.so for the version you run
// (see docs/SIGNATURES.md). Empty pattern = "not configured": the native bridge then
// stays inactive (fail closed) and nothing is hooked or read.

namespace radar::sigs {

struct Entry {
  const char *name;
  const char *pattern;  // IDA-style: "FD 7B ?? A9 ..."
  bool required;
};

inline constexpr Entry kEntries[] = {
    // --- Verified unique in libminecraftpe.so 1.26.52.03 (found via embedded source-location records).
    // Used by Probe mode. Prologue bytes only; "?? ?? ?? ??" = a PC-relative instruction.
    {"LevelRendererCamera::render",
     "EB 2B B8 6D E9 23 01 6D FD 7B 02 A9 FC 6F 03 A9 FA 67 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9 FD 83 00 91 FF 43 24 D1",
     false},
    {"InGamePlayScreen::_renderLevelPrep",
     "EB 2B B8 6D E9 23 01 6D FD 7B 02 A9 FC 6F 03 A9 FA 67 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9 FD 83 00 91 FF 43 17 D1",
     false},
    {"DataDrivenRenderer::render",
     "FF C3 05 D1 EC 73 00 FD EB 2B 0F 6D E9 23 10 6D FD 7B 11 A9 FC 6F 12 A9 FA 67 13 A9 F8 5F 14 A9 F6 57 15 A9 F4 4F 16 A9 FD 43 04 91 48 D0 3B D5",
     false},
    // --- Still to be found (needed for live data):
    // Called once per client frame/tick. Used to capture the ClientInstance and poll state.
    {"ClientInstance::update", "", true},
    // Level actor enumeration (villagers, zombie villagers).
    {"Level::getRuntimeActorList", "", true},
};

// Struct offsets (bytes) for the targeted build. -1 = unknown.
struct Offsets {
  int clientInstanceLocalPlayer = -1;
  int actorPosition = -1;
  int actorRotation = -1;
  int actorTypeId = -1;
  int villagerProfession = -1;
  int villagerTradeTier = -1;
  int villagerRestockTicks = -1;
};
inline constexpr Offsets kOffsets{};

}  // namespace radar::sigs
