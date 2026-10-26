#pragma once
// Smart Villager & Structure Radar - created by ChrisNiruu
// World -> screen projection. Minecraft convention: yaw 0 faces +Z, positive yaw turns
// clockwise seen from above (towards -X), positive pitch looks down.

#include <cmath>
#include <optional>

namespace radar {

struct Vec3 {
  float x = 0, y = 0, z = 0;
};
struct Vec2 {
  float x = 0, y = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

constexpr float kDegToRad = 3.14159265358979f / 180.0f;

struct Camera {
  Vec3 pos;  // eye position
  float yawDeg = 0;
  float pitchDeg = 0;
  float fovYDeg = 70;  // vertical field of view
  float screenW = 1920;
  float screenH = 1080;

  Vec3 forward() const {
    const float y = yawDeg * kDegToRad, p = pitchDeg * kDegToRad;
    return {-std::sin(y) * std::cos(p), -std::sin(p), std::cos(y) * std::cos(p)};
  }
  Vec3 right() const {
    const float y = yawDeg * kDegToRad;
    return {-std::cos(y), 0.0f, -std::sin(y)};
  }
  Vec3 up() const { return cross(right(), forward()); }

  // Returns nullopt when the point is behind (or almost on) the camera plane.
  std::optional<Vec2> project(Vec3 world, float *depthOut = nullptr) const {
    const Vec3 d = world - pos;
    const float depth = dot(d, forward());
    if (depthOut) *depthOut = depth;
    if (depth < 0.05f) return std::nullopt;
    const float focal = (screenH * 0.5f) / std::tan(fovYDeg * 0.5f * kDegToRad);
    return Vec2{screenW * 0.5f + focal * dot(d, right()) / depth,
                screenH * 0.5f - focal * dot(d, up()) / depth};
  }

  // Signed horizontal angle (deg) from the view direction to a target; positive = to the right.
  float bearingTo(Vec3 world) const {
    const Vec3 d = world - pos;
    const float targetYaw = std::atan2(-d.x, d.z) / kDegToRad;
    float diff = targetYaw - yawDeg;
    while (diff > 180) diff -= 360;
    while (diff < -180) diff += 360;
    return diff;
  }
};

}  // namespace radar
