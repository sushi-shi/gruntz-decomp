// ImageRotate.cpp - the rotated-blit transform setup (0x145f60). __cdecl.
// Reads the source rect dims, resolves the four pivot corners (explicit pivot
// arg, else the centered default box), rotates each corner by `angle` (x87
// sin/cos, scaled), assembles a 4-corner transform matrix on the stack, then
// hands it to the rotated rasterizer at 0x146550. The corner math is heavy x87
// (fild/fxch over the constant pool at 0x5efb14). Field names are placeholders;
// offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <math.h> // sin/cos -> fsin/fcos intrinsics at /O2 /Oi
#include <rva.h>

// The source rect/image arg: width @+0x18, height @+0x1c.
struct ImgRect {
    char p0[0x18];
    i32 m_18; // +0x18  width
    i32 m_1c; // +0x1c  height
};

// The rasterizer the assembled transform is handed to (0x146550).
extern void RotateRasterize(void* xform, i32 n, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8); // 0x146550

// A rotated corner {x, y} computed for the matrix.
struct RotCorner {
    float x;
    float y;
};

// @early-stop
// x87 scheduling wall: the pivot resolution, the per-corner 2D rotation, and the
// rasterizer hand-off are faithful, but MSVC's fld/fxch corner-math interleave is
// not source-steerable (~30%).
RVA(0x00145f60, 0x242)
void ImageRotateBlit(
    i32 a1, i32 a2, i32* pivot, i32 a4, ImgRect* in, i32 a6, float angle, float scale, i32 a9, i32 a10
) {
    i32 w = in->m_18;
    i32 h = in->m_1c;

    i32 c0, c1, c2, c3;
    if (pivot != 0) {
        c0 = pivot[0];
        c1 = pivot[1];
        c2 = pivot[2];
        c3 = pivot[3];
    } else {
        c0 = 0;
        c1 = 0;
        c2 = h - 1;
        c3 = w - 1;
    }

    float a = angle * 0.01745329238f; // K(0x5efb14)  deg->rad
    float sn = (float)sin(a);
    float cs = (float)cos(a);

    // Centered half-extents of the source box.
    i32 hx = w >> 1;
    i32 hy = h >> 1;
    i32 lo[2] = {-hy, -hx};
    i32 hi[2] = {h - hy, w - hx};

    // Rotate the four centered corners; assemble the transform matrix.
    RotCorner mtx[4];
    float xs[2] = {(float)lo[1], (float)hi[1]};
    float ys[2] = {(float)lo[0], (float)hi[0]};
    i32 k = 0;
    for (i32 iy = 0; iy < 2; iy++) {
        for (i32 ix = 0; ix < 2; ix++) {
            float px = xs[ix] * scale;
            float py = ys[iy] * scale;
            mtx[k].x = px * cs - py * sn;
            mtx[k].y = px * sn + py * cs;
            k++;
        }
    }

    // The original (un-rotated) corners are folded in as the source quad.
    i32 srcQuad[4] = {c0, c1, c2, c3};

    RotateRasterize(mtx, 4, a6, a2, a9, a10, a1, srcQuad[0] | a4);
}
