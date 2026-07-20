#ifndef SRC_IMAGE_CBLITINFO_H
#define SRC_IMAGE_CBLITINFO_H

#include <Ints.h>
#include <Image/CImage.h> // BlitRect (the {left,top,right,bottom} rect)

class CDDrawWorkerHost;
typedef CDDrawWorkerHost CPlaneRender;

struct CBlitXform {
    char _00[0x5c];
    CPlaneRender* m_planeRender; // +0x5c  coordinate-wrap plane renderer
};

class CBlitInfo {
public:
    char _00[0x08];
    i32 m_flags; // +0x08  flags (bit 0x40000)
    char _0c[0x10 - 0x0c];
    i32 m_adjustX;       // +0x10  draw-position adjust x
    i32 m_adjustY;       // +0x14  draw-position adjust y
    i32 m_outLeft;       // +0x18  out: clipped left (top-left point)
    i32 m_outTop;        // +0x1c  out: clipped top
    BlitRect m_outRect;  // +0x20  out: clipped rect {left, top, right, bottom}
    i32 m_outWidth;      // +0x30  out: width
    i32 m_outHeight;     // +0x34  out: height
    i32 m_result;        // +0x38  out: result (0 ok / -1 culled)
    CBlitXform* m_xform; // +0x3c  origin transform
    i32 m_mode;          // +0x40  RenderImage selector: bit 1=cull, bit 2/4=flip axes,
                         //        bit 8=animate, bit 0x10000000=live gate
    u32 m_44; // +0x44  animation counter (clamped against the draw-delta g_engineFrameDelta)
    u32 m_48; // +0x48  m_44 reset value (loaded on wrap)
    i32 m_notifyArg1; // +0x4c  shade pre-notify arg (Notify 2nd)
    i32 m_notifyArg0; // +0x50  shade pre-notify arg (Notify 1st)
    char _54[0x58 - 0x54];
    i32 m_notify;     // +0x58  shade pre-notify gate
    i32 m_drawX;      // +0x5c  draw x
    i32 m_drawY;      // +0x60  draw y
    i32 m_clipLeft;   // +0x64  clip left / 0x80000000 sentinel
    i32 m_clipTop;    // +0x68  clip top
    i32 m_clipRight;  // +0x6c  clip right
    i32 m_clipBottom; // +0x70  clip bottom
};

#endif // SRC_IMAGE_CBLITINFO_H
