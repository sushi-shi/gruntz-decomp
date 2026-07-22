#ifndef GRUNTZ_GRUNTZ_CGRUNTVOICE_H
#define GRUNTZ_GRUNTZ_CGRUNTVOICE_H

struct CVariantSlot; // folded CVActColl2 (struct tag = canonical PAU mangling, matches Globals)

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/zBitVec.h> // GetRetAddr + g_projActCache/g_retAddrBreadcrumb (canonical owner)

#include <Mfc.h> // CObject base + <windows.h>

#include <Gruntz/UserLogic.h>  // CUserLogic : CUserBase, EngStr, CGameObject
#include <Gruntz/InGameIcon.h> // s_actKeyB ("B" @0x60d1bc), g_frameTime (@0x645588)

struct CVoiceSample {};
SIZE_UNKNOWN();

struct CVActColl {
    void Construct(i32 lo, i32 hi); // 0x408710 (__thiscall ret 8: build the registry)
    i32 Find(i32 coord, i32 z);     // 0x16da80 (__thiscall ret 8)
};
SIZE_UNKNOWN();

extern i32 g_vactLo;                 // 0x2514e0
extern i32 g_vactHi;                 // 0x2514e4
extern char* g_vactBase;             // 0x2514e8
extern i32 g_vactStride;             // 0x2514f0
extern struct CVActEntry* g_vactCur; // 0x2514ec
extern i32 g_vactScratch;            // 0x2514f8
extern CVActColl g_vactColl;         // 0x2514d8
extern CVariantSlot* g_vactColl2;    // 0x2514dc

class CGruntVoice : public CUserLogic, public CWapX {
public:
public:
    CGruntVoice(CGameObject* obj);   // 0x1198a0 (folds CUserLogic(obj) + the voice tail)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).

    static void InitActReg(); // 0x119dc0 (construct g_vactColl over [2000,2010])
    virtual void FireActivation(i32 id) OVERRIDE;    // 0x119e40
    i32 Setup(i32 a0, void* sample, i32 a2, i32 a3); // 0x11a7e0
    void Reset();                                    // 0x11a870
    i32 Update();                                    // 0x11a8e0 (per-frame: elapse + reposition)

    // --- CGruntVoice own fields (offsets load-bearing; roles from Setup/Reset) ---
    i32 m_sample;     // +0x54  the play request's sample object (Setup stores, Reset clears)
    i32 m_icon;       // +0x58  play-start stamp LO (an i64 pair w/ m_5c; the elapsed
                      //         check reads *(i64*)&m_icon - INTERLEAVED-zero keep, see task 23)
    i32 m_5c;         // +0x5c  play-start stamp HI
    i32 m_durationMs; // +0x60  sample play duration LO (i64 pair w/ m_64)
    i32 m_64;         // +0x64  sample play duration HI
    i32 m_source;     // +0x68  the play request's source (Setup arg0, cleared by Reset)
    i32 m_playFlags;  // +0x6c  the play request's flag word (Setup arg2, cleared by Reset)
    i32 m_owner;      // +0x70  the play request's owner (Setup arg3)
    char m_pad74[0x78 - 0x74]; // +0x74  (size 0x78 proven from the state pump's
                               //         `new CGruntVoice` = operator new(0x78))
};
SIZE(0x78);

typedef void (CUserLogic::*VActHandler)();
struct CVActEntry {
    VActHandler m_fn; // [entry]
};
SIZE_UNKNOWN();

static inline CVActEntry* VActLookup(i32 coord) {
    g_vactScratch = 0;
    if (coord >= g_vactLo && coord <= g_vactHi) {
        return reinterpret_cast<CVActEntry*>((g_vactBase + (coord - g_vactLo) * g_vactStride));
    }
    if (reinterpret_cast<i32>((reinterpret_cast<_zvec*>(&g_vactColl))->GrowTo(coord, 0))) {
        return reinterpret_cast<CVActEntry*>((g_vactBase + (coord - g_vactLo) * g_vactStride));
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_vactColl2->Set(&g_vactColl, reinterpret_cast<i32>(item), 0xc);
    return g_vactCur;
}

#include <Gruntz/ActReg.h> // CActReg (extern below)
extern CActReg g_vtrigActReg; // 0x00251500


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern i32 VTrigLogic_11a700();

#endif // GRUNTZ_GRUNTZ_CGRUNTVOICE_H
