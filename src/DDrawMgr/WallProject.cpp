// WallProject.cpp - ProjectWallQuad (0x1471d0, __cdecl, leaf, returns 1). Builds a
// rotated/scaled quad in the static draw workspace (0x6a21f8..0x6a226c, four 0x1c-
// byte vertex records) from a segment (p1,p2)->(p3,p4) with half-thickness p5: it
// takes the segment angle (atan2), |dx|/|dy|, the segment length (sqrt), and the
// sin/cos of the angle, rotates the four base corners into the workspace, then calls
// the build helper (0x1461b0) and, on success, the draw helper (0x146fe0).
//
// Compiled WITHOUT the intrinsic FPU flag the retail used (/Oi), so atan2/sin/cos/
// sqrt here lower to CRT calls rather than the inline fpatan/fsin/fcos/fsqrt of
// retail. Modeled with offset-faithful workspace + const externs; the real home is
// the DDraw line/wall draw family.
#include <rva.h>

#include <Ints.h>
#include <math.h>
#include <Globals.h>
// The shared 28-byte-vertex workspace (g_rasterVtxB), its published clipped count
// (g_rasterVtxCount) and the clip/fill spine (ImagePolyClipRect @0x1461b0,
// FillPolygon @0x146fe0). ProjectWallQuad builds four ClipVtx records in g_rasterVtxB,
// clips them, then fills; p0 is the dest surface. Owner DATA() bindings elsewhere.
#include <Image/RasterVtx.h>

// Read-only float constants the rotation uses (0x5efb10/0x5efb20/0x5efb24).
DATA(0x001efb10)
extern float g_c10;
DATA(0x001efb20)
extern float g_c20;
DATA(0x001efb24)
float g_c24 = -3.1415927f; // 0x5efb24  -pi (owner-TU def; len = sqrt(dx*dx+dy*dy - g_c24))

// @early-stop
// intrinsic-FPU wall: retail inlined fpatan/fsin/fcos/fsqrt (/Oi) into one fxch-
// scheduled FPU block writing the workspace; the base TU (no /Oi) lowers atan2/sin/
// cos/sqrt to CRT calls, so the transform body diverges structurally even though the
// deltas, the four-vertex rotation, the workspace writes and the build/draw helper
// calls are the correct shape. Re-attack in the final sweep once an /Oi-intrinsic
// flag profile exists.
RVA(0x001471d0, 0x1b4)
i32 ProjectWallQuad(
    i32 p0,
    i32 p1,
    i32 p2,
    i32 p3,
    i32 p4,
    i32 p5,
    i32 p6,
    i32 p7,
    i32 p8,
    i32 p9,
    i32 p10
) {
    i32 dx = p3 - p1;
    i32 dy = p4 - p2;
    double ang = atan2(static_cast<double>(dy), static_cast<double>(dx));
    double adx = fabs(static_cast<double>(dx));
    double ady = fabs(static_cast<double>(dy));
    double len = sqrt(adx * adx + ady * ady - g_c24);
    double s = sin(ang);
    double c = cos(ang);
    double hw = static_cast<double>(p5);

    // The workspace is written as a flat float grid (7 floats == one ClipVtx record).
    float* w = (float*)g_rasterVtxB;
    w[0] = static_cast<float>((-s));
    w[1] = static_cast<float>(len);
    w[5] = static_cast<float>(c);
    w[6] = static_cast<float>((c + len));

    // rotate the four base corners through (c, -s) into the workspace records.
    float* v = &w[1];
    for (i32 i = 0; i < 4; i++) {
        double bx = static_cast<double>(v[-1]);
        double by = -static_cast<double>(v[0]);
        v[-8] = static_cast<float>((bx * c * hw - by * s * hw));
        v[-7] = static_cast<float>((bx * s * hw + by * c * hw));
        v += 7;
    }
    for (i32 j = 0; j < 4; j++) {
        v[-8] = static_cast<float>((static_cast<double>(p1) + static_cast<double>(v[-1])));
        v[-7] = static_cast<float>((static_cast<double>(p2) + static_cast<double>(v[0])));
        v += 7;
    }

    if (ImagePolyClipRect(g_rasterVtxB, 4, p8, p8, p9, p10) != 0) {
        FillPolygon(g_rasterVtxB, g_rasterVtxCount, (CDDSurface*)p0, static_cast<i16>(p6));
    }
    return 1;
}
