// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#include <mc1/fast_math.h>
#include <mc1/framebuffer.h>

#include <mr32intrin.h>

#include <cstdint>
#include <cstring>

namespace {

//--------------------------------------------------------------------------------------------------
// Floating point helper for creating constants that can be represented as a 21-bit immediate.
//--------------------------------------------------------------------------------------------------

/// @brief Type punning cast.
/// @param x The source value of type @c T1.
/// @returns a value of type @c T2 that has the same in-memory representation as @c x.
template <typename T2, typename T1>
T2 bit_cast(const T1 x) {
  static_assert(sizeof(T1) == sizeof(T2));
  T2 y;
  std::memcpy(&y, &x, sizeof(T2));
  return y;
}

/// @brief Create a float constant that can be loaded with a single ldhi/ldhio instruction.
/// @param x The value to approximate.
/// @returns a constant value that approximates @c x to 21 bits precision (13 effective significand
/// bits).
template <typename T>
constexpr float flt21(const T x) {
  const auto xi = bit_cast<uint32_t>(static_cast<float>(x));
  if ((xi & 0x00000400u) == 0u) {
    // Cater for ldhi.
    return bit_cast<float>(xi & 0xfffff800u);
  } else {
    // Cater for ldhio.
    return bit_cast<float>(xi | 0x000007ffu);
  }
}

//--------------------------------------------------------------------------------------------------
// A simple vector library.
//--------------------------------------------------------------------------------------------------

struct vec3_t {
  float x;
  float y;
  float z;
};

vec3_t vec3(const float x, const float y, const float z) {
  vec3_t v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

vec3_t operator+(const vec3_t a, const vec3_t b) {
  return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

vec3_t operator-(const vec3_t a, const vec3_t b) {
  return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

vec3_t operator*(const vec3_t a, const float s) {
  return vec3(a.x * s, a.y * s, a.z * s);
}

float dot(const vec3_t a, const vec3_t b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3_t cross(const vec3_t a, const vec3_t b) {
  return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

vec3_t normalize(const vec3_t a) {
  return a * fast_rsqrt(dot(a, a));
}

//--------------------------------------------------------------------------------------------------
// Different video modes to try (we try lower and lower until the framebuffer fits in memory).
//--------------------------------------------------------------------------------------------------

struct vmode_t {
  int16_t width;
  int16_t height;
  int16_t mode;
};

const vmode_t VMODES[] = {
    {470, 264, CMODE_RGBA8888},
    {320, 180, CMODE_RGBA8888},
    {304, 171, CMODE_RGBA5551},
    {180, 101, CMODE_RGBA5551},
};

//--------------------------------------------------------------------------------------------------
// Scene definition.
//--------------------------------------------------------------------------------------------------

struct sphere_t {
  vec3_t center;
  float r2;
  float r_inv;
  float r2_inv;
};

const int NUM_SPHERES = 4;

const sphere_t s_spheres[NUM_SPHERES] = {{{-1.5f, 0.0f, 1.0f}, 1.0f, 1.0f, 1.0f},
                                         {{1.5f, 0.0f, 1.0f}, 1.0f, 1.0f, 1.0f},
                                         {{0.0f, -1.5f, 0.5f}, 0.25f, 2.0f, 4.0f},
                                         {{0.0f, 1.5f, 0.5f}, 0.25f, 2.0f, 4.0f}};

const vec3_t s_colors[NUM_SPHERES] = {{0.25f, 0.4f, 0.25f},
                                      {0.4f, 0.4f, 0.25f},
                                      {0.25f, 0.4f, 0.4f},
                                      {0.25f, 0.25f, 0.4f}};

//--------------------------------------------------------------------------------------------------
// Raytracing routines.
//--------------------------------------------------------------------------------------------------

fb_t* s_fb;

struct camera_t {
  vec3_t forward;
  vec3_t right;
  vec3_t up;
};

camera_t look_at(const vec3_t origin, const vec3_t target) {
  camera_t cam;
  cam.forward = normalize(target - origin);
  cam.right = normalize(cross(cam.forward, vec3(0.0f, 0.0f, 1.0f)));
  cam.up = cross(cam.right, cam.forward);
  return cam;
}

struct ray_t {
  vec3_t origin;
  vec3_t dir;
};

ray_t make_ray(const vec3_t origin, const vec3_t dir) {
  ray_t ray;
  ray.origin = origin;
  ray.dir = dir;
  return ray;
}

vec3_t reflect(const vec3_t v, const vec3_t n) {
  // Note: The normal, n, needs to be unit length.
  return v - (n * (2.0f * dot(v, n)));
}

float intersect_sphere(const ray_t ray, const sphere_t sphere) {
  // Translate the ray origin to compensate for the sphere origin.
  const auto origin = ray.origin - sphere.center;

  const auto b = dot(ray.dir, origin);
  const auto c = dot(origin, origin) - sphere.r2;
  const auto discriminant = b * b - c;

  // No hit?
  if (discriminant <= 0.0f) {
    return flt21(-1e10);
  }

  // We're only interested in the hit closest to minus infinity.
  return -b - fast_sqrt(discriminant);
}

float intersect_ground(const ray_t ray) {
  // Note: ray.dir.z must be negative.
  return -ray.origin.z / ray.dir.z;
}

vec3_t trace_ray(const ray_t ray, int recursion_left) {
  vec3_t col;
  vec3_t pos;
  vec3_t normal;
  float t = flt21(-1e10);

  // First, try all spheres.
  int sphere_idx = -1;
  for (int i = 0; i < NUM_SPHERES; ++i) {
    const auto& sphere = s_spheres[i];
    const auto t1 = intersect_sphere(ray, sphere);
    if (t1 > 0.0f && (t1 < t || t < 0.0f)) {
      sphere_idx = i;
      t = t1;
    }
  }

  // If we hit a sphere, calculate surface properties for the sphere.
  // Note: If we've hit a sphere, we'll never hit the ground (since all rays are above the ground).
  if (sphere_idx >= 0) {
    pos = ray.origin + ray.dir * t;
    normal = (pos - s_spheres[sphere_idx].center) * s_spheres[sphere_idx].r_inv;

    // Modulate the color based on the value of the normal z-axis (fake GI / AO).
    const auto light = 0.5f * (1.0f + normal.z);
    col = s_colors[sphere_idx] * light;
  } else if (ray.dir.z < 0.0f) {
    // Second, go for the ground (we have a hit if the ray is going down).
    t = intersect_ground(ray);
    pos = vec3(ray.origin.x + ray.dir.x * t, ray.origin.y + ray.dir.y * t, 0.0f);
    normal = vec3(0.0f, 0.0f, 1.0f);

    // Modulate light with distance to the scene center, and with the distance to spheres (fake AO).
    auto light = 2.0f * fast_rsqrt(4.0f + pos.x * pos.x + pos.y * pos.y);
    for (int i = 0; i < NUM_SPHERES; ++i) {
      const auto dv = vec3(pos.x - s_spheres[i].center.x, pos.y - s_spheres[i].center.y, 0.0f);
      const auto d2 = dot(dv, dv);
      if (d2 < s_spheres[i].r2) {
        light *= d2 * s_spheres[i].r2_inv;
      }
    }

    // Apply a checkerboard pattern.
    const auto checker_idx =
        (static_cast<int>(pos.x + 131072.0f) ^ static_cast<int>(pos.y + 131072.0f)) & 2;
    const auto checker_dcol = light * (flt21(0.6) + flt21(0.2) * static_cast<float>(checker_idx));
    col = vec3(checker_dcol, flt21(0.2) * checker_dcol, flt21(0.2) * checker_dcol);
  } else {
    // No hit! The ray is looking at the sky (and we know that ray.dir.z > 0).
    const auto s = flt21(0.4);
    const auto fade = s * ray.dir.z;
    return vec3(s - fade, s - fade, 1.0f - fade);
  }

  // Since we hit a surface (sphere or ground), do reflection.
  if (recursion_left > 0) {
    const auto ray2 = make_ray(pos, reflect(ray.dir, normal));
    const auto col2 = trace_ray(ray2, recursion_left - 1);
    col = col + col2 * flt21(0.3);
  }

  return col;
}

ray_t make_camera_ray(const vec3_t origin, const camera_t cam, const float dx, const float dy) {
  ray_t ray;
  ray.origin = origin;
  ray.dir = normalize(cam.forward + cam.right * dx + cam.up * dy);
  return ray;
}

uint32_t clamp5(const uint32_t x) {
  // Clamp to an unsigned 5-bit value. This requires a single MINU instruction on MRISC32.
  return x <= 31u ? x : 31u;
}

void render_image(const float t) {
  // Set up the camera.
  const auto origin =
      vec3(4.0f * fast_sin(t), 4.0f * fast_cos(t), 1.0f + 0.5f * fast_cos(flt21(0.37) * t));
  const auto target = vec3(0.0f, 0.0f, 0.5f);
  const auto cam = look_at(origin, target);

  // Iterate over all the pixels.
  auto* pixels = reinterpret_cast<uint8_t*>(s_fb->pixels);
  const auto pix_scale = 1.0f / static_cast<float>(s_fb->width);
  for (int sy = 0; sy < s_fb->height; ++sy) {
    const auto dy = static_cast<float>(s_fb->height - 2 * sy) * pix_scale;
    for (int sx = 0; sx < s_fb->width; ++sx) {
      const auto dx = static_cast<float>(2 * sx - s_fb->width) * pix_scale;

      // Calculate the ray for this pixel.
      const auto ray = make_camera_ray(origin, cam, dx, dy);

      // Trace!
      const auto col = trace_ray(ray, 2);

      // Write the pixel to the framebuffer memory.
      if (s_fb->mode == CMODE_RGBA8888) {
        const uint32_t r = _mr32_ftour(col.x, 8);
        const uint32_t g = _mr32_ftour(col.y, 8);
        const uint32_t b = _mr32_ftour(col.z, 8);
        const uint32_t pix = _mr32_packsu_h(_mr32_packsu(255, g), _mr32_packsu(b, r));
        *reinterpret_cast<uint32_t*>(pixels) = pix;
        pixels += 4;
      } else if (s_fb->mode == CMODE_RGBA5551) {
        const uint32_t r = clamp5(_mr32_ftour(col.x, 5));
        const uint32_t g = clamp5(_mr32_ftour(col.y, 5));
        const uint32_t b = clamp5(_mr32_ftour(col.z, 5));
        const uint32_t pix = r | (g << 5) | (b << 10) | 0x8000u;
        *reinterpret_cast<uint16_t*>(pixels) = static_cast<uint16_t>(pix);
        pixels += 2;
      }
    }
  }
}

}  // namespace

//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

extern "C" void raytrace_init(void) {
  const int NUM_VMODES = static_cast<int>(sizeof(VMODES) / sizeof(VMODES[0]));
  for (int i = 0; i < NUM_VMODES && s_fb == nullptr; ++i) {
    const auto& vm = VMODES[i];
    s_fb = fb_create(vm.width, vm.height, vm.mode);
  }
}

extern "C" void raytrace_deinit(void) {
  if (s_fb != nullptr) {
    fb_destroy(s_fb);
    s_fb = nullptr;
  }
}

extern "C" void raytrace(int frame_no) {
  if (s_fb == nullptr) {
    return;
  }

  fb_show(s_fb, LAYER_1);

  const float t = flt21(0.1) * static_cast<float>(frame_no);
  render_image(t);
}
