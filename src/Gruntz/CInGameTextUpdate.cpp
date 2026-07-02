// CInGameTextUpdate.cpp - CInGameText::Update (0x997c0), the in-game text/help
// object's per-frame tick (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp.
//
// Ghidra symbol cluster proves CInGameText ownership: 0x997c0 sits inside the
// CInGameText method run (ctor 0x99110, InitActReg 0x993e0, Dispatch 0x99460,
// RegisterTextLogic 0x995c0, Update 0x997c0, Serialize 0x99a30). The class is also
// modeled in CInGameText.cpp (via <Gruntz/CInGameText.h>); this TU carries an
// independent minimal view (the update-path offsets only) so it does not perturb the
// ctor/dtor TU's polymorphic model. Only offsets / code bytes are load-bearing;
// helpers are reloc-masked externals.
#include <rva.h>
#include <string.h> // strcmp (inlined /O2)

// The area-hit-test + object sub-objects (HitTestCell result chain).
SIZE_UNKNOWN(HbF14);
struct HbF14 {
    char m_pad00[0x1c];
    i32 m_1c; // +0x1c  type index fed to g_typeColl.IndexToPtr
};
SIZE_UNKNOWN(HbFoundObj);
struct HbFoundObj { // HitTestCell result
    char m_pad00[0x14];
    HbF14* m_14;                                    // +0x14
    i32 LoadPickupSprites(i32, i32, i32, i32, i32); // FUN_00403c6a (thunk) __thiscall
};
SIZE_UNKNOWN(HbCellMgr);
struct HbCellMgr { // g_mgrSettings->m_68
    // FUN_004035f3 (thunk) __thiscall: hit-test a cell, returning the object +
    // its (areaId, subId) out-params.
    HbFoundObj* HitTestCell(i32 x, i32 y, i32* areaId, i32* subId, i32 flag);
};
SIZE_UNKNOWN(HbSub1a0);
struct HbSub1a0 {         // m_38 + 0x1a0
    void Tick(i32 clock); // FUN_0095c360 __thiscall
};
SIZE_UNKNOWN(HbLogic);
struct HbLogic { // this->m_38
    char m_pad00[0x40];
    i32 m_40; // +0x40  active bit (|=1 on cue, &=~1 on miss)
    char m_pad44[0x1a0 - 0x44];
    HbSub1a0 m_1a0; // +0x1a0
};
SIZE_UNKNOWN(HbOwner);
struct HbOwner { // this->m_10
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c  tile x
    i32 m_60; // +0x60  tile y
    char m_pad64[0x124 - 0x64];
    i32 m_124; // +0x124  pickup-sprite arg
};
// The shared sound chain (the CBootyState ambient-cue idiom, reused here).
SIZE_UNKNOWN(HbSndPlayer);
struct HbSndPlayer {
    void Play(i32 token, i32, i32, i32); // FUN_001360d0 __thiscall
};
SIZE_UNKNOWN(HbSndEntry);
struct HbSndEntry {
    char m_pad00[0x10];
    HbSndPlayer* m_10; // +0x10
    u32 m_14;          // +0x14  last-played stamp
    u32 m_18;          // +0x18  interval
};
SIZE_UNKNOWN(HbSndTable);
struct HbSndTable {
    void Find(char* szName, HbSndEntry** out); // FUN_001b8438 __thiscall, out-param
};
SIZE_UNKNOWN(HbSndSet);
struct HbSndSet {
    char m_pad00[0x10];
    HbSndTable m_10; // +0x10
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
    HbSndMgr* m_30; // +0x30  sound mgr
    char m_pad34[0x68 - 0x34];
    HbCellMgr* m_68; // +0x68  cell hit-tester
    char m_pad6c[0x13c - 0x6c];
    i32 m_13c; // +0x13c  area rect (x lo)
    i32 m_140; // +0x140  (y lo)
    i32 m_144; // +0x144  (x hi)
    i32 m_148; // +0x148  (y hi)
};
DATA(0x0024556c)
extern "C" HbMgr* g_mgrSettings; // _g_mgrSettings (the *0x64556c singleton)
// The 4-byte default-constructed CString cache nodes (FUN_001b9b93 == CString
// default ctor; matched array-touch loop). g_typeNodes is the base pointer.
SIZE_UNKNOWN(EngStr4);
struct EngStr4 {
    char* m_pszData; // +0x00 (4 bytes so the loop's `p++` advances by 4)
    void Ctor();     // FUN_001b9b93 __thiscall (CString default ctor)
};
struct CTypeKeyColl {
    char** IndexToPtr(i32 idx); // FUN_00403864 (thunk) __thiscall -> node (*node == name)
};
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl;
DATA(0x002bf66c)
extern void* g_typeNodes;
DATA(0x002bf670)
extern i32 g_typeCount;
DATA(0x002bf3bc)
extern "C" i32 g_6bf3bc; // sub-logic clock fed to HbSub1a0::Tick
DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0; // wrap-safe draw clock
DATA(0x00244c54)
extern "C" i32 g_644c54; // current area index
DATA(0x0021ab20)
extern i32 g_61ab20; // cue enable gate
DATA(0x0021ab24)
extern i32 g_sndCueTag; // ?g_sndCueTag@@3HA (HELPBOOK sound token)
DATA(0x0020d7f8)
extern "C" char g_str_K[]; // DAT_0060d7f8 == "K" (placeholder/null type marker)

SIZE_UNKNOWN(CInGameText);
class CInGameText {
public:
    i32 Update(); // 0x997c0
    char m_pad00[0x10];
    HbOwner* m_10; // +0x10
    char m_pad14[0x38 - 0x14];
    HbLogic* m_38; // +0x38
    char m_pad3c[0x54 - 0x3c];
    i32 m_54; // +0x54  cached areaId
    i32 m_58; // +0x58  cached subId
};

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
    m_38->m_1a0.Tick(g_6bf3bc);

    i32 areaId;
    i32 subId;
    HbFoundObj* found =
        g_mgrSettings->m_68->HitTestCell(m_10->m_5c, m_10->m_60, &areaId, &subId, 1);
    if (found == 0) {
        m_58 = -1;
        m_38->m_40 &= ~1;
        return 0;
    }
    if (areaId != g_644c54) {
        return 0;
    }
    if (m_58 != -1 && areaId == m_54 && subId == m_58) {
        return 0;
    }

    char** node = g_typeColl.IndexToPtr(found->m_14->m_1c);
    EngStr4* p = (EngStr4*)g_typeNodes;
    i32 n = g_typeCount;
    while (n-- != 0) {
        if (p != 0) {
            p->Ctor();
        }
        p++;
    }
    bool eq = (strcmp(*node, g_str_K) == 0);
    if (eq) {
        return 0;
    }

    if (!found->LoadPickupSprites(0x5e, 0, m_10->m_124, 0, 1)) {
        return 0;
    }

    HbOwner* o = m_10;
    i32 x = o->m_5c;
    i32 y = o->m_60;
    if (x < g_mgrSettings->m_144 && x >= g_mgrSettings->m_13c && y < g_mgrSettings->m_148
        && y >= g_mgrSettings->m_140) {
        HbSndSet* set = g_mgrSettings->m_30->m_28;
        if (set->m_30 == 0) {
            HbSndEntry* res = 0;
            set->m_10.Find("GAME_HELPBOOK", &res);
            if (res != 0) {
                i32 enable = g_61ab20;
                i32 token = g_sndCueTag;
                if (enable != 0) {
                    u32 now = g_6bf3c0;
                    if (now - res->m_14 >= res->m_18) {
                        res->m_14 = now;
                        res->m_10->Play(token, 0, 0, 0);
                    }
                }
            }
        }
    }

    m_54 = areaId;
    m_58 = subId;
    m_38->m_40 |= 1;
    return 0;
}
