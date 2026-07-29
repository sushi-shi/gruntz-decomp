#include <DDrawMgr/WallProject.h> // own const decls (external linkage)
#include <rva.h>

#include <Ints.h>
#include <math.h>
#include <Image/RasterVtx.h>

DATA(0x001efb10)
const float g_c10 = 0.0f; // retail rdata (owner def)
DATA(0x001efb20)
const float g_c20 = 0.5f; // retail rdata (owner def)
DATA(0x001efb24)
float g_c24 = -3.1415927f; // 0x5efb24  -pi (owner-TU def; len = sqrt(dx*dx+dy*dy - g_c24))

// Retail inlined the transcendentals (/Oi alone does not intrinsify them in cl5;
// the pragma does).
#pragma intrinsic(atan2, sin, cos, sqrt, fabs)

// @early-stop
// x87-spill wall (37.46). CORRECTNESS FIX 2026-07-28 (jcc_sieve OTHER): both vertex
// passes rewrite each record IN PLACE and the translate pass STARTS OVER at record 0 -
// we were storing to `v[-8]/v[-7]`, i.e. 32 bytes BEFORE the (x,y) just read (out of
// bounds on the first iteration), and letting the translate pass run over records 3..6.
// Both loop guards now match retail's strength-reduced signed `cmp eax,g_rasterVtxB+0x74
// / jl`. The transcendentals inline (the #pragma intrinsic above - fpatan/fsin/fcos/fsqrt
// match retail). Remaining: our 7 double locals spill to an 8-aligned ebp frame
// (`and esp,-8`) while retail keeps the whole transform on the x87 stack, frameless.
// Needs the FP-temp restructure (fewer live doubles across statements).
RVA(0x001471d0, 0x1b4)
i32 ProjectWallQuad(
    CDDSurface* surface,
    i32 x0,
    i32 y0,
    i32 x1,
    i32 y1,
    i32 halfWidth,
    i32 color,
    i32 clipLeft,
    i32 clipTop,
    i32 clipRight,
    i32 clipBottom
) {
    i32 dx = x1 - x0;
    i32 dy = y1 - y0;
    double ang = atan2(static_cast<double>(dy), static_cast<double>(dx));
    double adx = fabs(static_cast<double>(dx));
    double ady = fabs(static_cast<double>(dy));
    double len = sqrt(adx * adx + ady * ady - g_c24);
    double s = sin(ang);
    double c = cos(ang);
    double hw = static_cast<double>(halfWidth);

    // The workspace is written as a flat float grid (7 floats == one ClipVtx record),
    // walked from the first record's leading float member - no cast.
    float* w = &g_rasterVtxB[0].x;
    w[0] = static_cast<float>((-s));
    w[1] = static_cast<float>(len);
    w[5] = static_cast<float>(c);
    w[6] = static_cast<float>((c + len));

    // Both passes rewrite each record IN PLACE, and the second STARTS OVER at record 0.
    // Retail's stores land at [eax-0x20]/[eax-0x1c] AFTER `add eax,0x1c`, i.e. at the very
    // (x,y) the iteration just read at [eax-4]/[eax]; and the translate pass re-loads the
    // cursor (`mov eax,g_rasterVtxB+4`) instead of continuing from the rotate pass's end.
    // Writing `v[-8]/v[-7]` stored 32 bytes BEFORE the read - out of bounds on the first
    // iteration - and let the translate pass run over records 3..6 instead of 0..3.
    // The index must also be the ONLY induction variable: retail's guard is the SIGNED
    // `cmp eax,g_rasterVtxB+0x74 / jl` a strength-reduced `i < 4` leaves behind, where a
    // hand-advanced cursor alongside `i` makes cl emit `dec ecx / jne`.
    for (i32 i = 0; i < 4; i++) {
        float* v = &w[i * 7 + 1];
        double bx = static_cast<double>(v[-1]);
        double by = -static_cast<double>(v[0]);
        v[-1] = static_cast<float>((bx * c * hw - by * s * hw));
        v[0] = static_cast<float>((bx * s * hw + by * c * hw));
    }
    for (i32 j = 0; j < 4; j++) {
        float* v = &w[j * 7 + 1];
        v[-1] = static_cast<float>((static_cast<double>(x0) + static_cast<double>(v[-1])));
        v[0] = static_cast<float>((static_cast<double>(y0) + static_cast<double>(v[0])));
    }

    // ARG FIX 2026-07-29: this dropped arg7 and passed arg8 TWICE. Retail 0x147335..
    // 0x147348 pushes [esp+0x2c]/[esp+0x28]/[esp+0x24]/[esp+0x20] - four DISTINCT slots,
    // i.e. (clipLeft, clipTop, clipRight, clipBottom), which is also what
    // CDDSurface::DecodeThunk builds from its RECT.
    if (ImagePolyClipRect(g_rasterVtxB, 4, clipLeft, clipTop, clipRight, clipBottom) != 0) {
        FillPolygon(g_rasterVtxB, g_rasterVtxCount, surface, static_cast<i16>(color));
    }
    return 1;
}
