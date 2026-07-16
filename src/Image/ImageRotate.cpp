// ImageRotate.cpp - the rotated-blit transform setup (0x145f60). __cdecl, 9 args.
// Reads the source rect dims, resolves the four pivot corners (explicit pivot arg,
// else the centered default box), rotates each centered corner by `rot` (x87
// sin/cos over the deg->rad constant at 0x5efb14) and scales by `scale`, assembles
// a four-vertex ClipVtx transform matrix on the stack - screen x/y from the
// rotation, texel a/b from the source quad - then hands it to the rotated
// rasterizer RotateRasterize @0x146550 with the clip-default (-1) tail. Field names
// are placeholders; offsets + code bytes are the load-bearing fact.
//
// Signature recovered from the retail callee (fld [esp+0x128]=arg6 deg->rad,
// fmul [esp+0x12c]=arg7 scale; last arg ref = arg9) and its three real callers
// CDDSurface::RotateBlit/ScaleBlit/RotateScaleBlit (0x141040/0x141200/0x141240),
// which each push 9 args: (int,int,int*,void*,void*,float,float,int,int).
#include <Ints.h>
#include <math.h>            // sin/cos -> fsin/fcos intrinsics at /O2 /Oi
#include <Image/RasterVtx.h> // ClipVtx + RotateRasterize decl
#include <rva.h>

// The source image arg: RotateSrcImage (width @+0x18, height @+0x1c), the shared
// rotate-blit source geometry in <Image/RasterVtx.h> (same view PolyClipRaster uses).

// @early-stop
// regalloc + x87-schedule wall (~36%, complete + correct - signature and structure
// recovered from the retail callee + its three real callers, 2026-07-12). The pivot
// resolution, the centered-box extents, the per-corner 2D rotation+scale+translate,
// the source-quad texel assignment, and the 10-arg RotateRasterize hand-off (clip
// default -1,-1,-1,-1) are all faithful: the prod[4]/mtx[4] ClipVtx pair reproduces
// retail's twin 0x1c-stride scratch arrays and lifts the frame to sub esp,0x104 (vs
// retail 0x100). The residual is two coupled MSVC5 codegen walls not source-steerable:
// (1) retail colours the hot loop over FOUR callee-saved regs (ebx/ebp/esi/edi) on a
// 0x100 frame; cl uses one fewer + a 0x104 frame, shifting every [esp+N] local/arg
// offset by 4 across the whole body; (2) the software-pipelined fld/fxch/fstp corner
// schedule (retail recomputes products and re-fxch's per store; cl hoists) - the same
// x87 wall as the sibling RotateRasterize 0x146550 and CFaderRadial::Build. permute
// cannot grow the frame or recolour, so it does not apply.
RVA(0x00145f60, 0x242)
void ImageRotateBlit(
    i32 a1,
    i32 a2,
    i32* pivot,
    void* a4,
    void* inp,
    float rot,
    float scale,
    i32 mode,
    i32 colorkey
) {
    RotateSrcImage* in = (RotateSrcImage*)inp;
    i32 h = in->m_1c;
    i32 w = in->m_18;

    // The source quad corners, stored straight into the transform's texel slots.
    i32 sq[4];
    if (pivot != 0) {
        sq[0] = pivot[0];
        sq[1] = pivot[1];
        sq[2] = pivot[2];
        sq[3] = pivot[3];
    } else {
        sq[0] = 0;
        sq[1] = 0;
        sq[2] = h - 1;
        sq[3] = w - 1;
    }

    float rad = rot * 0.01745329238f; // K(0x5efb14)  deg->rad
    float sn = static_cast<float>(sin(rad));
    float cs = static_cast<float>(cos(rad));

    // Centered half-extents of the source box (kept as ints, fild'd per corner).
    i32 hy = h >> 1;
    i32 hx = w >> 1;
    i32 ex[2] = {-hx, w - hx};
    i32 ey[2] = {-hy, h - hy};

    float tx = static_cast<float>(a1);
    float ty = static_cast<float>(a2);

    // Pass 1: the scaled centered corner products.
    ClipVtx prod[4];
    i32 k = 0;
    i32 iy, ix;
    for (iy = 0; iy < 2; iy++) {
        for (ix = 0; ix < 2; ix++) {
            prod[k].x = static_cast<float>(ex[ix]) * scale;
            prod[k].y = static_cast<float>(ey[iy]) * scale;
            k++;
        }
    }

    // Pass 2: rotate + translate into the screen x/y, texel coords into u/v.
    ClipVtx mtx[4];
    k = 0;
    for (iy = 0; iy < 2; iy++) {
        for (ix = 0; ix < 2; ix++) {
            mtx[k].x = prod[k].x * cs - prod[k].y * sn + tx;
            mtx[k].y = prod[k].x * sn + prod[k].y * cs + ty;
            mtx[k].u = static_cast<float>(sq[ix != 0 ? 3 : 0]);
            mtx[k].v = static_cast<float>(sq[iy != 0 ? 2 : 1]);
            k++;
        }
    }

    RotateRasterize(mtx, 4, (i32)a4, (i32)inp, mode, colorkey, -1, -1, -1, -1);
}

SIZE_UNKNOWN(RotateSrcImage);
