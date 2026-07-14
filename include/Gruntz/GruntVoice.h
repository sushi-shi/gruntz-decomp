// GruntVoice.h - a grunt-voice game/sound object, a CUserLogic-derived leaf
// (vftables 0x5e705c / 0x5e70b4, the CUserLogic / CUserBase pair - same shape
// every UserLogic leaf dtor folds). Trace-discovered as CGruntVoice.
//
// Modeled as a CUserLogic leaf so the empty dtor (0x119ae0) folds to the bare
// CUserLogic teardown (store 0x5e705c, ~EngStr on +0x18, store 0x5e70b4) under a
// /GX frame - byte-identical to CGruntPuddle's dtor (0x10d10) except the
// reloc-masked handler/`~EngStr` operands.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes
// are load-bearing (campaign doctrine). Own state begins at +0x40 (CUserLogic
// ends at +0x40). Layout recovered from the method field stores:
//   Setup11a7e0 writes +0x68/+0x70/+0x54/+0x60/+0x64/+0x58/+0x5c/+0x6c/+0x30
//               and m_14->m_1c (g_buteTree.Find("B"));
//   Reset11a870 writes +0x54/+0x30/+0x6c/+0x68 and m_14->m_1c (Find("A"));
//   Dispatch119e40 reads no own field (dispatches through the registry).
#ifndef GRUNTZ_GRUNTZ_CGRUNTVOICE_H
#define GRUNTZ_GRUNTZ_CGRUNTVOICE_H

struct CVariantSlot; // folded CVActColl2 (struct tag = canonical PAU mangling, matches Globals)

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

#include <Mfc.h> // CObject base + <windows.h>

#include <Gruntz/UserLogic.h>  // CUserLogic : CUserBase, EngStr, CGameObject
#include <Gruntz/InGameIcon.h> // s_actKeyB ("B" @0x60d1bc), g_frameTime (@0x645588)

// The bute store the setup/reset paths query (mov ecx,&g_buteTree; call Find).
// CButeTree is declared in <Bute/ButeMgr.h> (pulled via UserLogic.h); g_buteTree
// is owned by another TU - redeclared here so the Find call reloc-masks.
extern CButeTree g_buteTree;

// The idle-anim bute key "A" (0x60a454) the reset path looks up is the canonical
// s_codeA (bound in toobspikez, declared locally in GruntVoice.cpp); the former
// per-TU g_voiceKeyA alias lost the per-rva dedup and was folded onto it.

// ---------------------------------------------------------------------------
// The audio/sample object Setup is handed as `sample` (arg2). Its rate helper
// (0x137590, __thiscall) computes (m_a8 * 5^3 * 8) / m_3c - the play duration
// stamped into m_60. External/no-body so the call reloc-masks.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CVoiceSample);
struct CVoiceSample {};

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CGruntVoice::Dispatch (0x119e40)
// dispatches through - the SAME ActLookup/FireActivation shape as
// CSecretTeleporterTrigger::FireActivation (0x042150), but on CGruntVoice's OWN
// registry statics at 0x6514xx (the shared collection methods + the cache/alloc
// scratch globals g_actCache/g_retAddrBreadcrumb are reused). A coordinate maps to
// an Entry* either directly (within the fast [g_vactLo,g_vactHi] range) via
// g_vactBase + (coord-g_vactLo)*g_vactStride, or by a slow lookup in g_vactColl
// (0x16da80, __thiscall ret 8) which on miss rebuilds the table
// (GetRetAddr 0x16d990 -> g_actCache, g_vactColl2 insert 0x16d850 __thiscall ret
// 0xc) and yields g_vactCur. The entry's first dword is a fn-ptr; a nonzero
// entry's handler is called __thiscall on `this`. All registry globals are
// unnamed BSS (DATA-pinned so the loads reloc-mask).
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CVActColl);
struct CVActColl {
    void Construct(i32 lo, i32 hi); // 0x408710 (__thiscall ret 8: build the registry)
    i32 Find(i32 coord, i32 z);     // 0x16da80 (__thiscall ret 8)
};
extern void* GetRetAddr(); // 0x16d990

// CGruntVoice's own activation-registry statics (@0x2514xx). The DATA bindings
// live in GruntVoice.cpp (a header DATA() is not scanned by labels.py).
extern i32 g_vactLo;                 // 0x2514e0
extern i32 g_vactHi;                 // 0x2514e4
extern char* g_vactBase;             // 0x2514e8
extern i32 g_vactStride;             // 0x2514f0
extern struct CVActEntry* g_vactCur; // 0x2514ec
extern i32 g_vactScratch;            // 0x2514f8
extern CVActColl g_vactColl;         // 0x2514d8
extern CVariantSlot* g_vactColl2;    // 0x2514dc

// The cache/alloc scratch globals shared with the trigger registry (reused
// verbatim - 0x6bf464 / 0x6bf428; owned by UserLogic.cpp, declared extern here
// so the loads reloc-mask against the already-matched symbols).
extern void* g_projActCache; // 0x2bf464 canonical (?g_projActCache@@3PAXA); use this
extern void* g_actCache;     // unbound VA-typo alias of g_projActCache (legacy includers)
extern void* g_retAddrBreadcrumb;

// ---------------------------------------------------------------------------
// CGruntVoice : CUserLogic. Its own state begins at +0x40. The dtor (0x119ae0)
// adds no destructible members, so it folds the bare CUserLogic teardown. The
// class must be COMPLETE before the VActHandler typedef below so MSVC sizes the
// pointer-to-member-function on a complete single-inheritance type (a 4-byte
// code pointer with no this-adjustment) - matching retail's `mov ecx,this;
// call [entry]`. An incomplete type forces the 8-byte general PMF representation
// (an extra adjust-load + `add this`), which diverges the first dispatch call.
// ---------------------------------------------------------------------------
SIZE(CGruntVoice, 0x78);
class CGruntVoice : public CUserLogic {
public:
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    CGruntVoice(CGameObject* obj);   // 0x1198a0 (folds CUserLogic(obj) + the voice tail)
    virtual ~CGruntVoice() OVERRIDE; // 0x119ae0

    static void InitActReg(); // 0x119dc0 (construct g_vactColl over [2000,2010])
    void Dispatch(i32 coord); // 0x119e40
    i32 Setup(i32 a0, void* sample, i32 a2, i32 a3); // 0x11a7e0
    void Reset();                                    // 0x11a870
    i32 Update();                                    // 0x11a8e0 (per-frame: elapse + reposition)

    // --- CGruntVoice own fields (offsets load-bearing; roles from Setup/Reset) ---
    char m_pad40[0x54 - 0x40];
    i32 m_sample;     // +0x54  the play request's sample object (Setup stores, Reset clears)
    i32 m_icon;       // +0x58  in-game icon handle (seeded to g_frameTime)
    i32 m_5c;         // +0x5c  icon-companion slot (zeroed with m_icon; role unproven)
    i32 m_durationMs; // +0x60  cached sample play duration (sample->ComputeDuration)
    i32 m_64;         // +0x64  running/elapsed slot (zeroed on setup; role unproven)
    i32 m_source;     // +0x68  the play request's source (Setup arg0, cleared by Reset)
    i32 m_playFlags;  // +0x6c  the play request's flag word (Setup arg2, cleared by Reset)
    i32 m_owner;      // +0x70  the play request's owner (Setup arg3)
    char m_pad74[0x78 - 0x74]; // +0x74  (size 0x78 proven from the state pump's
                               //         `new CGruntVoice` = operator new(0x78))
};
VTBL(CGruntVoice, 0x1eaf6c);

// The registry Entry: its first dword is a pointer-to-member-function of
// CGruntVoice (single inheritance -> a 4-byte code pointer); Dispatch invokes it
// on `this`, emitting `mov ecx,this; call [entry]`.
typedef void (CGruntVoice::*VActHandler)();
SIZE_UNKNOWN(CVActEntry);
struct CVActEntry {
    VActHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup Dispatch folds in twice.
static inline CVActEntry* VActLookup(i32 coord) {
    g_vactScratch = 0;
    if (coord >= g_vactLo && coord <= g_vactHi) {
        return (CVActEntry*)(g_vactBase + (coord - g_vactLo) * g_vactStride);
    }
    if ((i32)((_zvec*)&g_vactColl)->GrowTo(coord, 0)) {
        return (CVActEntry*)(g_vactBase + (coord - g_vactLo) * g_vactStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_vactColl2->Set(&g_vactColl, (i32)item, 0xc);
    return g_vactCur;
}

#endif // GRUNTZ_GRUNTZ_CGRUNTVOICE_H
