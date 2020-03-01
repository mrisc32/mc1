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

//--------------------------------------------------------------------------------------------------
// Simple raytracer
// Based on an old experiment called "distray", but simplified.
//--------------------------------------------------------------------------------------------------

#include <system/types.h>
#include <system/framebuffer.h>


//--------------------------------------------------------------------------------------------------
// Types
//--------------------------------------------------------------------------------------------------

typedef float FLOAT;
typedef unsigned char UBYTE;

typedef struct {
  FLOAT x, y, z;
} VECTOR;

typedef struct {
  VECTOR color;    // Object color (r,g,b)
  FLOAT diffuse;   // Diffuse reflection (0-1)
  FLOAT reflect;   // Relefction (0-1)
  FLOAT roughness; // How rough the reflection is (0=very sharp)
} TEXTURE;

typedef struct {
  VECTOR pos; // Position (x,y,z)
  FLOAT r;    // Radius (or size)
  TEXTURE t;  // Texture
} OBJ;


//--------------------------------------------------------------------------------------------------
// Configuration.
//--------------------------------------------------------------------------------------------------

#define WIDTH 320
#define HEIGHT 180

#define EPSILON (1e-5f) // Very small value, used for coordinate-comparsions
#define MAXT (1e5f)     // Maximum t-distance for an intersection-point
#define MAXREC 5        // Maximum amount of recursions (reflection etc.)
#define DISTRIB 8       // Number of distributed rays per "virtual" ray


//--------------------------------------------------------------------------------------------------
// Scene specification.
//--------------------------------------------------------------------------------------------------

// Objects ( = spheres ). Only one sphere. Add more if you like :)
static const OBJ objs[] = {
    // Object 1
    {{0.0f, 4.0f, 1.0f}, 1.0f, {{1.0f, 0.4f, 0.0f}, 0.4f, 0.8f, 0.02f}},
    // Object 2
    {{-1.0f, 3.0f, 0.4f}, 0.4f, {{0.5f, 0.3f, 1.0f}, 0.5f, 0.9f, 0.01f}},
    // Object 3
    {{-0.3f, 1.0f, 0.4f}, 0.4f, {{0.1f, 0.95f, 0.2f}, 0.6f, 0.8f, 0.01f}},
    // Object 4
    {{1.0f, 2.0f, 0.4f}, 0.4f, {{0.86f, 0.83f, 0.0f}, 0.7f, 0.6f, 0.01f}}};

#define NUMOBJS (sizeof(objs) / sizeof(objs[0]))

// Ground position (z-pos), and textures (tiled).
static const FLOAT Groundpos = 0.0f;
static const TEXTURE Groundtxt[2] = {
    {{0.3f, 0.3f, 0.2f}, 0.8f, 0.1f, 0.02f},
    {{0.4f, 0.4f, 0.3f}, 0.8f, 0.1f, 0.01f},
};

// Only one light-source is supported (and it's white).
static const VECTOR Lightpos = {-3.0f, 1.0f, 5.0f};

// The camera position (x,y,z), and orientation.
static const VECTOR Camerapos = {1.5f, -1.4f, 0.6f};
static const VECTOR Cameraright = {3.0f, 1.0f, 0.0f};
static const VECTOR Cameradir = {-1.0f, 3.0f, 0.0f};
static const VECTOR Cameraup = {0.0f, 0.0f, 3.16228f*((FLOAT)HEIGHT/(FLOAT)WIDTH)};

// Ambient lighting (0.0-1.0)
static const FLOAT Ambient = 0.3f;

// Skycolors (Skycolor[0] = horizon, Skycolor[1] = zenit ).
static const VECTOR Skycolor[2] = {{0.3f, 0.6f, 1.0f}, {0.0f, 0.0f, 0.2f}};


//--------------------------------------------------------------------------------------------------
// For now we implement our own std functions. These should be provided by newlib or similar at
// some point.
//--------------------------------------------------------------------------------------------------

static float fabsf(float x) {
  return __builtin_fabsf(x);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

static unsigned float_to_uint(float x) {
  return *(unsigned*)&x;
}

static float uint_to_float(unsigned x) {
  return *(float*)&x;
}

#pragma GCC diagnostic pop

#if 0

static float sqrtf(float x) {
  // This is a classic Newton-Raphson implementation of the sqrt() function. It should complete in
  // less than 100 clock cycles on an MRSIC32-A1.

  if (x < 0.0f)
    return uint_to_float(0x7fffffffu);  // NaN

  // Initial guess is based on halving the exponent.
  unsigned c = float_to_uint(x);
  c = (((c & 0x7f800000u) - 0x3f800000u) / 2 + 0x3f800000u) & 0x7f800000u;
  float y = uint_to_float(c);

  // Do a few iterations to converge...
  float b;
  b = x / y; y = (y + b) * 0.5f;
  b = x / y; y = (y + b) * 0.5f;
  b = x / y; y = (y + b) * 0.5f;
  b = x / y; y = (y + b) * 0.5f;

  return y;
}

#else

static float sqrtf_normalize(const float arg, int* exp)
{
  const uint32_t arg_bits = float_to_uint(arg);

  // Find the exponent (power of 4, divided by 2).
  const uint32_t old_exponent = (arg_bits >> 24) & 0x7fu;
  *exp = ((int)old_exponent) - 63;

  // Set the exponent to 0 or 1.
  const uint32_t normalized_bits = (arg_bits & 0x80ffffffu) | 0x3f000000u;

  return uint_to_float(normalized_bits);
}

static float sqrtf_add_exp(const float x, const int exp)
{
  const uint32_t normalized_bits = float_to_uint(x);
  const uint32_t y_bits = (normalized_bits & 0x807fffffu) |
                          ((normalized_bits + (uint32_t)(exp << 23)) & 0x7f800000u);
  return uint_to_float(y_bits);
}

static float sqrtf(float x)
{
  // This implementation of sqrt is inspired by the Cephes Math Library Release 2.2.
  // Original copyright 1984, 1987, 1988, 1992 by Stephen L. Moshier

  // Separate significand and exponent.
  int e;
  x = sqrtf_normalize(x, &e);

  // Evaluate one of three polynomials depending on which range the value is in.
  float y;
  if (x > 1.41421356237F)
  {
    // x is between sqrt(2) and 2.
    x = x - 2.0F;
    y = -9.8843065718E-4F;
    y = (y * x) + 7.9479950957E-4F;
    y = (y * x) - 3.5890535377E-3F;
    y = (y * x) + 1.1028809744E-2F;
    y = (y * x) - 4.4195203560E-2F;
    y = (y * x) + 3.5355338194E-1F;
    y = (y * x) + 1.41421356237F;
  }
  else if (x > 0.707106781187F)
  {
    // x is between sqrt(2)/2 and sqrt(2).
    x = x - 1.0F;
    y = 1.35199291026E-2F;
    y = (y * x) - 2.26657767832E-2F;
    y = (y * x) + 2.78720776889E-2F;
    y = (y * x) - 3.89582788321E-2F;
    y = (y * x) + 6.24811144548E-2F;
    y = (y * x) - 1.25001503933E-1F;
    y = y * (x * x) + (0.5F * x) + 1.0F;
  }
  else
  {
    // x is between 0.5 and sqrt(2)/2.
    x = x - 0.5F;
    y = -3.9495006054E-1F;
    y = (y * x) + 5.1743034569E-1F;
    y = (y * x) - 4.3214437330E-1F;
    y = (y * x) + 3.5310730460E-1F;
    y = (y * x) - 3.5354581892E-1F;
    y = (y * x) + 7.0710676017E-1F;
    y = (y * x) + 7.07106781187E-1F;
  }

  // Re-apply the exponent.
  y = sqrtf_add_exp(y, e);

  return y;
}

#endif


//--------------------------------------------------------------------------------------------------
// Helpers (geometrical etc).
//--------------------------------------------------------------------------------------------------

static void ReflectVector(VECTOR* v2, const VECTOR* v1, const VECTOR* n) {
  FLOAT a, b;

  b = n->x * n->x + n->y * n->y + n->z * n->z;    // b = |n|^2
  a = v1->x * n->x + v1->y * n->y + v1->z * n->z; // a = v1·n 
  a = -2.0f * a / b;                              // a = -2*(v1·n)/|n|^2
  v2->x = v1->x + a * n->x;                       // v2 = v1 + n*a
  v2->y = v1->y + a * n->y;
  v2->z = v1->z + a * n->z;
}


//--------------------------------------------------------------------------------------------------
// Object intersection calculation routines.
//--------------------------------------------------------------------------------------------------

static FLOAT IntersectObjs(const VECTOR* LinP,
                           const VECTOR* LinD,
                           VECTOR* Pnt,
                           VECTOR* Norm,
                           const TEXTURE** txt) {
  unsigned objn;
  int tilenum;
  FLOAT t, ttmp, A, B, C;
  VECTOR Pos;

  t = -1.0f;

  // Try intersection with ground plane first
  if (fabsf(LinD->z) > EPSILON) {
    ttmp = (Groundpos - LinP->z) / LinD->z;
    if ((ttmp > EPSILON) && (ttmp < MAXT)) {
      t = ttmp;
      Pnt->x = LinP->x + LinD->x * t; // Calculate intersection point
      Pnt->y = LinP->y + LinD->y * t;
      Pnt->z = LinP->z + LinD->z * t;
      Norm->x = 0.0f; // Surface normal (always up)
      Norm->y = 0.0f;
      Norm->z = 1.0f;
      tilenum = (((int)(Pnt->x + 50000.0f)) + ((int)(Pnt->y + 50000.0f))) & 1;
      *txt = &Groundtxt[tilenum];
    }
  }

  // Get closest intersection (if any)
  for (objn = 0; objn < NUMOBJS; objn++) {
    Pos = objs[objn].pos;
    Pos.x -= LinP->x; // Translate object into "line-space"
    Pos.y -= LinP->y;
    Pos.z -= LinP->z;
    A = 1.0f / (LinD->x * LinD->x + LinD->y * LinD->y + LinD->z * LinD->z);
    B = (Pos.x * LinD->x + Pos.y * LinD->y + Pos.z * LinD->z) * A;
    C = (objs[objn].r * objs[objn].r - Pos.x * Pos.x - Pos.y * Pos.y - Pos.z * Pos.z) * A;
    if ((A = C + B * B) > 0.0f) { // ...else no hit
      A = sqrtf(A);
      if ((ttmp = B - A) < EPSILON)
        ttmp = B + A;
      if ((EPSILON < ttmp) && ((ttmp < t) || (t < 0.0f))) {
        t = ttmp;
        Pnt->x = LinD->x * t; // Calculate intersection point
        Pnt->y = LinD->y * t;
        Pnt->z = LinD->z * t;
        Norm->x = Pnt->x - Pos.x; // Calcualate surface normal
        Norm->y = Pnt->y - Pos.y;
        Norm->z = Pnt->z - Pos.z;
        Pnt->x += LinP->x; // Translate object back to "true-space"
        Pnt->y += LinP->y;
        Pnt->z += LinP->z;
        *txt = &objs[objn].t; // Get surface properties
      }
    }
  }

  return (t);
}


//--------------------------------------------------------------------------------------------------
// Line-tracer routine (works recursively).
//--------------------------------------------------------------------------------------------------

static void TraceLine(const VECTOR* LinP, const VECTOR* LinD, VECTOR* Color, int reccount) {
  VECTOR Pnt, Norm, LDir, NewDir, TmpCol;
  VECTOR TmpPnt, TmpNorm;
  FLOAT t, A, cosfi;
  const TEXTURE *txt, *tmptxt;
  int shadowcount;

  Color->x = Color->y = Color->z = 0.0f;

  if (reccount > 0) {
    // Try intersection with objects
    t = IntersectObjs(LinP, LinD, &Pnt, &Norm, &txt);

    // Get light-intensity in intersection-point (store in cosfi)
    if (t > EPSILON) {
      LDir.x = Lightpos.x - Pnt.x; // Get line to light from surface
      LDir.y = Lightpos.y - Pnt.y;
      LDir.z = Lightpos.z - Pnt.z;
      cosfi = LDir.x * Norm.x + LDir.y * Norm.y + LDir.z * Norm.z;
      if (cosfi > 0.0f) { // If angle between lightline and normal < PI/2
        shadowcount = 0;
        t = IntersectObjs(&Pnt, &LDir, &TmpPnt, &TmpNorm, &tmptxt);
        if ((t < EPSILON) || (t > 1.0f))
          shadowcount = DISTRIB;
        if (shadowcount > 0) {
          A = Norm.x * Norm.x + Norm.y * Norm.y + Norm.z * Norm.z;
          A *= LDir.x * LDir.x + LDir.y * LDir.y + LDir.z * LDir.z;
          cosfi = (cosfi / sqrtf(A)) * txt->diffuse * (FLOAT)shadowcount / (FLOAT)DISTRIB;
        } else {
          cosfi = 0.0f;
        }
      } else {
        cosfi = 0.0f;
      }
      Color->x = txt->color.x * (Ambient + cosfi);
      Color->y = txt->color.y * (Ambient + cosfi);
      Color->z = txt->color.z * (Ambient + cosfi);
      if (txt->reflect > EPSILON) {
        ReflectVector(&NewDir, LinD, &Norm);
        TmpCol.x = TmpCol.y = TmpCol.z = 0.0f;
        TraceLine(&Pnt, &NewDir, &TmpCol, reccount - 1);
        Color->x += TmpCol.x * txt->reflect;
        Color->y += TmpCol.y * txt->reflect;
        Color->z += TmpCol.z * txt->reflect;
      }
    } else {
      // Get sky-color (interpolate between horizon and zenit)
      A = sqrtf(LinD->x * LinD->x + LinD->y * LinD->y + LinD->z * LinD->z);
      A = fabsf(LinD->z) / A;
      Color->x = Skycolor[1].x * A + Skycolor[0].x * (1.0f - A);
      Color->y = Skycolor[1].y * A + Skycolor[0].y * (1.0f - A);
      Color->z = Skycolor[1].z * A + Skycolor[0].z * (1.0f - A);
    }

    // Make sure that the color does not exceed the maximum level
    if (Color->x > 1.0f)
      Color->x = 1.0f;
    if (Color->y > 1.0f)
      Color->y = 1.0f;
    if (Color->z > 1.0f)
      Color->z = 1.0f;
  }
}

static void TraceScene(uint32_t* pixels) {
  VECTOR PixColor, LinD, Scale;
  int sx, sy;

  Scale.y = 1.0f;
  for (sy = 0; sy < HEIGHT; sy++) {
    Scale.z = ((FLOAT)(HEIGHT / 2 - sy)) * (1.0f / (FLOAT)HEIGHT);
    for (sx = 0; sx < WIDTH; sx++) {
      Scale.x = ((FLOAT)(sx - WIDTH / 2)) * (1.0f / (FLOAT)WIDTH);

      // Calculate line-direction (from camera-center through a pixel)
      LinD.x = Cameraright.x * Scale.x + Cameradir.x * Scale.y + Cameraup.x * Scale.z;
      LinD.y = Cameraright.y * Scale.x + Cameradir.y * Scale.y + Cameraup.y * Scale.z;
      LinD.z = Cameraright.z * Scale.x + Cameradir.z * Scale.y + Cameraup.z * Scale.z;

      // Get color for pixel
      TraceLine(&Camerapos, &LinD, &PixColor, MAXREC);

      // Convert to ABGR32 and write the pixel to the framebuffer.
      uint32_t pix = (uint32_t)(PixColor.x * 255.0f) |
                     ((uint32_t)(PixColor.y * 255.0f) << 8) |
                     ((uint32_t)(PixColor.z * 255.0f) << 16) |
                     0xff000000u;
      *pixels++ = pix;
    }
  }
}


//--------------------------------------------------------------------------------------------------
// Line-tracer routine (works recursively).
//--------------------------------------------------------------------------------------------------

static fb_t* s_fb;

void raytrace_init(void) {
  s_fb = fb_create(WIDTH, HEIGHT, MODE_RGBA8888);
}

void raytrace_deinit(void) {
  fb_destroy(s_fb);
  s_fb = NULL;
}

void raytrace(int frame_no) {
  if (s_fb == NULL)
    return;

  (void)frame_no;

  fb_show(s_fb);
  TraceScene((uint32_t*)s_fb->pixels);
}
