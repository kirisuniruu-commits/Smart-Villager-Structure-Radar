#pragma once
// Smart Villager & Structure Radar - created by ChrisNiruu
// Turns a Snapshot + RadarConfig into 2D overlay primitives. Pure C++, host-testable.
//
// Primitive semantics mirror the LeviLaunchroid HUD overlay:
//   Line:           (x1,y1)->(x2,y2), `size` = stroke width
//   Rect/RectFill:  x,y,w,h
//   Circle (fill):  center x,y, radius = `size`
//   Triangle:       (x1,y1) (x2,y2) (x3,y3)
//   Text:           x,y = baseline origin, `size` = text size, `align` 0 left / 1 center
// Colors are ARGB.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "radar/Config.hpp"
#include "radar/Model.hpp"

namespace radar {

enum class PrimType { Line, Rect, RectFilled, CircleFilled, TriangleFilled, Text };

struct Primitive {
  PrimType type = PrimType::Line;
  float x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
  float w = 0, h = 0;
  float size = 1;
  int align = 0;
  std::uint32_t color = 0xFFFFFFFF;
  std::string text;
};

namespace detail {

inline std::uint32_t argb(std::uint32_t rgb, float alpha) {
  const auto a = static_cast<std::uint32_t>(clampF(alpha, 0.0f, 1.0f) * 255.0f + 0.5f);
  return (a << 24) | (rgb & 0x00FFFFFFu);
}

struct Ctx {
  const Snapshot &snap;
  const RadarConfig &cfg;
  std::vector<Primitive> &out;
  bool quality() const { return cfg.mode == QualityMode::Quality; }
  float a(float k = 1.0f) const { return cfg.opacity * k; }
};

inline void line(Ctx &c, Vec2 a, Vec2 b, std::uint32_t rgb, float width, float alphaK = 1.0f) {
  Primitive p;
  p.type = PrimType::Line;
  p.x1 = a.x; p.y1 = a.y; p.x2 = b.x; p.y2 = b.y;
  p.size = width;
  p.color = argb(rgb, c.a(alphaK));
  c.out.push_back(p);
}

// Draws a world-space segment. Both ends must be in front of the camera.
inline bool worldLine(Ctx &c, Vec3 a, Vec3 b, std::uint32_t rgb, float width) {
  const auto pa = c.snap.camera.project(a);
  const auto pb = c.snap.camera.project(b);
  if (!pa || !pb) return false;
  if (c.quality()) line(c, *pa, *pb, rgb, width * 3.0f, 0.25f);  // soft glow underlay
  line(c, *pa, *pb, rgb, width);
  return true;
}

inline void text(Ctx &c, const std::string &s, float x, float y, float size, std::uint32_t rgb,
                 int align = 1, float alphaK = 1.0f) {
  Primitive p;
  p.type = PrimType::Text;
  p.x1 = x; p.y1 = y; p.size = size; p.align = align; p.text = s;
  p.color = argb(rgb, c.a(alphaK));
  c.out.push_back(p);
}

inline void rectFilled(Ctx &c, float x, float y, float w, float h, std::uint32_t rgb, float alphaK) {
  Primitive p;
  p.type = PrimType::RectFilled;
  p.x1 = x; p.y1 = y; p.w = w; p.h = h;
  p.color = argb(rgb, c.a(alphaK));
  c.out.push_back(p);
}

inline void rectOutline(Ctx &c, float x, float y, float w, float h, std::uint32_t rgb, float width) {
  Primitive p;
  p.type = PrimType::Rect;
  p.x1 = x; p.y1 = y; p.w = w; p.h = h; p.size = width;
  p.color = argb(rgb, c.a());
  c.out.push_back(p);
}

inline void circleFilled(Ctx &c, float x, float y, float r, std::uint32_t rgb, float alphaK) {
  Primitive p;
  p.type = PrimType::CircleFilled;
  p.x1 = x; p.y1 = y; p.size = r;
  p.color = argb(rgb, c.a(alphaK));
  c.out.push_back(p);
}

inline void triangle(Ctx &c, Vec2 a, Vec2 b, Vec2 d, std::uint32_t rgb, float alphaK = 1.0f) {
  Primitive p;
  p.type = PrimType::TriangleFilled;
  p.x1 = a.x; p.y1 = a.y; p.x2 = b.x; p.y2 = b.y; p.x3 = d.x; p.y3 = d.y;
  p.color = argb(rgb, c.a(alphaK));
  c.out.push_back(p);
}

inline float distTo(const Snapshot &s, Vec3 p) { return length(p - s.camera.pos); }

// Distance from the camera to the closest point of an axis-aligned box.
inline float distToBox(const Snapshot &s, Vec3 mn, Vec3 mx) {
  const Vec3 p = s.camera.pos;
  const Vec3 q{std::min(std::max(p.x, mn.x), mx.x), std::min(std::max(p.y, mn.y), mx.y),
               std::min(std::max(p.z, mn.z), mx.z)};
  return length(p - q);
}

// ---------------------------------------------------------------- villagers

inline void drawArc(Ctx &c, Vec2 center, float radius, float fraction, std::uint32_t rgb) {
  const int segs = 24;
  const int n = static_cast<int>(std::lround(clampF(fraction, 0.0f, 1.0f) * segs));
  const float step = 2.0f * 3.14159265f / segs;
  for (int i = 0; i < n; ++i) {
    const float a0 = -1.5707963f + step * i, a1 = -1.5707963f + step * (i + 1);
    line(c, {center.x + radius * std::cos(a0), center.y + radius * std::sin(a0)},
         {center.x + radius * std::cos(a1), center.y + radius * std::sin(a1)}, rgb,
         2.5f * c.cfg.scale);
  }
}

inline std::string secondsText(float s) {
  char buf[16];
  if (s >= 60.0f) std::snprintf(buf, sizeof buf, "%dm%02ds", int(s) / 60, int(s) % 60);
  else std::snprintf(buf, sizeof buf, "%ds", int(s));
  return buf;
}

inline void drawZombie(Ctx &c, const VillagerInfo &v) {
  // Project the 8 corners of the zombie villager's box; skip if any corner is behind the camera.
  float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
  for (int i = 0; i < 8; ++i) {
    const Vec3 corner{v.pos.x + ((i & 1) ? 0.3f : -0.3f), v.pos.y + ((i & 2) ? 1.95f : 0.0f),
                      v.pos.z + ((i & 4) ? 0.3f : -0.3f)};
    const auto p = c.snap.camera.project(corner);
    if (!p) return;
    minX = std::min(minX, p->x); maxX = std::max(maxX, p->x);
    minY = std::min(minY, p->y); maxY = std::max(maxY, p->y);
  }
  const std::uint32_t kCure = 0x0040F0C0;
  if (c.quality()) rectFilled(c, minX, minY, maxX - minX, maxY - minY, kCure, 0.12f);
  rectOutline(c, minX, minY, maxX - minX, maxY - minY, kCure, 2.0f * c.cfg.scale);
  text(c, "Zombie Villager (curable)", (minX + maxX) * 0.5f, minY - 6.0f, 13.0f * c.cfg.scale,
       kCure);
}

inline void drawBadge(Ctx &c, const VillagerInfo &v, float depth) {
  const auto head = c.snap.camera.project(v.pos + Vec3{0, 2.55f, 0});
  if (!head) return;
  float r = 13.0f * c.cfg.scale;
  if (c.quality()) r *= clampF(14.0f / std::max(depth, 1.0f), 0.6f, 1.6f);
  const std::uint32_t col = professionColor(v.profession);

  if (c.quality()) circleFilled(c, head->x, head->y, r * 1.55f, col, 0.25f);  // glow
  if (c.quality()) circleFilled(c, head->x, head->y, r, col, 0.95f);
  else rectFilled(c, head->x - r, head->y - r, r * 2, r * 2, col, 0.95f);
  text(c, professionLabel(v.profession), head->x, head->y + r * 0.35f, r * 1.05f, 0x00101010);

  // Trade tier pips (1..5) under the badge
  const int tier = static_cast<int>(v.tier);
  const float pip = std::max(2.5f, r * 0.22f), gap = pip * 0.8f;
  const float rowW = 5 * pip * 2 + 4 * gap;
  for (int i = 0; i < 5; ++i) {
    const float px = head->x - rowW * 0.5f + i * (pip * 2 + gap);
    rectFilled(c, px, head->y + r + 3.0f, pip * 2, pip * 2, i < tier ? 0x00FFE070 : 0x00606060,
               i < tier ? 1.0f : 0.5f);
  }

  if (c.cfg.restockTimer && v.restockProgress >= 0.0f) {
    const std::uint32_t ring = v.restockProgress >= 0.999f ? 0x0060F060 : 0x0060C0FF;
    if (c.quality()) {
      drawArc(c, {head->x, head->y}, r * 1.35f, v.restockProgress, ring);
    } else {
      const float bw = r * 2.4f;  // flat progress bar for low-end devices
      rectFilled(c, head->x - bw * 0.5f, head->y - r - 9.0f, bw, 4.0f, 0x00303030, 0.7f);
      rectFilled(c, head->x - bw * 0.5f, head->y - r - 9.0f, bw * clampF(v.restockProgress, 0, 1),
                 4.0f, ring, 1.0f);
    }
    const std::string label =
        v.restockProgress >= 0.999f ? "Ready" : (v.restockSeconds >= 0 ? secondsText(v.restockSeconds) : "");
    if (!label.empty()) text(c, label, head->x, head->y - r - 14.0f, 11.0f * c.cfg.scale, ring);
  }
}

inline void drawWorkstationPointer(Ctx &c, const VillagerInfo &v) {
  const WorkstationInfo *best = nullptr;
  float bestD = 48.0f;  // only point at stations within 48 blocks
  for (const auto &w : c.snap.workstations) {
    if (w.claimed) continue;
    const float d = length(w.pos - v.pos);
    if (d < bestD) { bestD = d; best = &w; }
  }
  if (!best) return;
  const Vec3 from = v.pos + Vec3{0, 1.2f, 0};
  Vec3 dir = best->pos - from;
  const float len = length(dir);
  if (len < 0.01f) return;
  const Vec3 to = from + dir * (std::min(len, 3.0f) / len);
  const std::uint32_t col = 0x00FFC040;
  if (!worldLine(c, from, to, col, 2.0f * c.cfg.scale)) return;
  if (const auto tip = c.snap.camera.project(to)) {
    const float s = 6.0f * c.cfg.scale;
    triangle(c, {tip->x, tip->y - s}, {tip->x - s, tip->y + s}, {tip->x + s, tip->y + s}, col);
  }
}

// --------------------------------------------------------------- structures

inline const char *structureName(StructureKind k) {
  switch (k) {
    case StructureKind::Village: return "Village";
    case StructureKind::NetherFortress: return "Nether Fortress";
    case StructureKind::PillagerOutpost: return "Pillager Outpost";
    case StructureKind::OceanMonument: return "Ocean Monument";
    case StructureKind::Stronghold: return "Stronghold";
  }
  return "?";
}

inline void drawVillageRing(Ctx &c, const StructureInfo &s) {
  const int segs = c.quality() ? 64 : 28;
  const std::uint32_t col = 0x0070E070;
  Vec3 prev{s.center.x + s.radius, s.center.y, s.center.z};
  for (int i = 1; i <= segs; ++i) {
    const float a = 2.0f * 3.14159265f * i / segs;
    const Vec3 cur{s.center.x + s.radius * std::cos(a), s.center.y, s.center.z + s.radius * std::sin(a)};
    const Vec3 mid = (prev + cur) * 0.5f;
    if (distTo(c.snap, mid) <= static_cast<float>(c.cfg.renderDistance))
      worldLine(c, prev, cur, col, 2.0f * c.cfg.scale);
    prev = cur;
  }
  if (const auto p = c.snap.camera.project(s.center + Vec3{0, 1.0f, 0}))
    if (distTo(c.snap, s.center) <= static_cast<float>(c.cfg.renderDistance))
      text(c, "Village center", p->x, p->y, 13.0f * c.cfg.scale, col);
}

inline void drawBox(Ctx &c, const StructureInfo &s) {
  if (distToBox(c.snap, s.boundsMin, s.boundsMax) > static_cast<float>(c.cfg.renderDistance)) return;
  const std::uint32_t col = s.kind == StructureKind::NetherFortress ? 0x00FF6A4A : 0x00C070FF;
  Vec3 k[8];
  for (int i = 0; i < 8; ++i)
    k[i] = {(i & 1) ? s.boundsMax.x : s.boundsMin.x, (i & 2) ? s.boundsMax.y : s.boundsMin.y,
            (i & 4) ? s.boundsMax.z : s.boundsMin.z};
  static const int edges[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3},
                                   {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
  for (const auto &e : edges) worldLine(c, k[e[0]], k[e[1]], col, 2.0f * c.cfg.scale);
  if (const auto p = c.snap.camera.project((s.boundsMin + s.boundsMax) * 0.5f))
    text(c, structureName(s.kind), p->x, p->y, 13.0f * c.cfg.scale, col);
}

// Marker for far targets. On-screen: pinned over the target. Off-screen: arrow on the screen edge.
inline void drawVector(Ctx &c, const StructureInfo &s, int slot) {
  const float dist = distTo(c.snap, s.center);
  const float scanRadius = std::min(512.0f, static_cast<float>(c.cfg.renderDistance) * 4.0f);
  if (dist > scanRadius) return;
  const std::uint32_t col = s.kind == StructureKind::OceanMonument ? 0x0040C8FF : 0x00FFD84A;
  char buf[48];
  std::snprintf(buf, sizeof buf, "%s %dm", structureName(s.kind), static_cast<int>(dist));

  const float W = c.snap.camera.screenW, H = c.snap.camera.screenH;
  const auto p = c.snap.camera.project(s.center);
  const float sz = 9.0f * c.cfg.scale;
  if (p && p->x > 0 && p->x < W && p->y > 0 && p->y < H) {
    triangle(c, {p->x, p->y + sz}, {p->x - sz, p->y - sz}, {p->x + sz, p->y - sz}, col);
    text(c, buf, p->x, p->y - sz - 4.0f, 12.0f * c.cfg.scale, col);
    return;
  }
  const bool right = c.snap.camera.bearingTo(s.center) > 0;
  const float ex = right ? W - 24.0f : 24.0f;
  const float ey = H * 0.25f + slot * 34.0f * c.cfg.scale;
  const float dx = right ? sz : -sz;
  triangle(c, {ex + dx, ey}, {ex - dx, ey - sz}, {ex - dx, ey + sz}, col);
  text(c, buf, right ? ex - sz - 4.0f : ex + sz + 4.0f, ey + 4.0f, 12.0f * c.cfg.scale, col,
       0);
}

}  // namespace detail

inline std::vector<Primitive> buildOverlay(const Snapshot &snap, const RadarConfig &cfg) {
  std::vector<Primitive> out;
  if (!cfg.enabled || !snap.valid) return out;
  detail::Ctx c{snap, cfg, out};

  for (const auto &v : snap.villagers) {
    float depth = 0;
    const float d = detail::distTo(snap, v.pos);
    if (d > static_cast<float>(cfg.renderDistance)) continue;
    if (!snap.camera.project(v.pos + Vec3{0, 1.0f, 0}, &depth)) continue;

    if (v.isZombie) {
      if (cfg.zombieHighlight) detail::drawZombie(c, v);
      continue;
    }
    if (cfg.professionBadges) detail::drawBadge(c, v, depth);
    if (cfg.workstationPointer && v.profession == Profession::None)
      detail::drawWorkstationPointer(c, v);
  }

  int slot = 0;
  for (const auto &s : snap.structures) {
    switch (s.kind) {
      case StructureKind::Village:
        if (cfg.villageBoundary) detail::drawVillageRing(c, s);
        break;
      case StructureKind::NetherFortress:
      case StructureKind::PillagerOutpost:
        if (cfg.boundingBoxes) detail::drawBox(c, s);
        break;
      case StructureKind::OceanMonument:
      case StructureKind::Stronghold:
        if (cfg.distanceVectors) detail::drawVector(c, s, slot++);
        break;
    }
  }
  return out;
}

}  // namespace radar
