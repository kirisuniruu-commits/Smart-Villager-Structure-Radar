// Smart Villager & Structure Radar - created by ChrisNiruu
// LeviLauncher (Android) native preloader module: lifecycle, Mod Menu settings, overlay thread.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <pl/ModMenuConfig.hpp>

#include "GameBridge.hpp"
#include "radar/Config.hpp"
#include "radar/Renderer.hpp"

namespace {

constexpr const char *kModuleId = "chrisniruu.smart_villager_structure_radar";
constexpr std::size_t kMaxCommands = 1500;  // safety cap for low-end devices

using pl::modmenu::ConfigControlTypeV2;
using pl::modmenu::ConfigNodeV2;
using pl::modmenu::ConfigOptionV2;

std::string boolStr(bool v) { return v ? "true" : "false"; }

// Converts renderer primitives into Mod Menu HUD draw commands.
void toDrawCommands(const std::vector<radar::Primitive> &prims,
                    std::vector<pl::modmenu::DrawCommand> &out) {
  using pl::modmenu::DrawCommand;
  using pl::modmenu::DrawCommandType;
  out.clear();
  out.reserve(std::min(prims.size() * 2, kMaxCommands));

  auto line = [&](float x1, float y1, float x2, float y2, float size, std::uint32_t color) {
    DrawCommand c;
    c.type = DrawCommandType::Line;
    c.x = x1; c.y = y1; c.w = x2 - x1; c.h = y2 - y1;  // HUD lines use (x,y)->(x+w,y+h)
    c.size = size; c.color = color;
    out.push_back(std::move(c));
  };

  for (const auto &p : prims) {
    if (out.size() >= kMaxCommands) break;
    DrawCommand c;
    c.color = p.color;
    switch (p.type) {
      case radar::PrimType::Line:
        line(p.x1, p.y1, p.x2, p.y2, p.size, p.color);
        continue;
      case radar::PrimType::Rect:  // outline drawn as 4 lines (stroke width semantics are not relied on)
        line(p.x1, p.y1, p.x1 + p.w, p.y1, p.size, p.color);
        line(p.x1 + p.w, p.y1, p.x1 + p.w, p.y1 + p.h, p.size, p.color);
        line(p.x1 + p.w, p.y1 + p.h, p.x1, p.y1 + p.h, p.size, p.color);
        line(p.x1, p.y1 + p.h, p.x1, p.y1, p.size, p.color);
        continue;
      case radar::PrimType::RectFilled:
        c.type = DrawCommandType::RectFilled;
        c.x = p.x1; c.y = p.y1; c.w = p.w; c.h = p.h;
        break;
      case radar::PrimType::CircleFilled:
        c.type = DrawCommandType::CircleFilled;
        c.x = p.x1; c.y = p.y1; c.size = p.size;  // radius = size
        break;
      case radar::PrimType::TriangleFilled:
        c.type = DrawCommandType::TriangleFilled;
        c.x = p.x1; c.y = p.y1; c.w = p.x2; c.h = p.y2; c.x3 = p.x3; c.y3 = p.y3;  // absolute points
        break;
      case radar::PrimType::Text:
        c.type = DrawCommandType::Text;
        c.x = p.x1; c.y = p.y1; c.size = p.size;
        c.w = p.align == 1 ? -2.0f : 0.0f;  // -2 = centered text
        c.text = p.text;
        break;
    }
    out.push_back(std::move(c));
  }
}

std::string buildSchemaJson(const radar::RadarConfig &d) {
  pl::modmenu::ConfigSchemaBuilder b;
  b.category("main", "Radar").defaultCategory("main");

  auto node = [&](std::string key, std::string title, ConfigControlTypeV2 type, std::string def,
                  std::string section, std::string desc = {}) {
    ConfigNodeV2 n;
    n.id = key; n.key = key; n.category = "main"; n.title = std::move(title);
    n.type = type; n.defaultValue = std::move(def); n.section = std::move(section);
    n.description = std::move(desc);
    return n;
  };
  auto toggle = [&](const char *key, const char *title, bool def, const char *section,
                    const char *desc) {
    b.node(node(key, title, ConfigControlTypeV2::Toggle, boolStr(def), section, desc));
  };

  toggle("professionBadges", "Profession Badges & Tier Icons", d.professionBadges, "Villager Radar",
         "Floating icon above villagers showing profession and trade tier.");
  toggle("restockTimer", "Trade Restock Timer", d.restockTimer, "Villager Radar",
         "Countdown / status ring showing when trades restock.");
  toggle("zombieHighlight", "Zombie Villager Highlight", d.zombieHighlight, "Villager Radar",
         "Color-codes curable zombie villagers.");
  toggle("workstationPointer", "Unclaimed Workstation Pointer", d.workstationPointer,
         "Villager Radar", "Points unemployed villagers toward free lecterns, composters, smithing tables.");

  toggle("villageBoundary", "Village Boundary Ring", d.villageBoundary, "Structure Radar",
         "Village center and boundary perimeter on the ground.");
  toggle("boundingBoxes", "Fortress & Outpost Bounding Boxes", d.boundingBoxes, "Structure Radar",
         "Bounding boxes for nether fortresses and pillager outposts.");
  toggle("distanceVectors", "Monument / Stronghold Distance Vector", d.distanceVectors,
         "Structure Radar", "Direction + distance indicators toward nearby structures.");

  {
    auto n = node("renderDistance", "Radar Render Distance", ConfigControlTypeV2::SliderInt,
                  std::to_string(d.renderDistance), "Visual Tuning");
    n.minValue = "16"; n.maxValue = "128"; n.step = "8"; n.unit = "blocks";
    b.node(std::move(n));
  }
  {
    auto n = node("opacity", "Overlay Opacity", ConfigControlTypeV2::SliderFloat, "0.85",
                  "Visual Tuning");
    n.minValue = "0.1"; n.maxValue = "1.0"; n.step = "0.05";
    b.node(std::move(n));
  }
  {
    auto n = node("scale", "Overlay Scale", ConfigControlTypeV2::SliderFloat, "1.0", "Visual Tuning");
    n.minValue = "0.5"; n.maxValue = "2.0"; n.step = "0.1";
    b.node(std::move(n));
  }
  {
    auto n = node("mode", "Performance Preset", ConfigControlTypeV2::Choice, "performance",
                  "Visual Tuning", "Performance: simple 2D icons and lines. Quality: glowing, animated.");
    auto option = [](const char *value, const char *label, const char *desc) {
      ConfigOptionV2 o;
      o.value = value; o.label = label; o.description = desc;
      return o;
    };
    n.options.push_back(option("performance", "Performance Mode", "Simple 2D icons and basic lines"));
    n.options.push_back(option("quality", "Quality Mode", "Glowing vectors, scaled icons, animated rings"));
    b.node(std::move(n));
  }
  toggle("demoMode", "Demo Mode (test scene)", d.demoMode, "Diagnostics",
         "Draws a synthetic scene so you can test the menu and both presets.");

  {
    auto n = node("credits", radar::kCredit, ConfigControlTypeV2::Info, "", "Credits");
    b.node(std::move(n));
  }
  return b.toJson();
}

class RadarMod {
 public:
  bool load(pl::mod::ModContext &ctx) {
    mLog = &ctx.logger();
    mDemo = radar::makeDemoSource();
    mNative = radar::makeNativeSource();

    const bool ok =
        pl::modmenu::ModuleBuilder(kModuleId, "Smart Villager & Structure Radar")
            .description(std::string("Villager and structure overlays. ") + radar::kCredit)
            .modId(ctx.id())
            .defaultEnabled(true)
            .config("professionBadges", "Profession Badges & Tier Icons", pl::modmenu::ConfigType::Toggle, "true")
            .config("restockTimer", "Trade Restock Timer", pl::modmenu::ConfigType::Toggle, "true")
            .config("zombieHighlight", "Zombie Villager Highlight", pl::modmenu::ConfigType::Toggle, "true")
            .config("workstationPointer", "Unclaimed Workstation Pointer", pl::modmenu::ConfigType::Toggle, "true")
            .config("villageBoundary", "Village Boundary Ring", pl::modmenu::ConfigType::Toggle, "true")
            .config("boundingBoxes", "Fortress & Outpost Bounding Boxes", pl::modmenu::ConfigType::Toggle, "true")
            .config("distanceVectors", "Monument / Stronghold Distance Vector", pl::modmenu::ConfigType::Toggle, "true")
            .config("renderDistance", "Radar Render Distance", pl::modmenu::ConfigType::SliderInt, "64", "16", "128")
            .config("opacity", "Overlay Opacity", pl::modmenu::ConfigType::SliderFloat, "0.85", "0.1", "1.0")
            .config("scale", "Overlay Scale", pl::modmenu::ConfigType::SliderFloat, "1.0", "0.5", "2.0")
            .config("mode", "Performance Preset", pl::modmenu::ConfigType::Radio, "0", "Performance,Quality")
            .config("demoMode", "Demo Mode (test scene)", pl::modmenu::ConfigType::Toggle, "false")
            .onToggle([this](std::string_view, bool enabled) { setEnabled(enabled); })
            .onConfigChanged([this](std::string_view, std::string_view key, std::string_view value) {
              std::lock_guard<std::mutex> lock(mMutex);
              // Radio entries report the option index (0/1); the schema Choice reports a name.
              if (key == "mode" && (value == "0" || value == "1"))
                mCfg.mode = value == "1" ? radar::QualityMode::Quality : radar::QualityMode::Performance;
              else
                radar::applyConfig(mCfg, key, value);
            })
            .registerModule();
    if (!ok) {
      mLog->error("could not register Mod Menu module");
      return false;
    }
#ifdef RADAR_USE_SCHEMA
    if (!pl::modmenu::setConfigSchemaJson(kModuleId, buildSchemaJson(mCfg)))
      mLog->warn("config schema rejected; credits are still shown in the module description");
#endif
    return true;
  }

  bool enable(pl::mod::ModContext &) {
    if (mRunning.exchange(true)) return true;
    mThread = std::thread([this] { loop(); });
    mLog->info("{}", radar::kCredit);
    return true;
  }

  bool disable(pl::mod::ModContext &) {
    stop();
    pl::modmenu::submitDrawCommands(kModuleId, {});
    pl::modmenu::unregisterModule(kModuleId);
    return true;
  }

  bool unload(pl::mod::ModContext &) {
    stop();
    return true;
  }

 private:
  void setEnabled(bool enabled) {
    {
      std::lock_guard<std::mutex> lock(mMutex);
      mCfg.enabled = enabled;
    }
    if (!enabled) pl::modmenu::submitDrawCommands(kModuleId, {});  // clears instantly, no restart
  }

  void stop() {
    if (!mRunning.exchange(false)) return;
    if (mThread.joinable()) mThread.join();
  }

  void loop() {
    std::vector<pl::modmenu::DrawCommand> cmds;
    radar::Snapshot snap;
    bool wasDrawing = false;
    std::string lastStatus;

    while (mRunning.load()) {
      radar::RadarConfig cfg;
      {
        std::lock_guard<std::mutex> lock(mMutex);
        cfg = mCfg;
      }
      const auto frame = std::chrono::milliseconds(cfg.mode == radar::QualityMode::Quality ? 22 : 40);

      if (!cfg.enabled) {
        if (wasDrawing) {
          pl::modmenu::submitDrawCommands(kModuleId, {});
          wasDrawing = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        continue;
      }

      const auto surface = pl::modmenu::getHudSurfaceSize();
      radar::SnapshotSource &src = cfg.demoMode ? *mDemo : *mNative;
      const bool live = surface.width > 0 && surface.height > 0 &&
                        src.poll(snap, cfg, surface.width, surface.height);

      if (live) {
        snap.camera.screenW = surface.width;
        snap.camera.screenH = surface.height;
        toDrawCommands(radar::buildOverlay(snap, cfg), cmds);
        pl::modmenu::submitDrawCommands(kModuleId, cmds);
        wasDrawing = true;
      } else if (wasDrawing) {
        pl::modmenu::submitDrawCommands(kModuleId, {});
        wasDrawing = false;
      }

      const std::string st = src.status();
      if (!live && st != lastStatus) {
        mLog->info("radar idle: {}", st);
        lastStatus = st;
      }
      std::this_thread::sleep_for(frame);
    }
  }

  pl::log::Logger *mLog = nullptr;
  std::unique_ptr<radar::SnapshotSource> mDemo, mNative;
  std::mutex mMutex;
  radar::RadarConfig mCfg;
  std::atomic<bool> mRunning{false};
  std::thread mThread;
};

RadarMod g_mod;

}  // namespace

PL_REGISTER_MOD(RadarMod, g_mod)
