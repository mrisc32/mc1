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

#include <stdint.h>

#define DEFAULT_WIDTH  304
#define DEFAULT_HEIGHT 171


//--------------------------------------------------------------------------------------------------
// A simple vector library.
//--------------------------------------------------------------------------------------------------

typedef struct {
  float x;
  float y;
  float z;
} vec3_t;

static vec3_t vec3(const float x, const float y, const float z) {
  vec3_t c;
  c.x = x;
  c.y = y;
  c.z = z;
  return c;
}

static vec3_t add(const vec3_t a, const vec3_t b) {
  return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static vec3_t sub(const vec3_t a, const vec3_t b) {
  return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static vec3_t scale(const vec3_t a, const float s) {
  return vec3(a.x * s, a.y * s, a.z * s);
}

static float dot(const vec3_t a, const vec3_t b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static vec3_t cross(const vec3_t a, const vec3_t b) {
  return vec3(a.y * b.z - a.z * b.y,
              a.z * b.x - a.x * b.z,
              a.x * b.y - a.y * b.x);
}

static vec3_t normalize(const vec3_t a) {
  return scale(a, fast_rsqrt(dot(a, a)));
}


//--------------------------------------------------------------------------------------------------
// Scene definition.
//--------------------------------------------------------------------------------------------------

typedef struct {
  vec3_t center;
  float  r2;
} sphere_t;

#define NUM_SPHERES 4
static const sphere_t s_spheres[NUM_SPHERES] = {
  {{-1.5f, 0.0f, 1.0f}, 1.0f},
  {{1.5f, 0.0f, 1.0f}, 1.0f},
  {{0.0f, -1.5f, 0.5f}, 0.25f},
  {{0.0f, 1.5f, 0.5f}, 0.25f}
};

static const vec3_t s_colors[NUM_SPHERES] = {
  {0.25f, 0.4f, 0.25f},
  {0.4f, 0.4f, 0.25f},
  {0.25f, 0.4f, 0.4f},
  {0.25f, 0.25f, 0.4f}
};


//--------------------------------------------------------------------------------------------------
// Raytracing routines.
//--------------------------------------------------------------------------------------------------

typedef struct {
  vec3_t forward;
  vec3_t right;
  vec3_t up;
} camera_t;

typedef struct {
  vec3_t origin;
  vec3_t dir;
} ray_t;

static camera_t look_at(const vec3_t origin, const vec3_t target) {
  camera_t cam;
  cam.forward = normalize(sub(target, origin));
  cam.right = normalize(cross(cam.forward, vec3(0.0f, 0.0f, 1.0f)));
  cam.up = cross(cam.right, cam.forward);
  return cam;
}

static vec3_t reflect(const vec3_t v, const vec3_t n) {
  return sub(v, scale(n, 2.0f * dot(v, n) / dot(n, n)));
}

static float intersect_sphere(const ray_t ray, const sphere_t* sphere) {
  // Translate the ray origin to compensate for the sphere origin.
  const vec3_t origin = sub(ray.origin, sphere->center);

  const float b = dot(ray.dir, origin);
  const float c = dot(origin, origin) - sphere->r2;
  const float discriminant = b * b - c;

  // No hit?
  if (discriminant <= 0.0f) {
    return -1e10f;
  }

  // We're only interested in the hit closest to minus infinity.
  return -b - fast_sqrt(discriminant);
}

static float intersect_ground(const ray_t ray) {
  if (ray.dir.z >= 0.0f) {
    return -1e10f;
  }

  return -ray.origin.z / ray.dir.z;
}

static vec3_t trace_ray(const ray_t ray, int recursion_left) {
  vec3_t col;
  vec3_t pos;
  vec3_t normal;

  float t = -1.0f;

  // First, try all spheres.
  int sphere_idx = -1;
  for (int i = 0; i < NUM_SPHERES; ++i) {
    const sphere_t* sphere = &s_spheres[i];
    float t1 = intersect_sphere(ray, sphere);
    if (t1 > 0.0f && (t1 < t || t < 0.0f)) {
      sphere_idx = i;
      t = t1;
    }
  }

  // Second, try the ground.
  float t_ground = intersect_ground(ray);
  if (t_ground > 0.0f && (t_ground < t || t < 0.0f)) {
    sphere_idx = -1;
    t = t_ground;
    pos = add(ray.origin, scale(ray.dir, t));
    normal = vec3(0.0f, 0.0f, 1.0f);

    // Modulate light with distance to the scene center, and with the distance to spheres (fake AO).
    float light = 2.0f * fast_rsqrt(4.0f + pos.x * pos.x + pos.y * pos.y);
    for (int i = 0; i < NUM_SPHERES; ++i) {
      const vec3_t dv = sub(pos, s_spheres[i].center);
      const float d2 = dot(dv, dv) - s_spheres[i].r2;
      const float d = fast_sqrt(d2);
      if (d < 1.0f) {
        light *= d;
      }
    }

    // Calculate final color of the ground.
    const int checker_idx = (((int)(pos.x + 131072.0f) ^ (int)(pos.y + 131072.0f)) >> 1) & 1;
    const float checker_dcol = light * (0.6f + 0.4f * (float)checker_idx);
    col = vec3(1.0f * checker_dcol, 0.2f * checker_dcol, 0.2f * checker_dcol);
  }

  // If we hit a sphere, but not the ground, calculate surface properties for the sphere.
  if (sphere_idx >= 0) {
    pos = add(ray.origin, scale(ray.dir, t));
    normal = normalize(sub(pos, s_spheres[sphere_idx].center));

    // Modulate the color based on the value of the normal z-axis (fake GI / AO).
    const float light = 0.5f * (1.0f + normal.z);
    col = scale(s_colors[sphere_idx], light);
  }

  if (t < 0.0f) {
    // No hit! The ray is looking at the sky (and we know that ray.dir.z > 0).
    const float fade = 0.4f * ray.dir.z;
    col = vec3(0.4f - fade, 0.4f - fade, 1.0f - fade);
  } else {
    // Offset new ray origin from surface to avoid z-fighting.
    pos = add(pos, scale(normal, 0.00012207031251f));

    // Reflection.
    if (recursion_left > 0) {
      ray_t ray2;
      ray2.origin = pos;
      ray2.dir = reflect(ray.dir, normal);
      const vec3_t col2 = trace_ray(ray2, recursion_left - 1);
      col = add(col, scale(col2, 0.3f));
    }
  }

  return col;
}

static float clamp(const float x) {
  // For color clamping: We clamp to the range [0.0, 1.0).
  return x < 0.0f ? 0.0f : (x > 0.999999940395f ? 0.999999940395f : x);
}

static vec3_t clamp_color(const vec3_t col) {
  return vec3(clamp(col.x), clamp(col.y), clamp(col.z));
}

static void render_image(fb_t* fb, float t) {
  // Set up the camera.
  vec3_t origin = vec3(4.0f * fast_sin(t), 4.0f * fast_cos(t), 1.0f + 0.5f * fast_cos(0.37f * t));
  vec3_t target = vec3(0.0f, 0.0f, 0.5f);
  camera_t cam = look_at(origin, target);

  // Iterate over all the pixels.
  uint8_t* pixels = (uint8_t*)fb->pixels;
  const float pix_scale = 1.0f / (float)fb->width;
  for (int sy = 0; sy < fb->height; ++sy) {
    float dy = (float)(fb->height - 2 * sy) * pix_scale;
    for (int sx = 0; sx < fb->width; ++sx) {
      float dx = (float)(2 * sx - fb->width) * pix_scale;

      // Calculate the ray for this pixel.
      ray_t ray;
      ray.origin = origin;
      ray.dir = cam.forward;
      ray.dir = add(ray.dir, scale(cam.right, dx));
      ray.dir = add(ray.dir, scale(cam.up, dy));
      ray.dir = normalize(ray.dir);

      // Trace!
      vec3_t col = trace_ray(ray, 2);
      col = clamp_color(col);

      // Write the pixel to the framebuffer memory.
      if (fb->mode == MODE_RGBA8888) {
        uint32_t pix = (uint32_t)(col.x * 256.0f) |
                       ((uint32_t)(col.y * 256.0f) << 8) |
                       ((uint32_t)(col.z * 256.0f) << 16) |
                       0xff000000u;
        *((uint32_t*)pixels) = pix;
        pixels += 4;
      } else if (fb->mode == MODE_RGBA5551) {
        uint16_t pix = (uint16_t)(col.x * 32.0f) |
                       ((uint16_t)(col.y * 32.0f) << 5) |
                       ((uint16_t)(col.z * 32.0f) << 10) |
                       0x8000u;
        *((uint16_t*)pixels) = pix;
        pixels += 2;
      }
    }
  }
}


//--------------------------------------------------------------------------------------------------
// Public API.
//--------------------------------------------------------------------------------------------------

static fb_t* s_fb;

void raytrace_init(void) {
  if (s_fb == NULL) {
    s_fb = fb_create(DEFAULT_WIDTH, DEFAULT_HEIGHT, MODE_RGBA8888);
    if (s_fb == NULL) {
      s_fb = fb_create(DEFAULT_WIDTH, DEFAULT_HEIGHT, MODE_RGBA5551);
    }
  }
}

void raytrace_deinit(void) {
  if (s_fb != NULL) {
    fb_destroy(s_fb);
    s_fb = NULL;
  }
}

void raytrace(int frame_no) {
  if (s_fb == NULL) {
    return;
  }

  fb_show(s_fb, LAYER_1);

  const float t = 0.1f * (float)frame_no;
  render_image(s_fb, t);
}

