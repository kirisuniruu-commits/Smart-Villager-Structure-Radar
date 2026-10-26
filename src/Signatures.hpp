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
