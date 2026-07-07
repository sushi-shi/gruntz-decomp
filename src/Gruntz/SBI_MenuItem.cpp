#include <rva.h>
#include <Gruntz/SoundCueMgr.h>
#include <Mfc.h>
#include <Gruntz/GruntzMgr.h> // canonical MFC-side g_gameReg singleton view (CGruntzMgr)
#include <Gruntz/SBI_MenuItem.h>
#include <Gruntz/ResMgr.h> // canonical g_gameReg->m_world (m_world) view (CResMgr + CDrawTarget + CImageRegistry + CSprite)
#include <Gruntz/SbiConfig.h> // canonical config-host family (one shape)
#include <Image/CImage.h>     // canonical frame-record class (CImage::RenderFrame @0x153790)
class CDDrawWorkerRegistry {
public:
    i32 HasKeyEqual_155550(const char* k);
    void RemoveKeysEqual_155360(const char* a, const char* b);
    void Method_155630(i32 h, char* t, i32* o);
}; // 0x155550/0x155360/0x155630
// SBI_MenuItem.cpp - Gruntz CSBI_MenuItem (C:\Proj\Gruntz), the frameless methods.
// RTTI .?AVCSBI_MenuItem@@; most-derived of the SBI family
//   CSBI_MenuItem : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// The /GX-framed scalar destructor (0x1007d0) lives in SBI_MenuItemEh.cpp.
//
// These are concrete (mostly virtual-slot) methods modeled with the SBI family's
// manual-vtable-stamp device (no real `virtual`), so each matches without forcing
// a divergent compiler vtable. Sibling/engine callees are ILT-reloc-masked.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; the methods/fields touched are the only
// load-bearing facts - every call through them is reloc-masked).

// The drawable animation-frame object held at m_30 is the RTTI-confirmed CImage:
// its blit entry (CImage::RenderFrame, 0x153790, __thiscall) draws the frame at a
// screen position; m_18/m_1c are the frame's draw-origin offsets. Modeled by the
// shared <Image/CImage.h> definition; the RenderFrame call is reloc-masked.

// The keyed image-registry record (the map-lookup result) is the CSprite the image
// registry yields (its [m_64..m_68] range gates the frame table m_10.m_pData at
// +0x14; the config name is at record+0x24). The config-HOST lookup record used by
// ResolveFrame is the shared CSbiConfigRecord (<Gruntz/SbiConfig.h>), a same-shaped
// but distinct object reached through the m_24 config host, not through m_30.

// The owning game manager held at g_gameReg->m_world is the canonical CResMgr
// (ResMgr.h): the draw surface context is m_drawTarget->m_drawContext (+0x04 ->
// +0x14) and the config/name image registry is m_10 (its map embedded at +0x10).

// CMiCue/CSoundCueMgr/CMiCueMap/CMiMusicHost moved to <Gruntz/SBI_MenuItem.h>.

// The g_gameReg singleton (*0x24556c) - the canonical MFC-side CGruntzMgr view
// (<Gruntz/GruntzMgr.h>). Its +0x30 world slot (m_world) is the resource manager
// here, cast locally to CResMgr at the deref sites.
DATA(0x0024556c)
extern CGruntzMgr* g_gameReg;

// The reentrancy gate + cue-item id pair the highlight cue plays through, and the
// draw-clock mirror (wrap-safe gate compare).
DATA(0x0021ab20)
extern i32 g_61ab20;
DATA(0x0021ab24)
extern i32 g_61ab24;
DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0;

// CMiTabHost/CMiSelf moved to <Gruntz/SBI_MenuItem.h>.

// The config host + its lookup map + record come from the shared canonical family
// (<Gruntz/SbiConfig.h>): CSbiConfigHost / CSbiConfigMap / CSbiConfigRecord.

// Per-serialize round counter the CString archive helpers bump (DAT_00629ad0).
DATA(0x00229ad0)
extern i32 g_serialCounter;

// The frame-name reverse-lookup is CImageRegistry::ReadField (mgr->m_10,
// <Gruntz/ResMgr.h>); the former CMiNameReg view is gone. The archive is CSerialArchive.

// ---------------------------------------------------------------------------
// CSBI_MenuItem::ClearFrame() - zero the resolved frame handle.
// Shared by two vtable slots (0xe6d90 base-subobject teardown A, 0xe81a0 D).
RVA(0x000e6d90, 0x8)
void CSBI_MenuItem::ClearFrame() {
    m_30 = 0;
}

// CSBI_MenuItem::ClearFrame2() - the sibling subobject teardown (0xe81a0); the
// same single store, a distinct vtable slot.
RVA(0x000e81a0, 0x8)
void CSBI_MenuItem::ClearFrame2() {
    m_30 = 0;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::SerializeChain - the CSBI_Image-subobject leg of Serialize:
// transfer the resolved frame's registry name + index (read -> Lookup + frame
// range probe; write -> reverse name lookup), then tail-chain into the next base
// (CSBI_RectOnly::SerializeFields).
// @early-stop
// ~85% regalloc + stack-packing wall (CTimer::Serialize family): the switch, the
// vtable transfer, the strlen/Lookup/frame-range probe + the memset/ReadField are
// byte-correct. The residual is a register-naming coin-flip (retail pins ar in
// ebx + this in esi; the recompile swaps them) plus the dead-spill-slot packing
// (retail's temps share a slot, shifting the esp+ frame offsets by 4). Deferred.
RVA(0x000e6e40, 0x17c)
i32 CSBI_MenuItem::SerializeChain(void* arP, i32 kind, i32 a, i32 b) {
    CSerialArchive* ar = (CSerialArchive*)arP;
    if (ar == 0) {
        return 0;
    }
    CResMgr* mgr = (CResMgr*)g_gameReg->m_world;
    if (mgr == 0) {
        return 0;
    }

    char name[0x80];
    i32 idx;
    switch (kind) {
        case 7:
            // Read leg: pull the cue name + frame index from the archive, look up the
            // cue by name, and latch frame[index] (else clear).
            g_serialCounter++;
            ar->Read(name, 0x80);
            ar->Read(&idx, 4);
            if (strlen(name) != 0) {
                CSprite* r = 0;
                ((CMapStringToOb*)&mgr->m_10->m_10map)->Lookup(name, (CObject*&)r);
                if (r && idx >= r->m_firstFrame && idx <= r->m_lastFrame) {
                    m_30 = (i32)r->m_frames.m_pData[idx];
                } else {
                    m_30 = 0;
                }
            } else {
                m_30 = 0;
            }
            break;
        case 4:
            // Write leg: reverse-look-up the resolved frame's registry name + index.
            idx = 0;
            g_serialCounter++;
            memset(name, 0, sizeof(name));
            if (m_30) {
                // The registry's reverse name->id helper: CImageRegistry::ReadField
                // (0x155630) on mgr->m_10 (was the CMiNameReg protective view; folded to
                // the canonical wave 3). Adding ReadField to CImageRegistry ripples this
                // TU's DecCounter regalloc (100->74%, an accepted clean-room cost - the
                // "protective" view is not a valid keep; logic byte-unchanged).
                ((CDDrawWorkerRegistry*)mgr->m_10)->Method_155630(m_30, name, &idx);
            }
            ar->Write(name, 0x80);
            ar->Write(&idx, 4);
            break;
    }
    return SerializeFields(ar, kind, a, b) != 0;
}

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
i32 CSBI_MenuItem::InitItem(
    i32 cfg,
    i32 host,
    i32 cmd,
    i32 r0,
    i32 r1,
    i32 r2,
    i32 r3,
    i32 r4,
    void* obj,
    i32 key,
    i32 unused
) {
    if (obj == 0) {
        return 0;
    }
    if (host == 0 || cfg == 0) {
        return 0;
    }
    m_2c = (CMiTabHost*)cfg;
    m_24 = (CSbiConfigHost*)host;
    m_10 = r0;
    m_8 = 2;
    m_30 = 0;
    m_14 = r1;
    m_28 = 0;
    m_18 = r2;
    m_1c = r3;
    m_20 = r4;
    m_c = cmd;
    m_34 = 1;
    m_4 = 1;
    return ResolveFrame((i32)obj, key) != 0;
}

// ---------------------------------------------------------------------------
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
    CSbiConfigRecord* rec = 0;
    CSbiConfigHost* host = m_24;
    ((CMapStringToOb*)&host->m_10->m_10map)->Lookup((const char*)key, (CObject*&)rec);
    m_38 = rec;
    if (rec == 0) {
        return (i32)rec;
    }
    CSbiConfigRecord* r = rec;
    if (a == -1) {
        i32 lo = r->m_64;
        m_30 = r->m_14[lo];
        return m_30 != 0;
    }
    if (a >= r->m_64 && a <= r->m_68) {
        i32 v = r->m_14[a];
        m_30 = v;
        return v != 0;
    }
    m_30 = 0;
    return 0;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::DecCounter - decrement the live counter; while it is still up,
// blit the resolved frame at the menu's screen rect. 0-arg __thiscall (ret 1).
// The RenderFrame arg-block load schedule (retail loads m_18/m_14 on `this` before
// the frame's anchor fields) is register-schedule-sensitive to the TU's TOTAL
// transitive file-scope fwd-decl count (SBI_MenuItem.cpp -> GruntzMgr.h chain):
// crossing a threshold flips the two `a+b` loads pointee-first and craters this to
// 74% (see docs/patterns/header-fwd-decl-count-regalloc-butterfly.md). Kept byte-
// exact by pulling the +0x54/+0x74 sub-object DEFINITIONS (InputState.h/
// SpriteRefTable.h) into GruntzMgr.h instead of fwd-declaring them, which holds the
// count under the threshold while keeping those members typed with their real class.
RVA(0x000e82a0, 0x45)
i32 CSBI_MenuItem::DecCounter() {
    if (m_28 > 0) {
        m_28--;
        CImage* f = (CImage*)m_30;
        if (f) {
            f->RenderFrame(
                (void*)((CResMgr*)g_gameReg->m_world)->m_drawTarget->m_14,
                (void*)(m_14 + f->m_anchorX),
                (void*)(m_18 + f->m_anchorY),
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
    CMiTabHost* host = m_2c;
    if (state == 3) {
        host->TabBegin();
        host->m_10c = m_c;
        host->TabRefresh();
        host->TabCommit();
    } else if (state == 2 && a) {
        // The +0x28 sound object viewed as its cue host (multi-view cast on m_28; the
        // cue facet's map @+0x10 differs from CSoundRegistry's install map).
        CMiMusicHost* mh = (CMiMusicHost*)((CResMgr*)g_gameReg->m_world)->m_28;
        if (mh->m_30 == 0) {
            CMiCue* found = 0;
            ((CMapStringToOb*)((char*)mh + 0x10))->Lookup("GAME_TABHIGHLIGHT2", (CObject*&)found);
            if (found) {
                i32 gate = g_61ab20;
                i32 item = g_61ab24;
                if (gate != 0) {
                    CMiCue* p = found;
                    if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                        p->m_14 = g_6bf3c0;
                        p->m_10->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    CSbiConfigRecord* r = (CSbiConfigRecord*)m_38;
    i32 frame;
    if (state >= r->m_64 && state <= r->m_68) {
        frame = r->m_14[state];
    } else {
        frame = 0;
    }
    m_30 = frame;
    m_34 = state;
    ((CMiSelf*)this)->Refresh();
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
// CSBI_MenuItem::Serialize - the top (CSBI_MenuItem) leg: transfer the menu
// state tag + the cue name (read via strlen+Lookup / write via inline strcpy of
// the host name), then tail-chain into the CSBI_Image leg (SerializeChain).
// ~99.7%: byte-exact; residual is the reloc-symbol-naming tail (g_gameReg type
// name / g_serialCounter / the cue Lookup symbol vs retail's REL32 names).
RVA(0x000e8520, 0x152)
i32 CSBI_MenuItem::Serialize(void* arP, i32 kind, i32 a, i32 b) {
    CSerialArchive* ar = (CSerialArchive*)arP;
    if (ar == 0) {
        return 0;
    }
    CResMgr* mgr = (CResMgr*)g_gameReg->m_world;
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
                CSprite* found = 0;
                ((CMapStringToOb*)&mgr->m_10->m_10map)->Lookup(tmp, (CObject*&)found);
                m_38 = found;
            } else {
                m_38 = 0;
            }
            break;
        case 4:
            ar->Write(&m_34, 4);
            g_serialCounter++;
            memset(tmp, 0, sizeof(tmp));
            if (m_38) {
                strcpy(tmp, ((CSprite*)m_38)->m_name);
            }
            ar->Write(tmp, 0x80);
            break;
    }
    return SerializeChain(ar, kind, a, b) != 0;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::SetSubtype - tag the entry as the subtype-2 menu cursor.
RVA(0x001005b0, 0x8)
void CSBI_MenuItem::SetSubtype() {
    m_28 = 2;
}

// ---------------------------------------------------------------------------
// CSBI_MenuItem::SerializeFields - the CSBI_RectOnly leg: transfer the six
// owned rect/flag fields (m_4..m_18, then +0x28) through the archive.
RVA(0x0010bfc0, 0xe8)
i32 CSBI_MenuItem::SerializeFields(void* arP, i32 kind, i32 a, i32 b) {
    CSerialArchive* ar = (CSerialArchive*)arP;
    if (ar == 0) {
        return 0;
    }
    CResMgr* mgr = (CResMgr*)g_gameReg->m_world;
    if (mgr == 0) {
        return 0;
    }
    switch (kind) {
        case 7:
            ar->Read(&m_4, 4);
            ar->Read(&m_8, 4);
            ar->Read(&m_c, 4);
            ar->Read(&m_10, 4);
            ar->Read(&m_14, 0x10);
            ar->Read(&m_28, 4);
            break;
        case 4:
            ar->Write(&m_4, 4);
            ar->Write(&m_8, 4);
            ar->Write(&m_c, 4);
            ar->Write(&m_10, 4);
            ar->Write(&m_14, 0x10);
            ar->Write(&m_28, 4);
            break;
    }
    return 1;
}
