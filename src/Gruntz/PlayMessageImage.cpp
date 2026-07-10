// PlayMessageImage.cpp - the two CPlay "GAME_MESSAGEZ" overlay renderers.
//
// Both look up the GAME_MESSAGEZ image set in the image registry
// (m_c->m_10->m_10map, an MFC CMapStringToOb name->object map), pull a frame out
// of the set by a signed index, and blit it centered:
//   * DrawMessageFrame (0x0d1650, non-virtual): index + useFront are caller args;
//     the frame is blit into the active viewport via the shared 0x115300 layer
//     blit helper (winapi_115300_SetRect), centered on the viewport rect.
//   * Vslot23 (0x0cfef0, vtable slot 35): presents the state's message screen -
//     Present(0x3c), pick frame 3 (or 4 when Update()==7), CImage::RenderFrame it
//     centered on the draw surface, then Flip.
//
// Kept in its own TU so the CImageSet/CImage/CDDSurface includes do not perturb the
// matched Play.cpp regalloc. Only offsets + code bytes are load-bearing.
#include <Gruntz/Play.h> // CPlay + CSpriteFactoryHolder/CImageRegistry/CDrawTarget (m_c->m_10/m_24/m_drawTarget)
#include <Image/ImageSet.h>     // CImageSet::GetAt (m_frames/m_minIndex/m_maxIndex) + CImageFrame
#include <Image/CImage.h>       // CImage::RenderFrame (0x153790)
#include <DDrawMgr/DDSurface.h> // CDDSurface::Flip (0x13e850)
#include <Globals.h>            // s_GameMessagez ("GAME_MESSAGEZ" @0x611ab8)

// The shared __cdecl layer-blit helper (0x115300, ApiCallerStubs) - blits a rect
// source into the active layer node. Declared here (namespace + fwd-decl structs)
// so the reloc resolves to the retail symbol; body lives in src/Stub/ApiCallers.cpp.
namespace ApiCallerStubs {
    struct LayerHost_115300;
    struct RectSrc_115300;
    i32 winapi_115300_SetRect(LayerHost_115300*, RectSrc_115300*, i32, i32, i32, i32);
} // namespace ApiCallerStubs

// The Present host (0xfaec0). Its class identity is unrecovered - the only inbound
// edge is ILT thunk 0x1ec9, and Vslot23 invokes it on its own `this` (CPlay is
// passed as the host). @orphan placeholder pending identity recovery.
struct PresentHost_faec0 {
    void Present(i32 arg0); // 0xfaec0
};

// ===========================================================================
// CPlay::DrawMessageFrame (0x0d1650) - draw the GAME_MESSAGEZ image `index`
// centered in the active viewport. useFront selects the front/back layer node.
// ===========================================================================
// @early-stop
// ~88%: logic byte-faithful (lookup + inlined GetAt bounds/frame + centered blit
// all match). Wall is a regalloc register-CLASS coin-flip: retail keeps m_c in the
// caller-saved ecx (leaving esi/edi/ebx free to read all 4 viewport corners UP FRONT
// into registers), while cl assigns m_c to callee-saved esi (reusing `this`'s reg),
// which forces the two corner reads to be scheduled lazily around the /2 divides.
// Same instructions + operands, only the register names/read-order differ (verified
// sema disasm --diff); tried l/t/r/b locals, cx/cy locals, fresh Lookup local - none
// tip m_c out of esi. Not source-steerable (permuter: no change).
RVA(0x000d1650, 0x90)
void CPlay::DrawMessageFrame(i32 index, i32 useFront) {
    CImageSet* set = 0;
    ((CMapStringToOb*)&m_c->m_10->m_10map)->Lookup(s_GameMessagez, (CObject*&)set);
    if (set != 0) {
        CImageFrame* frame = set->GetAt(index);
        if (frame != 0) {
            CGameViewport::SViewRect& vp = m_c->m_24->m_viewport;
            i32 cx = vp.left + (vp.right - vp.left) / 2;
            i32 cy = vp.top + (vp.bottom - vp.top) / 2;
            ApiCallerStubs::winapi_115300_SetRect(
                (ApiCallerStubs::LayerHost_115300*)m_c,
                (ApiCallerStubs::RectSrc_115300*)frame,
                cx,
                cy,
                useFront,
                1
            );
        }
    }
}

// ===========================================================================
// CPlay::Vslot23 (0x0cfef0, slot 35) - present the state's GAME_MESSAGEZ screen.
// ===========================================================================
// @early-stop
// ~95%: logic byte-faithful. The escaped Lookup out-param is copied to a fresh local
// (`set`) so cl promotes it into the callee-saved ebx across the Update() virtual
// call - matching retail. Residual is a 2-instruction /O2 SCHEDULING swap: retail
// emits the `out = 0` init AFTER pushing the Lookup args (so the slot sits at
// [esp+0x14]); cl emits it before (at [esp+0xc]). Not source-steerable (the fresh
// local is required for the ebx promotion; permuter: no change). The 4*esi/esi*4 and
// `sar eax`/`sar eax,1` diff rows are disasm-formatting only, not real byte diffs.
RVA(0x000cfef0, 0xbc)
i32 CPlay::Vslot23() {
    ((PresentHost_faec0*)this)->Present(0x3c);

    CImageSet* lookup = 0;
    ((CMapStringToOb*)&m_c->m_10->m_10map)->Lookup(s_GameMessagez, (CObject*&)lookup);
    CImageSet* set = lookup;
    if (set == 0) {
        return 0;
    }

    i32 index = 3;
    if (Update() == 7) {
        index = 4;
    }
    CImageFrame* frame = set->GetAt(index);
    if (frame == 0) {
        return 0;
    }

    CDrawTarget::SurfaceB* surf = m_c->m_drawTarget->m_14;
    if (surf == 0) {
        return 0;
    }
    ((CImage*)frame)->RenderFrame(surf, (void*)(surf->m_10 / 2), (void*)(surf->m_14 / 2), 0);
    ((CDDSurface*)m_c->m_drawTarget->m_10->m_2c)->Flip((CDDSurface*)0);
    return 1;
}
