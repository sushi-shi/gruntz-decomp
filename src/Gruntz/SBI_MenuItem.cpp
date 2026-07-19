// SBI_MenuItem.cpp - Gruntz CSBI_MenuItem (C:\Proj\Gruntz), the whole class: the
// concrete (mostly virtual-slot) methods + the /GX chain destructor (0x1007d0),
// re-united by the *Eh.cpp collapse (retail's one TU was compiled /GX).
// RTTI .?AVCSBI_MenuItem@@; most-derived of the SBI family
//   CSBI_MenuItem : CSBI_Image : CSBI_RectOnly : CStatusBarItem (canonical chain).
#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies (see StatusBarItem.h)
#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SoundState.h>    // g_sndEnabled/g_sndCueTag
#include <Gruntz/SerialCounter.h> // g_serialCounter
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Dsndmgr/DirectSoundMgr.h>
#include <Mfc.h>
#include <Gruntz/GruntzMgr.h> // canonical MFC-side g_gameReg singleton view (CGruntzMgr)
#include <Gruntz/SBI_MenuItem.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/Sprite.h>                // CSprite (fold: ex via ResMgr.h)
#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (fold: ex ResMgr.h CDrawTarget) // canonical g_gameReg->m_world (m_world) view (CDDrawSurfaceMgr + CDDrawSubMgrPages + CImageRegistry + CSprite)
#include <Gruntz/SbiConfig.h>          // canonical config-host family (one shape)
#include <Gruntz/StatusBarMgr.h>       // canonical CStatusBarMgr (LoadTabSprites)
#include <Image/CImage.h> // canonical frame-record class (CImage::RenderFrame @0x153790)
// The former per-TU `class CSBI_RectOnly { ClearTabGroup; Deactivate; }` view is
// folded onto the canonical chain CSBI_RectOnly (SBI_Image.h) - same mangled symbols.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; the methods/fields touched are the only
// load-bearing facts - every call through them is reloc-masked).

// The drawable animation-frame object held at m_30 is the RTTI-confirmed CImage:
// its blit entry (CImage::RenderFrame, 0x153790, __thiscall) draws the frame at a
// screen position; m_rect14.m_0/.m_4 double as the frame's draw-origin. Modeled by the
// shared <Image/CImage.h> definition; the RenderFrame call is reloc-masked.

// The keyed image-registry record (the map-lookup result) is the CSprite the image
// registry yields (its [m_64..m_68] range gates the frame table m_10.m_pData at
// +0x14; the config name is at record+0x24). The config-HOST lookup record used by
// ResolveFrame is the shared CSbiConfigRecord (<Gruntz/SbiConfig.h>), a same-shaped
// but distinct object reached through the m_24 config host, not through m_30.

// The owning game manager held at g_gameReg->m_world is the canonical CDDrawSurfaceMgr
// (ResMgr.h): the draw surface context is m_drawTarget->m_drawContext (+0x04 ->
// +0x14) and the config/name image registry is m_10 (its map embedded at +0x10).

// CMiCue/DSoundCloneInst/CMapStringToOb/CMiMusicHost moved to <Gruntz/SBI_MenuItem.h>.

// The g_gameReg singleton (*0x24556c) - the canonical MFC-side CGruntzMgr view
// (<Gruntz/GruntzMgr.h>). Its +0x30 world slot (m_world) is the resource manager
// here, cast locally to CDDrawSurfaceMgr at the deref sites.

// The reentrancy gate + cue-item id pair the highlight cue plays through, and the
// draw-clock mirror (wrap-safe gate compare).
extern "C" u32 g_killCueClock;

// CMiTabHost moved to <Gruntz/SBI_MenuItem.h>; CMiSelf is gone (it was a self-cast view
// of slot 10, which is the real CStatusBarItem::SetSubtype).

// The config host + its lookup map + record come from the shared canonical family
// (<Gruntz/SbiConfig.h>): CDDrawSurfaceMgr / CSbiConfigMap / CSbiConfigRecord.

// Per-serialize round counter the CString archive helpers bump (DAT_00629ad0).

// The frame-name reverse-lookup is CImageRegistry::ReadField (mgr->m_10,
// <Gruntz/ResMgr.h>); the former CMiNameReg view is gone. The archive is CSerialArchive.

// (0xe6d90 ClearFrame + 0xe6e40 SerializeChain re-attributed to CSBI_Image -
// SBI_Image.cpp; dossier #16.)

// ---------------------------------------------------------------------------
// CSBI_MenuItem::InitItem - configure the menu entry from its config record,
// then resolve its initial frame (ResolveFrame). 11-arg __thiscall (ret 0x2c).
// @early-stop
// scheduling/regalloc wall (~61%): logic + arg count byte-correct (ret 0x2c, the
// 13 field stores, the neg/sbb/neg bool-normalized ResolveFrame tail). The
// residual is MSVC's store schedule: retail CSEs `lea edx,[ecx+0x14]` and writes
// the m_14..m_20 block through edx, and inlines the first guard's return (sharing
// only the last two); the recompile writes the fields directly + shares one fail
// tail. Not steerable from C; deferred to the final sweep.
RVA(0x000e80e0, 0x8c)
i32 CSBI_MenuItem::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 a4,
    SbRect rc,
    const char* key,
    i32 frame,
    i32 unused
) {
    (void)unused;
    if (key == 0) {
        return 0;
    }
    if (host == 0 || owner == 0) {
        return 0;
    }
    m_2c = reinterpret_cast<i32>(owner); // owning tab host (CMiTabHost view at the deref sites)
    m_24 = reinterpret_cast<i32>(host);  // config host (CDDrawSurfaceMgr, cast at the deref sites)
    m_10 = a4;
    m_8 = 2;
    m_30 = 0;
    m_rect14.m_0 = rc.left;
    m_28 = 0;
    m_rect14.m_4 = rc.top;
    m_rect14.m_8 = rc.right;
    m_rect14.m_c = rc.bottom;
    m_c = cmd;
    m_34 = 1;
    m_4 = 1;
    return ResolveFrame(reinterpret_cast<i32>(key), frame) != 0;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::ClearFrame2 (0xe81a0): drop the resolved frame. Out-of-line (matcher-5).
RVA(0x000e81a0, 0x8)
void CSBI_MenuItem::ClearFrame2() {
    m_30 = 0;
}

// CSBI_MenuItem::ResolveFrame - look up the keyed config record in the host's
// map; if found and in range, latch its frame handle into m_30. Returns whether
// a frame was resolved. 2-arg __thiscall (ret 8).
// @early-stop
// per-path idiom/scheduling wall (~46%): logic byte-correct. The residual is two
// retail micro-idioms not steerable from C: (1) the two null guards `return` the
// already-zero key/rec register instead of `xor eax,eax`; (2) the a==-1 default
// path stores m_30 then RE-READS it for the `setne`, while the in-range path tests
// the loaded value pre-store. Each return is inline (no shared fail tail). Deferred.
RVA(0x000e81e0, 0x8b)
i32 CSBI_MenuItem::ResolveFrame(i32 key, i32 a) {
    if (key == 0) {
        return key;
    }
    // m_10map IS a CMapStringToOb (Lookup 0x1b8008, mfc_class-proven) -> CObject& out-param.
    CObject* rec_v = 0;
    CDDrawSurfaceMgr* host = reinterpret_cast<CDDrawSurfaceMgr*>(m_24);
    host->m_imageRegistry->m_10map.Lookup(reinterpret_cast<const char*>(key), rec_v);
    CImageSet* rec = (CImageSet*)rec_v;
    m_38 = rec;
    if (rec == 0) {
        return reinterpret_cast<i32>(rec);
    }
    CImageSet* r = rec;
    if (a == -1) {
        i32 lo = r->m_minIndex;
        m_30 = (CImage*)r->m_items.GetAt(lo);
        return m_30 != 0;
    }
    if (a >= r->m_minIndex && a <= r->m_maxIndex) {
        CImage* v = (CImage*)r->m_items.GetAt(a);
        m_30 = v;
        return v != 0;
    }
    m_30 = 0;
    return 0;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::DecCounter - decrement the live counter; while it is still up,
// blit the resolved frame at the menu's screen rect. 0-arg __thiscall (ret 1).
// The RenderFrame arg-block load schedule (retail loads the rect block on `this` before
// the frame's anchor fields) is register-schedule-sensitive to the TU's TOTAL
// transitive file-scope fwd-decl count (SBI_MenuItem.cpp -> GruntzMgr.h chain):
// crossing a threshold flips the two `a+b` loads pointee-first and craters this to
// 74% (see docs/patterns/header-fwd-decl-count-regalloc-butterfly.md). The old
// under-threshold trick (definitions instead of fwd-decls in GruntzMgr.h) was
// re-armed 2026-07-14 by the CSoundCueMgr==DSoundCloneInst identity fold (this TU
// now pulls the real <Dsndmgr/DirectSoundMgr.h> for ConfigureItem); a header
// fwd-decl diet did not get back under. @early-stop: 74% is the butterfly's
// documented floor - the shape is byte-correct, final-sweep decl-census material.
RVA(0x000e82a0, 0x45)
i32 CSBI_MenuItem::DecCounter() {
    if (m_28 > 0) {
        m_28--;
        CImage* f = m_30;
        if (f) {
            f->RenderFrame(
                (void*)g_gameReg->m_world->m_drawTarget->m_backPair,
                (void*)(m_rect14.m_0 + f->m_anchorX),
                (void*)(m_rect14.m_4 + f->m_anchorY),
                0
            );
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::SetState - drive the menu entry's tab state through its owning
// host (m_2c); on the activate path resolve + commit the new tab, on the
// highlight path play the GAME_TABHIGHLIGHT2 cue, then re-resolve the frame and
// fire the slot-0x28 refresh notifier. 2-arg __thiscall (ret 8).
// @early-stop
// ~93% regalloc/CSE wall (HlClickGroup0 cue-play family): logic byte-correct. The
// cue-play block + self-virtual slot-0x28 dispatch match; the residual is a
// register-naming coin-flip (retail pins the `state` arg in edi and re-reads
// m_2c each use; the recompile uses ebx + CSEs m_2c into edi) plus the
// reloc-symbol-naming tail on the cue string/globals. Not steerable from C.
RVA(0x000e8310, 0x112)
i32 CSBI_MenuItem::SetState(i32 state, i32 a) {
    if (m_34 == state || m_38 == 0) {
        return 0;
    }
    if (state == 2 && m_34 == 3) {
        return 1;
    }
    CMiTabHost* host = reinterpret_cast<CMiTabHost*>(m_2c);
    if (state == 3) {
        ((CStatusBarMgr*)host)->ClearTabGroup();
        host->m_10c = m_c;
        ((CStatusBarMgr*)host)->LoadTabSprites();
        ((CStatusBarMgr*)host)->Deactivate();
    } else if (state == 2 && a) {
        // The +0x28 sound object viewed as its cue host (multi-view cast on m_28; the
        // cue facet's map @+0x10 differs from CDDrawSubMgrLeafScan's install map).
        CMiMusicHost* mh = (CMiMusicHost*)g_gameReg->m_world->m_soundRegistry;
        if (mh->m_30 == 0) {
            CMiCue* found = 0;
            ((CMapStringToOb*)((char*)mh + 0x10))->Lookup("GAME_TABHIGHLIGHT2", (CObject*&)found);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    CMiCue* p = found;
                    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                        p->m_14 = g_killCueClock;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    CImageSet* r = m_38;
    CImage* frame;
    if (state >= r->m_minIndex && state <= r->m_maxIndex) {
        frame = (CImage*)r->m_items.GetAt(state);
    } else {
        frame = 0;
    }
    m_30 = frame;
    m_34 = state;
    SetSubtype(); // slot 10 (+0x28); the CMiSelf view called it "Refresh"
    return 1;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::ProbeState - report whether the given state can be entered; on
// the matching state pair fire the show notifier. 1-arg __thiscall (ret 4).
RVA(0x000e8480, 0x4a)
i32 CSBI_MenuItem::ProbeState(i32 state) {
    if (state == 1 || m_38 == 0) {
        return 0;
    }
    if (state == 2 && m_34 == state) {
        return SetState(1, 1);
    }
    if (state == 3 && m_34 == 3) {
        return SetState(1, 1);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::Blit - if the menu is in the ready state (m_34 == 2) fire the
// show notifier, else report ready. 0-arg __thiscall (ret 1).
RVA(0x000e84f0, 0x16)
i32 CSBI_MenuItem::Blit() {
    if (m_34 != 2) {
        return 1;
    }
    return SetState(1, 1);
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::SerializeFields - the top (CSBI_MenuItem) leg of vtable slot 1:
// transfer the menu state tag + the cue name (read via strlen+Lookup / write via
// inline strcpy of the host name), then tail-chain into the CSBI_Image leg.
// ~99.7%: byte-exact; residual is the reloc-symbol-naming tail (g_gameReg type
// name / g_serialCounter / the cue Lookup symbol vs retail's REL32 names).
RVA(0x000e8520, 0x152)
i32 CSBI_MenuItem::SerializeFields(CSerialArchive* ar, i32 kind, i32 a, i32 b) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == 0) {
        return 0;
    }

    char tmp[0x80];
    switch (kind) {
        case 7:
            ar->Read(&m_34, 4);
            g_serialCounter++;
            ar->Read(tmp, 0x80);
            if (strlen(tmp) != 0) {
                CObject* found_ob = 0;
                mgr->m_imageRegistry->m_10map.Lookup(tmp, found_ob);
                m_38 = (CImageSet*)found_ob;
            } else {
                m_38 = 0;
            }
            break;
        case 4:
            ar->Write(&m_34, 4);
            g_serialCounter++;
            memset(tmp, 0, sizeof(tmp));
            if (m_38) {
                strcpy(tmp, m_38->m_name);
            }
            ar->Write(tmp, 0x80);
            break;
    }
    // QUALIFIED = the direct CSBI_Image base leg (0xe6e40); unqualified is recursion.
    return CSBI_Image::SerializeFields(ar, kind, a, b) != 0;
}

// ---------------------------------------------------------------------------
// ~CSBI_MenuItem (0x1007d0): the /GX chain destructor - stamp ??_7CSBI_MenuItem,
// run ClearFrame2 (the slot-3 teardown above, 0xe81a0; the chain view called it
// DtorMenu), then MSVC folds the three inline base dtors in (??_7CSBI_Image +
// DtorImage, ??_7CSBI_RectOnly + DtorRect, ??_7CStatusBarItem + DtorStatus - the
// SBI_DTOR_CHAIN device) behind the /GX SEH frame with descending trylevels.
// Collapsed from SBI_MenuItemEh.cpp (the split companion TU was our invention;
// retail's one TU was compiled /GX).
RVA(0x001007d0, 0x7f)
CSBI_MenuItem::~CSBI_MenuItem() {
    ClearFrame2();
}

// ---------------------------------------------------------------------------
// CStatusBarItem::DtorStatus (0x10bfa0): the base member-teardown leg the SBI
// SBI_DTOR_CHAIN device runs from every ~CSBI_X. CStatusBarItem holds only scalar
// fields (m_4..m_2c), so the standalone body is a bare `ret` (1 byte). Defined here
// (its retail obj neighborhood is the 0x10bxxx band, immediately before
// SerializeFields) so the many dtor-chain callers reloc-bind to the real 0x10bfa0.
RVA(0x0010bfa0, 0x1)
void CStatusBarItem::DtorStatus() {}

// ---------------------------------------------------------------------------
// CStatusBarItem::SerializeFields (0x10bfc0, vtable slot 1 base leg - thunk 0x1848;
// re-attributed from CSBI_MenuItem, dossier #16): transfer the six base-region
// rect/flag fields (m_4..m_rect14, then +0x28) through the archive. Body kept in
// this TU (its retail obj neighborhood is the 0xfdc00/0x10bxxx band).
//
// This is the VIRTUAL slot-1 leg (see StatusBarItem.h): it was declared non-virtual
// (`QAE`) beside a fabricated 0-arg `SbiVfunc0` that held the slot, so nothing joined
// the declared slot to this body. The `void*` first arg was a placeholder the body then
// cast back to CSerialArchive* on its first line; the leaves already spelled the same
// param `CSerialArchive*`/`CImageSetStream*` (both typedefs of the real CFileMemBase),
// which is what an override requires. Typed here, so the cast is gone.
RVA(0x0010bfc0, 0xe8)
i32 CStatusBarItem::SerializeFields(CSerialArchive* ar, i32 kind, i32 a, i32 b) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == 0) {
        return 0;
    }
    switch (kind) {
        case 7:
            ar->Read(&m_4, 4);
            ar->Read(&m_8, 4);
            ar->Read(&m_c, 4);
            ar->Read(&m_10, 4);
            ar->Read(&m_rect14, 0x10);
            ar->Read(&m_28, 4);
            break;
        case 4:
            ar->Write(&m_4, 4);
            ar->Write(&m_8, 4);
            ar->Write(&m_c, 4);
            ar->Write(&m_10, 4);
            ar->Write(&m_rect14, 0x10);
            ar->Write(&m_28, 4);
            break;
    }
    return 1;
}
