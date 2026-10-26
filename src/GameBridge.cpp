// Smart Villager & Structure Radar - created by ChrisNiruu
#include "GameBridge.hpp"

#include <chrono>

#include <pl/Logger.hpp>
#include <pl/memory/Signature.hpp>

#include "Signatures.hpp"
#include "radar/Demo.hpp"

namespace radar {
namespace {

class DemoSource final : public SnapshotSource {
 public:
  bool poll(Snapshot &out, const RadarConfig &cfg, float w, float h) override {
    using clock = std::chrono::steady_clock;
    static const auto t0 = clock::now();
    const float t = std::chrono::duration<float>(clock::now() - t0).count();
    Camera cam;
    cam.pos = {0, 64, 0};
    cam.yawDeg = std::fmod(t * 12.0f, 360.0f);
    cam.pitchDeg = 8.0f;
    cam.fovYDeg = cfg.fallbackFov;
    cam.screenW = w;
    cam.screenH = h;
    out = makeDemoSnapshot(cam, t);
    return true;
  }
  std::string status() const override { return "demo scene"; }
};

class NativeSource final : public SnapshotSource {
 public:
  NativeSource() {
    auto &log = pl::log::Logger::getOrCreate("VillagerRadar");
    for (const auto &e : sigs::kEntries) {
      if (e.pattern[0] == '\0') {
        if (e.required) mMissing = true;
        continue;
      }
      if (pl::memory::resolveSignature(e.pattern, "libminecraftpe.so") == 0 && e.required)
        mMissing = true;
    }
    mStatus = mMissing ? "inactive: game signatures not configured for this Minecraft build"
                       : "signatures resolved";
    log.info("native bridge: {}", mStatus);
  }

  bool poll(Snapshot &, const RadarConfig &, float, float) override {
    // Live reading (ClientInstance capture, actor list walk, structure queries) is implemented
    // against sigs::kOffsets once they are filled in. Until then: fail closed, draw nothing.
    return false;
  }
  std::string status() const override { return mStatus; }

 private:
  bool mMissing = false;
  std::string mStatus;
};

}  // namespace

std::unique_ptr<SnapshotSource> makeDemoSource() { return std::make_unique<DemoSource>(); }
std::unique_ptr<SnapshotSource> makeNativeSource() { return std::make_unique<NativeSource>(); }

}  // namespace radar
