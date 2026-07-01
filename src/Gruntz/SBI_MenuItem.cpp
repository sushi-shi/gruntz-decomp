#include <rva.h>
#include <Mfc.h>
#include <Gruntz/SBI_MenuItem.h>
#include <Gruntz/CGameRegistry.h>
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

// The drawable animation-frame object held at m_30: its blit entry (RenderFrame,
// 0x153790, __thiscall) draws the frame at a screen position; m_18/m_1c are the
// frame's draw-origin offsets. No body on RenderFrame -> reloc-masked.
struct CMiFrame {
    void RenderFrame(i32 pSurf, i32 x, i32 y, i32 z); // 0x153790
    char m_pad0[0x18];
    i32 m_18; // +0x18  x offset
    i32 m_1c; // +0x1c  y offset
};
SIZE_UNKNOWN(CMiFrame);

// A keyed config record (the map-lookup result): a frame range + frame table.
struct CMiCueRec {
    char m_pad0[0x14];
    i32* m_14; // +0x14  frame table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame range lo
    i32 m_68; // +0x68  frame range hi
};
SIZE_UNKNOWN(CMiCueRec);

// The owning game manager held at g_gameReg->m_30: the draw surface lives at
// m_4->m_14, and the config/name registry at m_10.
struct CMiDrawHost {
    char m_pad0[0x14];
    i32 m_14; // +0x14  blit surface handle
};
SIZE_UNKNOWN(CMiDrawHost);
struct CMiGameMgr {
    char m_pad0[0x4];
    CMiDrawHost* m_4; // +0x04  draw host
    char m_pad8[0x10 - 0x8];
    void* m_10; // +0x10  config/name registry (Lookup host @+0x10)
};
SIZE_UNKNOWN(CMiGameMgr);

// The music host (mgr->m_28): a non-null +0x30 gate suppresses the cue play; its
// cue map `this` is host + 0x10.
struct CMiMusicHost {
    char m_pad0[0x30];
    void* m_30; // +0x30  gate
};
SIZE_UNKNOWN(CMiMusicHost);
struct CMiGameMgrFull {
    char m_pad0[0x28];
    CMiMusicHost* m_28; // +0x28  music host
};
SIZE_UNKNOWN(CMiGameMgrFull);
// The CGameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c).
SIZE_UNKNOWN(CGameRegistry);
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The cue lookup map (CMapStringToOb::Lookup, 0x1b8438, __thiscall ret 8).
struct CMiStrMap {
    i32 Lookup(char* key, void** out); // 0x1b8438
};
SIZE_UNKNOWN(CMiStrMap);

// A resolved cue record: a player at +0x10 plus a draw-clock gate (+0x14 last,
// +0x18 interval).
struct CMiCue {
    char m_pad0[0x10];
    void* m_10; // +0x10  player (ConfigureItem this)
    i32 m_14;   // +0x14  last draw-clock
    i32 m_18;   // +0x18  interval
};
SIZE_UNKNOWN(CMiCue);
struct CMiCuePlayer {
    void ConfigureItem(i32 item, i32 a, i32 b, i32 c); // 0x1360d0
};
SIZE_UNKNOWN(CMiCuePlayer);

// The reentrancy gate + cue-item id pair the highlight cue plays through, and the
// draw-clock mirror (wrap-safe gate compare).
DATA(0x0021ab20)
extern i32 g_61ab20;
DATA(0x0021ab24)
extern i32 g_61ab24;
DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0;

// The owning rect-only host at m_2c: SetState drives its tab state through three
// sibling thunks (all ILT-reloc-masked). m_10c is the active-tab latch.
struct CMiTabHost {
    void TabBegin();   // 0x100b00 (call 0x2329)
    void TabRefresh(); // 0x102250 (call 0x1690)
    void TabCommit();  // 0x100cb0 (call 0x125d)
    char m_pad0[0x10c];
    i32 m_10c; // +0x10c  active-tab latch
};
SIZE_UNKNOWN(CMiTabHost);

// A polymorphic view of `this` used only for the self-virtual slot-0x28 dispatch:
// 10 leading slots + Refresh at index 10 (byte 0x28). Declared (never defined) so
// no ??_7 is emitted here; `((CMiSelf*)this)->Refresh()` lowers to the exact
// mov eax,[this]; mov ecx,this; call [eax+0x28] __thiscall dispatch.
class CMiSelf {
public:
    virtual void v0();
    virtual void v4();
    virtual void v8();
    virtual void vc();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void Refresh(); // +0x28 (slot 10)
};
SIZE_UNKNOWN(CMiSelf);

// CMapWordToOb::Lookup (0x1b8008, __thiscall, ret 8): key in, *out gets the obj.
struct CMiWordMap {
    i32 Lookup(i32 key, void** out); // 0x1b8008
};
SIZE_UNKNOWN(CMiWordMap);

// The config host at +0x24: holds the config object at +0x10; the lookup map
// `this` is that object + 0x10.
struct CMiCfgHost {
    char m_pad0[0x10];
    void* m_10; // +0x10  the object carrying the map at +0x10
};
SIZE_UNKNOWN(CMiCfgHost);

// Per-serialize round counter the CString archive helpers bump (DAT_00629ad0).
DATA(0x00229ad0)
extern i32 g_serialCounter;

// The frame-name reverse-lookup helper (0x155630) on the config registry.
struct CMiNameReg {
    void ReadField(i32 handle, char* tmp, i32* outZero); // 0x155630
};
SIZE_UNKNOWN(CMiNameReg);

// The archive object passed to Serialize: field-transfer virtuals at vtable
// byte-offsets 0x2c (Read) and 0x30 (Write).
struct CMiArchive {
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Read(void* buf, i32 n);  // +0x2c
    virtual void Write(void* buf, i32 n); // +0x30
};
SIZE_UNKNOWN(CMiArchive);

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
    CMiArchive* ar = (CMiArchive*)arP;
    if (ar == 0) {
        return 0;
    }
    CMiGameMgr* mgr = (CMiGameMgr*)g_gameReg->m_30;
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
                void* found = 0;
                ((CMiStrMap*)((char*)mgr->m_10 + 0x10))->Lookup(name, &found);
                CMiCueRec* r = (CMiCueRec*)found;
                if (r && idx >= r->m_64 && idx <= r->m_68) {
                    m_30 = r->m_14[idx];
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
                ((CMiNameReg*)mgr->m_10)->ReadField(m_30, name, &idx);
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
    m_2c = (void*)cfg;
    m_24 = (void*)host;
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
    void* rec = 0;
    CMiCfgHost* host = (CMiCfgHost*)m_24;
    CMiWordMap* map = (CMiWordMap*)((char*)host->m_10 + 0x10);
    map->Lookup(key, &rec);
    m_38 = rec;
    if (rec == 0) {
        return (i32)rec;
    }
    CMiCueRec* r = (CMiCueRec*)rec;
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
// @early-stop
// ~92%: byte-exact except a 1-instruction load-schedule coin-flip in the
// RenderFrame arg setup (retail loads m_18 before f->m_1c; the recompile swaps the
// two loads within the sub-expression) + the reloc-naming tail. Not steerable from
// C (the within-expression evaluation order is the optimizer's); deferred.
RVA(0x000e82a0, 0x45)
i32 CSBI_MenuItem::DecCounter() {
    if (m_28 > 0) {
        m_28--;
        CMiFrame* f = (CMiFrame*)(void*)m_30;
        if (f) {
            f->RenderFrame(
                ((CMiGameMgr*)g_gameReg->m_30)->m_4->m_14,
                m_14 + f->m_18,
                m_18 + f->m_1c,
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
    CMiTabHost* host = (CMiTabHost*)m_2c;
    if (state == 3) {
        host->TabBegin();
        host->m_10c = m_c;
        host->TabRefresh();
        host->TabCommit();
    } else if (state == 2 && a) {
        CMiMusicHost* mh = ((CMiGameMgrFull*)g_gameReg->m_30)->m_28;
        if (mh->m_30 == 0) {
            void* found = 0;
            ((CMiStrMap*)((char*)mh + 0x10))->Lookup("GAME_TABHIGHLIGHT2", &found);
            if (found) {
                i32 gate = g_61ab20;
                i32 item = g_61ab24;
                if (gate != 0) {
                    CMiCue* p = (CMiCue*)found;
                    if (g_6bf3c0 - (u32)p->m_14 >= (u32)p->m_18) {
                        p->m_14 = g_6bf3c0;
                        ((CMiCuePlayer*)p->m_10)->ConfigureItem(item, 0, 0, 0);
                    }
                }
            }
        }
    }
    CMiCueRec* r = (CMiCueRec*)m_38;
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
    CMiArchive* ar = (CMiArchive*)arP;
    if (ar == 0) {
        return 0;
    }
    CMiGameMgr* mgr = (CMiGameMgr*)g_gameReg->m_30;
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
                void* found = 0;
                ((CMiStrMap*)((char*)mgr->m_10 + 0x10))->Lookup(tmp, &found);
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
                strcpy(tmp, (char*)m_38 + 0x24);
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
    CMiArchive* ar = (CMiArchive*)arP;
    if (ar == 0) {
        return 0;
    }
    CMiGameMgr* mgr = (CMiGameMgr*)g_gameReg->m_30;
    if (mgr == 0) {
        return 0;
    }
    char* B = (char*)this;
    switch (kind) {
        case 7:
            ar->Read(B + 0x4, 4);
            ar->Read(B + 0x8, 4);
            ar->Read(B + 0xc, 4);
            ar->Read(B + 0x10, 4);
            ar->Read(B + 0x14, 0x10);
            ar->Read(B + 0x28, 4);
            break;
        case 4:
            ar->Write(B + 0x4, 4);
            ar->Write(B + 0x8, 4);
            ar->Write(B + 0xc, 4);
            ar->Write(B + 0x10, 4);
            ar->Write(B + 0x14, 0x10);
            ar->Write(B + 0x28, 4);
            break;
    }
    return 1;
}
