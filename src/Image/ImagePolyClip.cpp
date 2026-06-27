// ImagePolyClip.cpp - the 4-edge polygon clipper used by the CFileImage warp
// blit (the image worker CFileImage::Run, 0x1471d0, calls it). __cdecl free
// function: Sutherland-Hodgman clip of an n-vertex polygon against the four
// rectangle edges (left=a2 / top=a3 / right=a4 / bottom=a5), ping-ponging between
// two global vertex buffers (0x6a1708 <-> 0x6a21f8). Each vertex is 28 bytes
// (x,y + 5 carried attribute floats); inside vertices are copied whole (rep movs
// 7 dwords), edge crossings emit a new (x,y)-interpolated vertex. Returns 0 the
// moment a pass empties the polygon, else publishes the final vertex count to
// 0x6becf8 and returns 1. The four int clip args are converted to float in the
// prologue (fild/fstp). Field names are placeholders; offsets + code bytes are
// load-bearing. See <Gruntz/CDDrawShadeBlit.h> family for the surrounding blit.
#include <Ints.h>

#include <rva.h>
#include <string.h> // inline rep-movs struct copy

// A clip-buffer vertex: x,y (clipped) + 5 carried attribute floats (copied
// verbatim for inside vertices, left as-is for new intersection vertices).
struct PolyVtx {
    float x;
    float y;
    float attr[5];
};

// The two ping-pong clip output buffers and the published result count.
DATA(0x002a1708)
extern PolyVtx g_polyBufA[]; // 0x6a1708
DATA(0x002a21f8)
extern PolyVtx g_polyBufB[]; // 0x6a21f8
DATA(0x002becf8)
extern i32 g_polyClipCount; // 0x6becf8

// ===========================================================================
// 0x1461b0 - clip `poly` (n verts) to the rect [a2 (left) .. a4 (right)] x
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
i32 ImagePolyClipRect(PolyVtx* poly, i32 n, i32 a2, i32 a3, i32 a4, i32 a5) {
    float left = (float)a2;
    float right = (float)a4;
    float top = (float)a3;
    float bottom = (float)a5;
    i32 i;

    // Pass 1: keep x >= left. poly -> bufA.
    PolyVtx* out = g_polyBufA;
    {
        PolyVtx* prev = &poly[n - 1];
        PolyVtx* cur = poly;
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
    i32 n1 = (i32)(out - g_polyBufA);
    if (n1 == 0) {
        return 0;
    }

    // Pass 2: keep x < right. bufA -> bufB.
    out = g_polyBufB;
    {
        PolyVtx* prev = &g_polyBufA[n1 - 1];
        PolyVtx* cur = g_polyBufA;
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
    i32 n2 = (i32)(out - g_polyBufB);
    if (n2 == 0) {
        return 0;
    }

    // Pass 3: keep y >= top. bufB -> bufA.
    out = g_polyBufA;
    {
        PolyVtx* prev = &g_polyBufB[n2 - 1];
        PolyVtx* cur = g_polyBufB;
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
    i32 n3 = (i32)(out - g_polyBufA);
    if (n3 == 0) {
        return 0;
    }

    // Pass 4: keep y < bottom. bufA -> bufB.
    out = g_polyBufB;
    {
        PolyVtx* prev = &g_polyBufA[n3 - 1];
        PolyVtx* cur = g_polyBufA;
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
    i32 n4 = (i32)(out - g_polyBufB);
    if (n4 == 0) {
        return 0;
    }
    g_polyClipCount = n4;
    return 1;
}
