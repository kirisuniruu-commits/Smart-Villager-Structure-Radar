#pragma once
// Smart Villager & Structure Radar - created by ChrisNiruu
#include <memory>
#include <string>

#include "radar/Config.hpp"
#include "radar/Model.hpp"

namespace radar {

// Anything that can produce a Snapshot of the world once per poll.
class SnapshotSource {
 public:
  virtual ~SnapshotSource() = default;
  // Fills `out` and returns true when live data is available.
  virtual bool poll(Snapshot &out, const RadarConfig &cfg, float surfaceW, float surfaceH) = 0;
  virtual std::string status() const = 0;
};

// Synthetic scene (spinning camera) for testing the menu and renderer on a device.
std::unique_ptr<SnapshotSource> makeDemoSource();

// Reads the real game through hooks. Inactive until signatures/offsets are configured.
std::unique_ptr<SnapshotSource> makeNativeSource();

// Installs the probe hooks once (only called when Probe Mode is first switched on).
// Returns how many of the 3 hooks are active.
int installProbeHooks();

}  // namespace radar
