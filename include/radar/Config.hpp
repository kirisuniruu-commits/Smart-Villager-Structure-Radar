#pragma once
// Smart Villager & Structure Radar - created by ChrisNiruu
// Pure settings model. No game or Android dependencies, so it is unit-testable on a PC.

#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>

namespace radar {

enum class QualityMode { Performance = 0, Quality = 1 };

struct RadarConfig {
  // 1. Master switch
  bool enabled = true;

  // 2. Villager radar
  bool professionBadges = true;
  bool restockTimer = true;
  bool zombieHighlight = true;
  bool workstationPointer = true;

  // 3. Structure radar
  bool villageBoundary = true;
  bool boundingBoxes = true;    // nether fortress + pillager outpost
  bool distanceVectors = true;  // ocean monument + stronghold

  // 4. Visual tuning
  int renderDistance = 64;  // blocks, 16..128
  float opacity = 0.85f;    // 0.1..1.0
  float scale = 1.0f;       // 0.5..2.0
  QualityMode mode = QualityMode::Performance;

  // Vertical FOV (degrees) used when the game bridge cannot read the real one
  float fallbackFov = 70.0f;

  // Draws synthetic data so menu + rendering can be tested without game signatures
  bool demoMode = false;

  // Shows raw game memory on the HUD so struct offsets can be found from a screenshot
  bool probeMode = false;
  int probePage = 0;  // 0..5
};

inline bool parseBool(std::string_view v) {
  return v == "true" || v == "1" || v == "True" || v == "on";
}

inline int clampInt(int v, int lo, int hi) { return std::min(std::max(v, lo), hi); }
inline float clampF(float v, float lo, float hi) { return std::min(std::max(v, lo), hi); }

// Applies one Mod Menu config change. Returns false for unknown keys.
inline bool applyConfig(RadarConfig &c, std::string_view key, std::string_view value) {
  const std::string s(value);
  if (key == "professionBadges") c.professionBadges = parseBool(value);
  else if (key == "restockTimer") c.restockTimer = parseBool(value);
  else if (key == "zombieHighlight") c.zombieHighlight = parseBool(value);
  else if (key == "workstationPointer") c.workstationPointer = parseBool(value);
  else if (key == "villageBoundary") c.villageBoundary = parseBool(value);
  else if (key == "boundingBoxes") c.boundingBoxes = parseBool(value);
  else if (key == "distanceVectors") c.distanceVectors = parseBool(value);
  else if (key == "demoMode") c.demoMode = parseBool(value);
  else if (key == "probeMode") c.probeMode = parseBool(value);
  else if (key == "probePage") c.probePage = clampInt(std::atoi(s.c_str()), 0, 5);
  else if (key == "renderDistance") c.renderDistance = clampInt(std::atoi(s.c_str()), 16, 128);
  else if (key == "opacity") c.opacity = clampF(std::strtof(s.c_str(), nullptr), 0.1f, 1.0f);
  else if (key == "scale") c.scale = clampF(std::strtof(s.c_str(), nullptr), 0.5f, 2.0f);
  else if (key == "mode") {
    c.mode = (s == "quality" || s == "1" || s == "Quality") ? QualityMode::Quality
                                                            : QualityMode::Performance;
  } else {
    return false;
  }
  return true;
}

inline constexpr const char *kCredit = "Smart Villager & Structure Radar created by ChrisNiruu";

}  // namespace radar
