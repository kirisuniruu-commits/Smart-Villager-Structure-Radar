#pragma once
// Smart Villager & Structure Radar - created by ChrisNiruu
// Synthetic scene so the menu, sliders and both render modes can be verified on a device
// before (or without) live game data. Enable with the "Demo mode" switch.

#include <cmath>

#include "radar/Model.hpp"

namespace radar {

inline Snapshot makeDemoSnapshot(const Camera &cam, float seconds) {
  Snapshot s;
  s.valid = true;
  s.camera = cam;
  const Vec3 f = cam.forward(), r = cam.right();
  const Vec3 flat = Vec3{f.x, 0, f.z};
  const float fl = length(flat) > 0.001f ? length(flat) : 1.0f;
  const Vec3 ahead = flat * (1.0f / fl);
  const Vec3 base{cam.pos.x, cam.pos.y - 1.62f, cam.pos.z};
  auto at = [&](float fwd, float side) { return base + ahead * fwd + r * side; };

  const float prog = std::fmod(seconds / 20.0f, 1.0f);
  s.villagers.push_back({at(8, -3), Profession::Librarian, TradeTier::Master, false, prog, (1 - prog) * 20});
  s.villagers.push_back({at(12, 2), Profession::Farmer, TradeTier::Journeyman, false, 1.0f, 0});
  s.villagers.push_back({at(16, 6), Profession::Cleric, TradeTier::Novice, false, 0.3f, 14});
  s.villagers.push_back({at(10, 7), Profession::None, TradeTier::Novice, false, -1, -1});
  s.villagers.push_back({at(14, -7), Profession::None, TradeTier::Novice, true, -1, -1});
  s.workstations.push_back({at(14, 14), WorkstationKind::Lectern, false});

  StructureInfo village;
  village.kind = StructureKind::Village;
  village.center = at(20, 0);
  village.radius = 32;
  s.structures.push_back(village);

  StructureInfo fort;
  fort.kind = StructureKind::NetherFortress;
  fort.boundsMin = at(30, -10) + Vec3{0, 0, 0};
  fort.boundsMax = at(44, 10) + Vec3{0, 9, 0};
  fort.center = (fort.boundsMin + fort.boundsMax) * 0.5f;
  // keep min/max ordered for the AABB distance test
  Vec3 mn{std::min(fort.boundsMin.x, fort.boundsMax.x), std::min(fort.boundsMin.y, fort.boundsMax.y),
          std::min(fort.boundsMin.z, fort.boundsMax.z)};
  Vec3 mx{std::max(fort.boundsMin.x, fort.boundsMax.x), std::max(fort.boundsMin.y, fort.boundsMax.y),
          std::max(fort.boundsMin.z, fort.boundsMax.z)};
  fort.boundsMin = mn;
  fort.boundsMax = mx;
  s.structures.push_back(fort);

  StructureInfo mon;
  mon.kind = StructureKind::OceanMonument;
  mon.center = at(150, 80);
  s.structures.push_back(mon);

  StructureInfo sh;
  sh.kind = StructureKind::Stronghold;
  sh.center = at(-120, -60);
  s.structures.push_back(sh);
  return s;
}

}  // namespace radar
