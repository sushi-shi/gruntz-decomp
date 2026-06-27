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

// The static draw workspace (0x6a21f8..). Four vertex records, 0x1c stride; the
// helpers take &g_drawWS[0].
DATA(0x002a21f8)
extern float g_drawWS[];

// Read-only float constants the rotation uses (0x5efb10/0x5efb20/0x5efb24).
DATA(0x001efb10)
extern float g_c10;
DATA(0x001efb20)
extern float g_c20;
extern float g_c24;

// A draw param the draw helper consumes (0x6becf8).
DATA(0x002becf8)
extern i32 g_drawParam;

// The build/draw helpers (still Boundary stubs; reloc-masked __cdecl externs).
extern "C" i32 BuildWallQuad(float* ws, i32 n, i32 a, i32 b, i32 c, i32 d); // 0x1461b0
extern "C" void DrawWallQuad(float* ws, i32 surf, i32 e, i32 f);            // 0x146fe0

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
    double ang = atan2((double)dy, (double)dx);
    double adx = fabs((double)dx);
    double ady = fabs((double)dy);
    double len = sqrt(adx * adx + ady * ady - g_c24);
    double s = sin(ang);
    double c = cos(ang);
    double hw = (double)p5;

    g_drawWS[0] = (float)(-s);
    g_drawWS[1] = (float)len;
    g_drawWS[5] = (float)c;
    g_drawWS[6] = (float)(c + len);

    // rotate the four base corners through (c, -s) into the workspace records.
    float* v = &g_drawWS[1];
    for (i32 i = 0; i < 4; i++) {
        double bx = (double)v[-1];
        double by = -(double)v[0];
        v[-8] = (float)(bx * c * hw - by * s * hw);
        v[-7] = (float)(bx * s * hw + by * c * hw);
        v += 7;
    }
    for (i32 j = 0; j < 4; j++) {
        v[-8] = (float)((double)p1 + (double)v[-1]);
        v[-7] = (float)((double)p2 + (double)v[0]);
        v += 7;
    }

    if (BuildWallQuad(g_drawWS, 4, p8, p8, p9, p10) != 0) {
        DrawWallQuad(g_drawWS, g_drawParam, p0, p6);
    }
    return 1;
}
