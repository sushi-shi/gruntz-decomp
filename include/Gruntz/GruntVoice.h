#ifndef GRUNTZ_GRUNTZ_CGRUNTVOICE_H
#define GRUNTZ_GRUNTZ_CGRUNTVOICE_H

#include <rva.h>
#include <Clock64.h> // the {lo,hi} 64-bit clock pairs at +0x58/+0x60
#include <Wap32/ZVec.h>
#include <Wap32/zBitVec.h> // GetRetAddr + g_errOutOfMem/g_retAddrBreadcrumb (canonical owner)

#include <Mfc.h> // CObject base + <windows.h>

#include <Gruntz/UserLogic.h>  // CUserLogic : CUserBase, EngStr, CGameObject
#include <Gruntz/InGameIcon.h> // s_actKeyB ("B" @0x60d1bc), g_frameTime (@0x645588)
#include <Gruntz/ActReg.h>     // CActReg (extern below)

struct CVoiceSample {};
SIZE_UNKNOWN();

struct StreamVoice; // <Dsndmgr/StreamVoice.h> - the play request's sample object (m_sample)

class CGruntVoice : public CUserLogic, public CWapX {
public:
public:
    CGruntVoice(CGameObject* obj); // 0x1198a0 (folds CUserLogic(obj) + the voice tail)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).

    virtual void FireActivation(i32 id) OVERRIDE;                         // 0x119e40
    i32 Setup(i32 source, StreamVoice* sample, i32 playFlags, i32 owner); // 0x11a7e0
    void Reset();                                                         // 0x11a870
    // The act-"A" (idle) slot: keep the voice sprite hidden and report "not running".
    // It is the registrar's first CActHandler (RegisterActs_6514d8 stores ILT 0x4037bf
    // -> 0x11a8c0), and Reset/Update switch m_objAux->m_1c to "A" to select it.
    i32 IdleHidden(); // 0x11a8c0
    i32 Update();     // 0x11a8e0 (per-frame: elapse + reposition)

    // --- CGruntVoice own fields (offsets load-bearing; roles from Setup/Reset) ---
    StreamVoice* m_sample; // +0x54  the play request's sample object (Setup stores, Reset clears)
    // The two clock pairs are compared 64-bit but zeroed half by half (interleaved
    // across both pairs), so each carries BOTH names - see <Clock64.h>.
    union {
        Clock64 m_startStamp; // +0x58  play-start stamp, 64-bit
        struct {
            i32 m_icon; // +0x58  lo
            i32 m_5c;   // +0x5c  hi
        };
    };
    union {
        Clock64 m_duration; // +0x60  sample play duration, 64-bit
        struct {
            i32 m_durationMs; // +0x60  lo
            i32 m_64;         // +0x64  hi
        };
    };
    i32 m_source;              // +0x68  the play request's source (Setup arg0, cleared by Reset)
    i32 m_playFlags;           // +0x6c  the play request's flag word (Setup arg2, cleared by Reset)
    i32 m_owner;               // +0x70  the play request's owner (Setup arg3)
    char m_pad74[0x78 - 0x74]; // +0x74  (size 0x78 proven from the state pump's
                               //         `new CGruntVoice` = operator new(0x78))
};
SIZE(0x78);

typedef i32 (CUserLogic::*CActHandler)(); // == CActHandler (the slot type)

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).

#endif // GRUNTZ_GRUNTZ_CGRUNTVOICE_H
