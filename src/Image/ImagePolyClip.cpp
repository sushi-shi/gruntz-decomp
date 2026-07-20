#include <Ints.h>

#include <Image/RasterVtx.h> // ClipVtx + g_rasterVtx* + ImagePolyClipRect decl
#include <rva.h>
#include <string.h> // inline rep-movs struct copy

DATA(0x002a1708)
extern "C" ClipVtx g_rasterVtxA[]; // 0x6a1708
DATA(0x002a21f8)
extern "C" ClipVtx g_rasterVtxB[]; // 0x6a21f8
DATA(0x002becf8)
i32 g_rasterVtxCount = 0; // decl in Image/RasterVtx.h

// ===========================================================================
// PolyIsConvexCW (0x145e30, __cdecl) - winding-consistency test over a `count`-
// vertex polygon. For every vertex triple (i, i+1, i+2 mod count) compute the 2D
// cross product of the two edge vectors; classify its sign (0 unchanged / 1 CCW /
// 2 CW), and bail (return 0) the moment two triples disagree. Returns 1 only when
// every non-degenerate triple is clockwise (dir==2). The loop deliberately runs
// i=0..count inclusive (the closing triple re-checks vertex 0's fan).
// ===========================================================================
// @early-stop
// x87 FP-stack schedule wall (~83%, same family as ImagePolyClipRect/PolyClipRaster
// in this TU): logic/CFG/the modulo-index fan/the 3-way float-vs-0.0 sign compare are
// byte-faithful, but MSVC5's exact fld/fsub/fchs/fmulp/faddp ordering for the 2D cross
// product (v0.x kept on the FP stack, v0.y spilled to a temp) is not source-steerable.
// docs/patterns/x87-fp-stack-schedule.md.
RVA(0x00145e30, 0x125)
i32 PolyIsConvexCW(ClipVtx* verts, i32 count) {
    i32 sign = 0;
    i32 dir = 0;
    for (i32 i = 0; i <= count; i++) {
        ClipVtx* v0 = &verts[i % count];
        ClipVtx* v1 = &verts[(i + 1) % count];
        ClipVtx* v2 = &verts[(i + 2) % count];
        float dx1 = v1->x - v0->x;
        float dy1 = v1->y - v0->y;
        float dx2 = v2->x - v0->x;
        float dy2 = v2->y - v0->y;
        float cross = dx1 * dy2 - dx2 * dy1;
        if (cross != 0.0f) {
            if (cross < 0.0f) {
                sign = 2;
            } else {
                sign = 1;
            }
        }
        if (sign != 0) {
            if (dir != 0 && dir != sign) {
                return 0;
            }
            dir = sign;
        }
    }
    return dir == 2;
}

// ===========================================================================
// Clip `poly` (n verts) to the rect [a2 (left) .. a4 (right)] x
// [a3 (top) .. a5 (bottom)] in four inlined Sutherland-Hodgman passes.
// ===========================================================================
// @early-stop
// x87-schedule wall (docs/patterns/x87-fp-stack-schedule.md): the per-edge
// interpolation (slope via fdivp, the clip*slope+base lerp) is reconstructed
// byte-for-structure, but MSVC5's exact fld/fxch/fdivp/fmulp ordering across the
// four near-identical inlined passes does not reproduce from this C spelling, and
// the int->float arg conversion's slot reuse + the repeated float compares differ.
// Clip topology, edge selection, the ping-pong buffers and the /28 vertex-count
// magic divide are all correct; the FP scheduling parks it. Final-sweep candidate.
RVA(0x001461b0, 0x399)
i32 ImagePolyClipRect(ClipVtx* poly, i32 n, i32 a2, i32 a3, i32 a4, i32 a5) {
    float left = static_cast<float>(a2);
    float right = static_cast<float>(a4);
    float top = static_cast<float>(a3);
    float bottom = static_cast<float>(a5);
    i32 i;

    // Pass 1: keep x >= left. poly -> bufA.
    ClipVtx* out = g_rasterVtxA;
    {
        ClipVtx* prev = &poly[n - 1];
        ClipVtx* cur = poly;
        for (i = n; i > 0; i--) {
            if (!(prev->x < left)) {
                *out++ = *prev;
            }
            i32 emit;
            if (prev->x < left) {
                emit = !(cur->x < left);
            } else {
                emit = (cur->x < left);
            }
            if (emit) {
                out->x = left;
                out->y = prev->y + (left - prev->x) * (cur->y - prev->y) / (cur->x - prev->x);
                out++;
            }
            prev = cur;
            cur++;
        }
    }
    i32 n1 = static_cast<i32>((out - g_rasterVtxA));
    if (n1 == 0) {
        return 0;
    }

    // Pass 2: keep x < right. bufA -> bufB.
    out = g_rasterVtxB;
    {
        ClipVtx* prev = &g_rasterVtxA[n1 - 1];
        ClipVtx* cur = g_rasterVtxA;
        for (i = n1; i > 0; i--) {
            if (prev->x < right) {
                *out++ = *prev;
            }
            i32 emit;
            if (prev->x < right) {
                emit = !(cur->x < right);
            } else {
                emit = (cur->x < right);
            }
            if (emit) {
                out->x = right;
                out->y = prev->y + (right - prev->x) * (cur->y - prev->y) / (cur->x - prev->x);
                out++;
            }
            prev = cur;
            cur++;
        }
    }
    i32 n2 = static_cast<i32>((out - g_rasterVtxB));
    if (n2 == 0) {
        return 0;
    }

    // Pass 3: keep y >= top. bufB -> bufA.
    out = g_rasterVtxA;
    {
        ClipVtx* prev = &g_rasterVtxB[n2 - 1];
        ClipVtx* cur = g_rasterVtxB;
        for (i = n2; i > 0; i--) {
            if (!(prev->y < top)) {
                *out++ = *prev;
            }
            i32 emit;
            if (prev->y < top) {
                emit = !(cur->y < top);
            } else {
                emit = (cur->y < top);
            }
            if (emit) {
                out->y = top;
                out->x = prev->x + (top - prev->y) * (cur->x - prev->x) / (cur->y - prev->y);
                out++;
            }
            prev = cur;
            cur++;
        }
    }
    i32 n3 = static_cast<i32>((out - g_rasterVtxA));
    if (n3 == 0) {
        return 0;
    }

    // Pass 4: keep y < bottom. bufA -> bufB.
    out = g_rasterVtxB;
    {
        ClipVtx* prev = &g_rasterVtxA[n3 - 1];
        ClipVtx* cur = g_rasterVtxA;
        for (i = n3; i > 0; i--) {
            if (prev->y < bottom) {
                *out++ = *prev;
            }
            i32 emit;
            if (prev->y < bottom) {
                emit = !(cur->y < bottom);
            } else {
                emit = (cur->y < bottom);
            }
            if (emit) {
                out->y = bottom;
                out->x = prev->x + (bottom - prev->y) * (cur->x - prev->x) / (cur->y - prev->y);
                out++;
            }
            prev = cur;
            cur++;
        }
    }
    i32 n4 = static_cast<i32>((out - g_rasterVtxB));
    if (n4 == 0) {
        return 0;
    }
    g_rasterVtxCount = n4;
    return 1;
}
