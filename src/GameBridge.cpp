// Smart Villager & Structure Radar - created by ChrisNiruu
#include "GameBridge.hpp"

#include <atomic>
#include <chrono>
#include <string_view>

#include <pl/Logger.hpp>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include "Probe.hpp"
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

// ---- Probe hooks: pass-through detours that copy argument memory when probe mode is on.
using Fn4 = void (*)(void *, void *, void *, void *);
Fn4 g_origLevelCamera = nullptr, g_origPlayPrep = nullptr, g_origDataDriven = nullptr;

void detourLevelCamera(void *t, void *ctx, void *view, void *client) {
  probe::capture(1, ctx);
  probe::capture(5, view);
  probe::capture(2, client);
  if (g_origLevelCamera) g_origLevelCamera(t, ctx, view, client);
}
void detourPlayPrep(void *t, void *screenCtx, void *levelRenderer, void *actor) {
  probe::capture(3, screenCtx);
  probe::capture(4, actor);
  if (g_origPlayPrep) g_origPlayPrep(t, screenCtx, levelRenderer, actor);
}
void detourDataDriven(void *t, void *ctx, void *renderData, void *) {
  probe::capture(0, renderData);
  if (g_origDataDriven) g_origDataDriven(t, ctx, renderData, nullptr);
}

// Resolves and hooks one function. Returns true when the hook is installed.
bool installOne(const char *name, void *detour, Fn4 *orig) {
  auto &log = pl::log::Logger::getOrCreate("VillagerRadar");
  for (const auto &e : sigs::kEntries) {
    if (std::string_view(e.name) != name || e.pattern[0] == '\0') continue;
    const auto addr = pl::memory::resolveSignature(e.pattern, "libminecraftpe.so");
    if (addr == 0) {
      log.warn("probe: {} not found in this build", name);
      return false;
    }
    if (pl::memory::hook(reinterpret_cast<pl::memory::FuncPtr>(addr),
                         reinterpret_cast<pl::memory::FuncPtr>(detour),
                         reinterpret_cast<pl::memory::FuncPtr *>(orig)) != 0) {
      log.warn("probe: hook failed for {}", name);
      return false;
    }
    return true;
  }
  return false;
}

class NativeSource final : public SnapshotSource {
 public:
  // Nothing risky happens at load time: no signature scans, no hooks.
  NativeSource() {
    for (const auto &e : sigs::kEntries)
      if (e.required && e.pattern[0] == '\0') mMissing = true;
  }

  bool poll(Snapshot &, const RadarConfig &, float, float) override {
    // Live reading (ClientInstance capture, actor list walk, structure queries) is implemented
    // against sigs::kOffsets once they are filled in. Until then: fail closed, draw nothing.
    return false;
  }
  std::string status() const override {
    return mMissing ? "live data not available yet (game offsets not configured)" : "ready";
  }

 private:
  bool mMissing = false;
};

}  // namespace

int installProbeHooks() {
  static std::atomic<bool> attempted{false};
  static std::atomic<int> count{0};
  if (attempted.exchange(true)) return count.load();
  int n = 0;
  n += installOne("LevelRendererCamera::render", reinterpret_cast<void *>(&detourLevelCamera), &g_origLevelCamera);
  n += installOne("InGamePlayScreen::_renderLevelPrep", reinterpret_cast<void *>(&detourPlayPrep), &g_origPlayPrep);
  n += installOne("DataDrivenRenderer::render", reinterpret_cast<void *>(&detourDataDriven), &g_origDataDriven);
  count.store(n);
  return n;
}

std::unique_ptr<SnapshotSource> makeDemoSource() { return std::make_unique<DemoSource>(); }
std::unique_ptr<SnapshotSource> makeNativeSource() { return std::make_unique<NativeSource>(); }

}  // namespace radar
