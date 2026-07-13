#ifndef SRC_IMAGE_CBLITINFO_H
#define SRC_IMAGE_CBLITINFO_H

// CBlitInfo - the sprite blit/draw request the DDrawMgr worker hands to the CImage
// sprite blitters (ImageSpriteBlit.cpp) and the RenderImage vtable-slot-14 selector
// (CImage.cpp). Single-source shared header (both TUs include it); field names are
// placeholders, only the OFFSETS + emitted bytes are load-bearing.

#include <Ints.h>
#include <Image/CImage.h> // BlitRect (the {left,top,right,bottom} rect)

// 0xa000 WrapCoord (world-coord wrap/transform); pointer-only here. CPlaneRender is a
// typedef of the canonical plane class now, so the fwd decl names the class itself.
class CDDrawWorkerHost;
typedef CDDrawWorkerHost CPlaneRender;

// The origin-remap target reached through info->m_xform->m_planeRender (bit 0x40000):
// CSpritePlaneRender::WrapCoord (0xa000, via the 0x295a ILT thunk; reloc-masked).
struct CBlitXform {
    char _00[0x5c];
    CPlaneRender* m_planeRender; // +0x5c  coordinate-wrap plane renderer
};

// The blit request the worker hands in (esi). Inputs: m_flags, m_adjustX/m_adjustY
// (draw-position adjust), m_xform (origin transform), m_notifyArg1/m_notifyArg0/
// m_notify (shaded pre-notify), m_drawX/m_drawY (draw position), m_clipLeft..
// m_clipBottom (clip box / sentinel). The RenderImage selector additionally reads
// m_mode (+0x40, the flip/shade/animate bit field) and the m_44/m_48 animation-counter
// pair. Outputs: m_outLeft..m_result (clipped rect, dims, result code).
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
    u32 m_44;            // +0x44  animation counter (clamped against the draw-delta g_engineFrameDelta)
    u32 m_48;            // +0x48  m_44 reset value (loaded on wrap)
    i32 m_notifyArg1;    // +0x4c  shade pre-notify arg (Notify 2nd)
    i32 m_notifyArg0;    // +0x50  shade pre-notify arg (Notify 1st)
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
