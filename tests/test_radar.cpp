// Host-side checks for the pure radar logic (no Android needed).
#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>

#include "radar/Config.hpp"
#include "radar/Demo.hpp"
#include "radar/Math.hpp"
#include "radar/Renderer.hpp"

using namespace radar;

static bool near(float a, float b, float eps = 0.5f) { return std::fabs(a - b) < eps; }
static int count(const std::vector<Primitive> &v, PrimType t) {
  int n = 0; for (auto &p : v) n += p.type == t; return n;
}

int main() {
  Camera cam; cam.pos = {0, 64, 0}; cam.screenW = 2000; cam.screenH = 1000;

  // yaw 0 faces +Z: straight ahead -> screen centre
  auto c = cam.project({0, 64, 10});
  assert(c && near(c->x, 1000) && near(c->y, 500));
  // facing +Z, the player's right is -X
  auto r = cam.project({-5, 64, 10});
  assert(r && r->x > 1000);
  auto l = cam.project({5, 64, 10});
  assert(l && l->x < 1000);
  // above -> smaller y on screen
  auto u = cam.project({0, 70, 10});
  assert(u && u->y < 500);
  // behind the camera is culled
  assert(!cam.project({0, 64, -10}));
  // bearing: target on the right is positive
  assert(cam.bearingTo({-10, 64, 10}) > 0 && cam.bearingTo({10, 64, 10}) < 0);
  // yaw 90 faces -X
  Camera c90 = cam; c90.yawDeg = 90;
  auto w = c90.project({-10, 64, 0});
  assert(w && near(w->x, 1000));
  // looking down (pitch>0) puts a ground point near the centre
  Camera cd = cam; cd.pitchDeg = 90;
  auto g = cd.project({0, 54, 0.001f});
  assert(g && near(g->x, 1000, 2) );

  // config clamping + parsing
  RadarConfig cfg;
  assert(applyConfig(cfg, "renderDistance", "999") && cfg.renderDistance == 128);
  assert(applyConfig(cfg, "renderDistance", "1") && cfg.renderDistance == 16);
  assert(applyConfig(cfg, "opacity", "5") && near(cfg.opacity, 1.0f, 1e-4f));
  assert(applyConfig(cfg, "scale", "0") && near(cfg.scale, 0.5f, 1e-4f));
  assert(applyConfig(cfg, "mode", "quality") && cfg.mode == QualityMode::Quality);
  assert(!applyConfig(cfg, "nope", "1"));
  cfg = RadarConfig{};

  Snapshot snap = makeDemoSnapshot(cam, 5.0f);
  // master switch / no data -> nothing drawn
  RadarConfig off; off.enabled = false;
  assert(buildOverlay(snap, off).empty());
  Snapshot none; assert(buildOverlay(none, cfg).empty());

  // performance mode: no filled circles; quality mode: circles present
  cfg.mode = QualityMode::Performance;
  auto perf = buildOverlay(snap, cfg);
  assert(!perf.empty() && count(perf, PrimType::CircleFilled) == 0);
  cfg.mode = QualityMode::Quality;
  auto qual = buildOverlay(snap, cfg);
  assert(count(qual, PrimType::CircleFilled) > 0 && qual.size() > perf.size());

  // each toggle removes its own output
  auto withOnly = [&](auto setter) { RadarConfig x; x.professionBadges = x.restockTimer = x.zombieHighlight =
      x.workstationPointer = x.villageBoundary = x.boundingBoxes = x.distanceVectors = false; setter(x);
      return buildOverlay(snap, x); };
  assert(withOnly([](RadarConfig &) {}).empty());
  assert(!withOnly([](RadarConfig &x) { x.professionBadges = true; }).empty());
  assert(!withOnly([](RadarConfig &x) { x.zombieHighlight = true; }).empty());
  assert(!withOnly([](RadarConfig &x) { x.workstationPointer = true; }).empty());
  assert(!withOnly([](RadarConfig &x) { x.villageBoundary = true; }).empty());
  assert(!withOnly([](RadarConfig &x) { x.boundingBoxes = true; }).empty());
  assert(!withOnly([](RadarConfig &x) { x.distanceVectors = true; }).empty());
  // restock only matters together with badges
  assert(withOnly([](RadarConfig &x) { x.restockTimer = true; }).empty());

  // render distance slider culls villagers: at 16 blocks only the near ones remain
  RadarConfig near16; near16.renderDistance = 16; near16.professionBadges = true;
  near16.zombieHighlight = near16.workstationPointer = near16.villageBoundary =
      near16.boundingBoxes = near16.distanceVectors = false;
  RadarConfig far128 = near16; far128.renderDistance = 128;
  assert(buildOverlay(snap, near16).size() < buildOverlay(snap, far128).size());

  // opacity scales alpha
  RadarConfig faint; faint.opacity = 0.1f;
  auto fp = buildOverlay(snap, faint);
  for (auto &p : fp) assert(((p.color >> 24) & 0xFF) <= 26);

  // every primitive has finite coordinates
  for (auto &p : qual) assert(std::isfinite(p.x1) && std::isfinite(p.y1) && std::isfinite(p.x2) && std::isfinite(p.y2));

  std::printf("all radar tests passed (perf=%zu quality=%zu primitives)\n", perf.size(), qual.size());
  return 0;
}
