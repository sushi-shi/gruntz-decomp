#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies (see StatusBarItem.h)
#include <rva.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
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
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // m_soundRegistry's real class (the cue host: m_10 cue map + m_30 gate)
#include <Gruntz/LeafCue.h>            // the cue-map value class (ex the CMiCue view)
#include <Gruntz/SbiConfig.h>          // canonical config-host family (one shape)
#include <Gruntz/StatusBarMgr.h>       // canonical CStatusBarMgr (LoadTabSprites)
#include <Image/CImage.h> // canonical frame-record class (CImage::RenderFrame @0x153790)


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
    static_cast<void>(unused);
    if (key == 0) {
        return 0;
    }
    if (host == 0 || owner == 0) {
        return 0;
    }
    m_2c = owner; // owning tab host (CMiTabHost view at the deref sites)
    m_24 = host;  // config host (CDDrawSurfaceMgr, cast at the deref sites)
    m_tab = a4;
    m_kind = 2;
    m_frame = 0;
    m_rect14.m_0 = rc.left;
    m_28 = 0;
    m_rect14.m_4 = rc.top;
    m_rect14.m_8 = rc.right;
    m_rect14.m_c = rc.bottom;
    m_cmd = cmd;
    m_state = 1;
    m_enabled = 1;
    return ResolveFrame(reinterpret_cast<i32>(key), frame) != 0;
}

RVA(0x000e81a0, 0x8)
void CSBI_MenuItem::ClearFrame2() {
    m_frame = 0;
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
    CDDrawSurfaceMgr* host = static_cast<CDDrawSurfaceMgr*>(m_24);
    host->m_imageRegistry->m_10map.Lookup(reinterpret_cast<const char*>(key), rec_v);
    CImageSet* rec = static_cast<CImageSet*>(rec_v);
    m_record = rec;
    if (rec == 0) {
        return reinterpret_cast<i32>(rec);
    }
    CImageSet* r = rec;
    if (a == -1) {
        i32 lo = r->m_minIndex;
        m_frame = static_cast<CImage*>(r->m_items.GetAt(lo));
        return m_frame != 0;
    }
    if (a >= r->m_minIndex && a <= r->m_maxIndex) {
        CImage* v = static_cast<CImage*>(r->m_items.GetAt(a));
        m_frame = v;
        return v != 0;
    }
    m_frame = 0;
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
// fwd-decl diet did not get back under. FOURTH FIRING 2026-07-19: the
// CButeSection==CButeMgr fold (ButeMgr.h gained the CBSecStream class DEF) re-
// cratered 100->74 with the closure fwd-decl census UNCHANGED (216=216, multiset-
// identical) - the type-table CONTENT/position variant, no count lever exists.
// @early-stop: 74% is the butterfly's
// documented floor - the shape is byte-correct, final-sweep decl-census material.
RVA(0x000e82a0, 0x45)
i32 CSBI_MenuItem::DecCounter() {
    if (m_28 > 0) {
        m_28--;
        CImage* f = m_frame;
        if (f) {
            f->RenderFrame(
                static_cast<void*>(g_gameReg->m_world->m_drawTarget->m_backPair),
                reinterpret_cast<void*>((m_rect14.m_0 + f->m_anchorX)),
                reinterpret_cast<void*>((m_rect14.m_4 + f->m_anchorY)),
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
    if (m_state == state || m_record == 0) {
        return 0;
    }
    if (state == 2 && m_state == 3) {
        return 1;
    }
    // m_2c IS the owning CStatusBarMgr (the ex CMiTabHost view is DISSOLVED): the tab
    // ops are its own methods, and m_10c is its m_activeTab latch @+0x10c.
    if (state == 3) {
        m_2c->ClearTabGroup();
        m_2c->m_activeTab = m_cmd;
        m_2c->LoadTabSprites();
        m_2c->Deactivate();
    } else if (state == 2 && a) {
        // The sound registry IS the cue host - no view: m_10 is its keyed cue map
        // (CMapStringToPtr, Ptr band), m_30 its busy/gate guard.
        CDDrawSubMgrLeafScan* mh = g_gameReg->m_world->m_soundRegistry;
        if (mh->m_30 == 0) {
            LeafCue* found = 0;
            void* foundP = 0;
            // the cue map is the Ptr band (void* values), so the element read is a
            // plain from-void cast, not a class-to-class cross-cast.
            mh->m_10.Lookup("GAME_TABHIGHLIGHT2", foundP);
            found = reinterpret_cast<LeafCue*>(foundP);
            if (found) {
                i32 gate = g_sndEnabled;
                i32 item = g_sndCueTag;
                if (gate != 0) {
                    LeafCue* p = found;
                    if (g_killCueClock - static_cast<u32>(p->m_14) >= static_cast<u32>(p->m_18)) {
                        p->m_14 = g_killCueClock;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    CImageSet* r = m_record;
    CImage* frame;
    if (state >= r->m_minIndex && state <= r->m_maxIndex) {
        frame = static_cast<CImage*>(r->m_items.GetAt(state));
    } else {
        frame = 0;
    }
    m_frame = frame;
    m_state = state;
    SetSubtype(); // slot 10 (+0x28); the CMiSelf view called it "Refresh"
    return 1;
}

RVA(0x000e8480, 0x4a)
i32 CSBI_MenuItem::ProbeState(i32 state) {
    if (state == 1 || m_record == 0) {
        return 0;
    }
    if (state == 2 && m_state == state) {
        return SetState(1, 1);
    }
    if (state == 3 && m_state == 3) {
        return SetState(1, 1);
    }
    return 1;
}

RVA(0x000e84f0, 0x16)
i32 CSBI_MenuItem::Blit() {
    if (m_state != 2) {
        return 1;
    }
    return SetState(1, 1);
}

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
            ar->Read(&m_state, 4);
            g_serialCounter++;
            ar->Read(tmp, 0x80);
            if (strlen(tmp) != 0) {
                CObject* found_ob = 0;
                mgr->m_imageRegistry->m_10map.Lookup(tmp, found_ob);
                m_record = static_cast<CImageSet*>(found_ob);
            } else {
                m_record = 0;
            }
            break;
        case 4:
            ar->Write(&m_state, 4);
            g_serialCounter++;
            memset(tmp, 0, sizeof(tmp));
            if (m_record) {
                strcpy(tmp, m_record->m_name);
            }
            ar->Write(tmp, 0x80);
            break;
    }
    // QUALIFIED = the direct CSBI_Image base leg (0xe6e40); unqualified is recursion.
    return CSBI_Image::SerializeFields(ar, kind, a, b) != 0;
}

RVA(0x001007d0, 0x7f)
CSBI_MenuItem::~CSBI_MenuItem() {
    ClearFrame2();
}

RVA(0x0010bfa0, 0x1)
void CStatusBarItem::DtorStatus() {}

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
            ar->Read(&m_enabled, 4);
            ar->Read(&m_kind, 4);
            ar->Read(&m_cmd, 4);
            ar->Read(&m_tab, 4);
            ar->Read(&m_rect14, 0x10);
            ar->Read(&m_28, 4);
            break;
        case 4:
            ar->Write(&m_enabled, 4);
            ar->Write(&m_kind, 4);
            ar->Write(&m_cmd, 4);
            ar->Write(&m_tab, 4);
            ar->Write(&m_rect14, 0x10);
            ar->Write(&m_28, 4);
            break;
    }
    return 1;
}
