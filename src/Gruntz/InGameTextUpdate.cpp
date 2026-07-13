// InGameTextUpdate.cpp - CInGameText::Update (0x997c0), the in-game text/help
// object's per-frame tick (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp.
//
// Ghidra symbol cluster proves CInGameText ownership: 0x997c0 sits inside the
// CInGameText method run (ctor 0x99110, InitActReg 0x993e0, Dispatch 0x99460,
// RegisterTextLogic 0x995c0, Update 0x997c0, Serialize 0x99a30). The class is the
// canonical <Gruntz/InGameText.h> model (folded here - no per-TU view): the two
// receivers the update path drives (m_object @ +0x10, m_38 @ +0x38) are the shared
// CGameObject the ctor binds (both == obj), reached through their real CGameObject
// fields (m_screenX/m_screenY/m_124/m_stateFlags + the +0x1a0 per-leaf anim sub).
// Only offsets / code bytes are load-bearing; the engine sub-object helpers below
// (hit-test result chain, sound chain, type-key cache) are reloc-masked externals.
#include <Gruntz/InGameText.h> // the canonical CInGameText : CUserLogic model
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h> // the shared CTypeKeyColl (g_typeColl @0x6bf650)
#include <rva.h>
#include <string.h> // strcmp (inlined /O2)

// The area-hit-test result chain (HitTestCell -> object -> type index).
SIZE_UNKNOWN(HbF14);
struct HbF14 {
    char m_pad00[0x1c];
    i32 m_1c; // +0x1c  type index fed to g_typeColl.IndexToPtr
};
SIZE_UNKNOWN(HbCellMgr);
struct HbCellMgr { // g_gameReg->m_68
    // FUN_004035f3 (thunk) __thiscall: hit-test a cell, returning the object +
    // its (areaId, subId) out-params.
};
// The per-leaf anim sub-object embedded at CGameObject+0x1a0: its Advance @0x15c360 is
// CAniAdvanceCursor::Advance_15c360 (header-less; the canonical class is a CLoadable,
// see <Gruntz/AniAdvanceCursor.h>). Local minimal decl - only the non-virtual advance
// call is reached here (via a cast), no fields/vtable.
#include <Gruntz/AniAdvanceCursor.h> // canonical CAniAdvanceCursor (Advance_15c360)
// The shared sound chain (the CBootyState ambient-cue idiom, reused here).
SIZE_UNKNOWN(HbSndPlayer);
struct HbSndPlayer {
    // Play @0x1360d0 IS CSoundCueMgr::ConfigureItem; cast at the call.
};
SIZE_UNKNOWN(HbSndEntry);
struct HbSndEntry {
    char m_pad00[0x10];
    HbSndPlayer* m_10; // +0x10
    u32 m_14;          // +0x14  last-played stamp
    u32 m_18;          // +0x18  interval
};
// (The ex-`HbSndTable` view is DISSOLVED: an empty phantom aliasing the MFC library
// CMapStringToOb::Lookup @0x1b8438 - the member below is the real map.)
SIZE_UNKNOWN(HbSndSet);
struct HbSndSet {
    char m_pad00[0x10];
    CMapStringToOb m_10; // +0x10
    char m_pad11[0x30 - 0x11];
    i32 m_30; // +0x30  active guard
};
SIZE_UNKNOWN(HbSndMgr);
struct HbSndMgr {
    char m_pad00[0x28];
    HbSndSet* m_28; // +0x28
};
SIZE_UNKNOWN(HbMgr);
struct HbMgr { // the *0x64556c singleton, this method's view
    char m_pad00[0x30];
    HbSndMgr* m_world; // +0x30  sound mgr
    char m_pad34[0x68 - 0x34];
    HbCellMgr* m_68; // +0x68  cell hit-tester
    char m_pad6c[0x13c - 0x6c];
    i32 m_13c; // +0x13c  area rect (x lo)
    i32 m_140; // +0x140  (y lo)
    i32 m_144; // +0x144  (x hi)
    i32 m_148; // +0x148  (y hi)
};
DATA(0x0024556c)
extern "C" HbMgr* g_gameReg; // _g_mgrSettings (the *0x64556c singleton)
// The 4-byte default-constructed CString cache nodes (FUN_001b9b93 == CString
// default ctor; matched array-touch loop). g_typeNodes is the base pointer.
SIZE_UNKNOWN(EngStr4);
struct EngStr4 {
    char* m_pszData; // +0x00 (4 bytes so the loop's `p++` advances by 4)
    void Ctor();     // FUN_001b9b93 __thiscall (CString default ctor)
};
// CTypeKeyColl (IndexToPtr == thunk 0x403864 -> node) is the shared
// <Gruntz/TypeKeyColl.h> shape.
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl;
DATA(0x002bf66c)
extern EngStr4* g_typeNodes;
DATA(0x002bf670)
extern i32 g_typeCount;
DATA(0x002bf3bc)
extern "C" i32 g_6bf3bc; // sub-logic clock fed to CAniAdvanceCursor::Advance_15c360
DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0; // wrap-safe draw clock
DATA(0x00244c54)
extern "C" i32 g_curPlayer; // current area index
extern i32 g_sndEnabled;    // cue enable gate
extern i32 g_sndCueTag;     // ?g_sndCueTag@@3HA (HELPBOOK sound token)
DATA(0x0020d7f8)
extern char s_codeK[]; // "K" (0x60d7f8) - the anim type-code literal

// @early-stop
// regalloc/scheduling wall (~76%): complete + correct, verified instruction-by-
// instruction vs retail (the whole front half is byte-exact modulo reloc names).
// Residual: retail loads the __thiscall receivers into ecx eagerly and schedules the
// member loads earlier, while this /O2 recompile defers the receiver to ecx after the
// arg pushes - forcing a spill/reload at the Play call and an m_10-reg swap at
// LoadPickupSprites - cascading regalloc through both epilogues. Not source-steerable.
// See docs/patterns/pin-local-for-callee-saved-reg.md.
RVA(0x000997c0, 0x1e7)
i32 CInGameText::Update() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360((i32)g_6bf3bc);

    i32 areaId;
    i32 subId;
    CGrunt* found = (CGrunt*)((CTriggerMgr*)g_gameReg->m_68)
                        ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &areaId, &subId, 1);
    if (found == 0) {
        m_cachedSubId = -1;
        m_38->m_stateFlags &= ~1;
        return 0;
    }
    if (areaId != g_curPlayer) {
        return 0;
    }
    if (m_cachedSubId != -1 && areaId == m_cachedAreaId && subId == m_cachedSubId) {
        return 0;
    }

    char** node = (char**)((_zvec*)&g_typeColl)->IndexToPtr((i32)found->m_14->m_1c);
    EngStr4* p = g_typeNodes;
    i32 n = g_typeCount;
    while (n-- != 0) {
        if (p != 0) {
            p->Ctor();
        }
        p++;
    }
    bool eq = (strcmp(*node, s_codeK) == 0);
    if (eq) {
        return 0;
    }

    if (!found->LoadPickupSprites(0x5e, 0, m_object->m_124, 0, 1)) {
        return 0;
    }

    CGameObject* o = m_object;
    i32 x = o->m_screenX;
    i32 y = o->m_screenY;
    if (x < g_gameReg->m_144 && x >= g_gameReg->m_13c && y < g_gameReg->m_148
        && y >= g_gameReg->m_140) {
        HbSndSet* set = g_gameReg->m_world->m_28;
        if (set->m_30 == 0) {
            CObject* res_ob = 0;
            set->m_10.Lookup("GAME_HELPBOOK", res_ob);
            HbSndEntry* res = (HbSndEntry*)res_ob;
            if (res != 0) {
                i32 enable = g_sndEnabled;
                i32 token = g_sndCueTag;
                if (enable != 0) {
                    u32 now = g_6bf3c0;
                    if (now - res->m_14 >= res->m_18) {
                        res->m_14 = now;
                        ((CSoundCueMgr*)res->m_10)->ConfigureItem(token, 0, 0, 0);
                    }
                }
            }
        }
    }

    m_cachedAreaId = areaId;
    m_cachedSubId = subId;
    m_38->m_stateFlags |= 1;
    return 0;
}
