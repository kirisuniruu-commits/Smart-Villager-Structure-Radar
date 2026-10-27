#pragma once
// Smart Villager & Structure Radar - created by ChrisNiruu
//
// Probe mode: captures the first bytes of a few game structures (copied inside the game's own
// render hooks, where the pointers are valid) and shows them as hex on the HUD, so struct offsets
// can be worked out from a screenshot - no PC needed.

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "radar/Renderer.hpp"

namespace radar::probe {

constexpr int kPages = 6;
constexpr std::size_t kBytes = 256;

inline const char *pageName(int i) {
  static const char *names[kPages] = {
      "0 ActorRenderData (DataDrivenRenderer::render arg2)",
      "1 BaseActorRenderContext (LevelRendererCamera::render arg1)",
      "2 IClientInstance (LevelRendererCamera::render arg3)",
      "3 ScreenContext (InGamePlayScreen::_renderLevelPrep arg1)",
      "4 Actor / local player (InGamePlayScreen::_renderLevelPrep arg3)",
      "5 ViewRenderObject (LevelRendererCamera::render arg2)"};
  return (i >= 0 && i < kPages) ? names[i] : "?";
}

struct Page {
  std::atomic<std::uint32_t> seq{0};  // odd while being written
  std::uintptr_t ptr = 0;
  std::size_t len = 0;
  std::uint8_t data[kBytes] = {};
};

inline Page g_pages[kPages];
inline std::atomic<bool> g_on{false};

// Copies at most up to the end of the pointer's own page, so a short object can never fault.
inline void capture(int idx, const void *p) {
  if (!g_on.load(std::memory_order_relaxed) || !p || idx < 0 || idx >= kPages) return;
  const auto a = reinterpret_cast<std::uintptr_t>(p);
  if (a < 0x10000) return;
  const std::size_t n = std::min<std::size_t>(kBytes, 4096 - (a & 4095));
  Page &pg = g_pages[idx];
  pg.seq.fetch_add(1);
  std::memcpy(pg.data, p, n);
  pg.len = n;
  pg.ptr = a;
  pg.seq.fetch_add(1);
}

// Builds HUD text for one page (reads a consistent copy).
inline std::vector<radar::Primitive> draw(int page, float screenW) {
  using radar::PrimType;
  using radar::Primitive;
  std::vector<Primitive> out;
  if (page < 0 || page >= kPages) return out;
  Page &pg = g_pages[page];

  std::uint8_t buf[kBytes];
  std::size_t len = 0;
  std::uintptr_t ptr = 0;
  std::uint32_t s0 = 0, s1 = 1;
  for (int tries = 0; tries < 4 && (s0 != s1 || (s0 & 1)); ++tries) {
    s0 = pg.seq.load();
    len = pg.len;
    ptr = pg.ptr;
    std::memcpy(buf, pg.data, kBytes);
    s1 = pg.seq.load();
  }

  auto text = [&](const std::string &s, float y, std::uint32_t color) {
    Primitive p;
    p.type = PrimType::Text;
    p.x1 = 12.0f; p.y1 = y; p.size = 12.0f; p.align = 0; p.color = color; p.text = s;
    out.push_back(p);
  };

  Primitive bg;
  bg.type = PrimType::RectFilled;
  bg.x1 = 4; bg.y1 = 4; bg.w = std::min(screenW - 8.0f, 560.0f); bg.h = 36.0f + 14.0f * 17;
  bg.color = 0xB0000000;
  out.push_back(bg);

  char head[160];
  std::snprintf(head, sizeof head, "PROBE %s", pageName(page));
  text(head, 20, 0xFFFFE070);
  if (len == 0) {
    text("no data yet (open a world; for page 0 look at a mob)", 36, 0xFFFFFFFF);
    return out;
  }
  std::snprintf(head, sizeof head, "ptr=%llx len=%zu", static_cast<unsigned long long>(ptr), len);
  text(head, 34, 0xFFA0E0FF);

  for (std::size_t off = 0; off < len && off < kBytes; off += 16) {
    char line[128];
    int n = std::snprintf(line, sizeof line, "%02zx:", off);
    for (std::size_t i = 0; i < 16 && off + i < len; ++i) {
      if (i % 4 == 0) n += std::snprintf(line + n, sizeof line - n, " ");
      n += std::snprintf(line + n, sizeof line - n, "%02x", buf[off + i]);
    }
    text(line, 50.0f + 14.0f * (off / 16), 0xFFFFFFFF);
  }
  return out;
}

}  // namespace radar::probe
