#pragma once
// Smart Villager & Structure Radar - created by ChrisNiruu
// Plain data the renderer consumes. The game bridge (or demo source) fills this in once per poll.

#include <cstdint>
#include <string>
#include <vector>

#include "radar/Math.hpp"

namespace radar {

enum class Profession : std::uint8_t {
  None, Nitwit, Armorer, Butcher, Cartographer, Cleric, Farmer, Fisherman,
  Fletcher, Leatherworker, Librarian, Mason, Shepherd, Toolsmith, Weaponsmith
};

enum class TradeTier : std::uint8_t { Novice = 1, Apprentice, Journeyman, Expert, Master };

struct VillagerInfo {
  Vec3 pos;
  Profession profession = Profession::None;
  TradeTier tier = TradeTier::Novice;
  bool isZombie = false;
  // Fraction of the restock wait that has elapsed, 0..1. Negative = unknown / not applicable.
  float restockProgress = -1.0f;
  // Seconds until the next restock opportunity. Negative = unknown.
  float restockSeconds = -1.0f;
};

enum class WorkstationKind : std::uint8_t { Lectern, Composter, SmithingTable, Other };

struct WorkstationInfo {
  Vec3 pos;
  WorkstationKind kind = WorkstationKind::Other;
  bool claimed = false;
};

enum class StructureKind : std::uint8_t {
  Village, NetherFortress, PillagerOutpost, OceanMonument, Stronghold
};

struct StructureInfo {
  StructureKind kind = StructureKind::Village;
  Vec3 center;
  Vec3 boundsMin;       // used by fortress / outpost boxes
  Vec3 boundsMax;
  float radius = 0.0f;  // used by village boundary ring
};

struct Snapshot {
  bool valid = false;  // false while the bridge has no live data
  Camera camera;
  std::vector<VillagerInfo> villagers;
  std::vector<WorkstationInfo> workstations;
  std::vector<StructureInfo> structures;
};

inline const char *professionLabel(Profession p) {
  switch (p) {
    case Profession::None: return "-";
    case Profession::Nitwit: return "NW";
    case Profession::Armorer: return "AR";
    case Profession::Butcher: return "BU";
    case Profession::Cartographer: return "CA";
    case Profession::Cleric: return "CL";
    case Profession::Farmer: return "FA";
    case Profession::Fisherman: return "FI";
    case Profession::Fletcher: return "FL";
    case Profession::Leatherworker: return "LE";
    case Profession::Librarian: return "LI";
    case Profession::Mason: return "MA";
    case Profession::Shepherd: return "SH";
    case Profession::Toolsmith: return "TO";
    case Profession::Weaponsmith: return "WE";
  }
  return "?";
}

// ARGB colors (alpha is replaced by the renderer from the opacity setting)
inline std::uint32_t professionColor(Profession p) {
  switch (p) {
    case Profession::Librarian: return 0x00B07CFF;
    case Profession::Cleric: return 0x00FF7AD9;
    case Profession::Farmer: return 0x0074D84A;
    case Profession::Armorer:
    case Profession::Toolsmith:
    case Profession::Weaponsmith: return 0x00A0A8B8;
    case Profession::Cartographer: return 0x0055B8FF;
    case Profession::Nitwit: return 0x00707070;
    case Profession::None: return 0x00FFD060;
    default: return 0x00E8C27A;
  }
}

}  // namespace radar
