// CSoundFxEmitter.h - the owning class of five sibling sound-effect/transition
// emitter methods (0xfa410, 0xfa550, 0xfa790, 0xfa8f0, 0xfaa60). Owner class
// unidentified by RTTI; modeled here under a placeholder name since names are
// matching-neutral. Each method: walk the resource chain to a DirectDraw channel,
// build a CFxModeT2/T3 transition descriptor on the stack, register it with the
// CFaderMgr (Add), then - gated on g_fxDirectGate - either apply the channel op
// immediately (ActiveWait + Fill/Blt) or defer it through the fresh fader; finally
// Remove the fader. The chain/channel and the bank-stop helpers are reloc-masked
// engine externs reached through the recovered offsets.
//
// Field names are placeholders (m_<hexoffset>); only offsets + emitted code bytes
// are load-bearing (campaign doctrine).
#ifndef GRUNTZ_CSOUNDFXEMITTER_H
#define GRUNTZ_CSOUNDFXEMITTER_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/CFaderMgr.h>   // CFaderMgr::Add / Remove + the minimal CFader
#include <Gruntz/CFxModeDesc.h> // CFxModeT2 / CFxModeT3 transition descriptors

// Gate global (VA 0x6455c4 = RVA 0x2455c4): nonzero => apply the channel op
// directly this frame; zero => defer it through the freshly-allocated fader.
DATA(0x002455c4)
extern i32 g_fxDirectGate;

// Reloc-masked engine externs (real mangled names so the relocs pair by symbol;
// no body - defined in their own TUs):
class CFileImage {
public:
    i32 Fill(u32 color); // 0x13e760 - colour-fill blt
};
class CDDSurface {
public:
    i32 Blt(CDDSurface* src); // 0x13ee60 - rect blt
};
class CDDrawWorkerMgr {
public:
    i32 Method_158d20(); // 0x158d20 - "is worker ready" predicate
};
class CGruntzMgr {
public:
    void StopBankIfActive();  // 0x92000
    void StopBank0IfActive(); // 0x92030
};
namespace Utils {
namespace WinAPI {
void ActiveWait(u32 milliseconds); // 0x13dfe0 busy-wait
}
} // namespace Utils
namespace ApiCallerStubs {
struct ThisStubOwnerUnknown {
    i32 winapi_17e620_GetTickCount(i32, i32, i32); // 0x17e620 (deferred-op method on the fader)
};
} // namespace ApiCallerStubs

// The resource chain reached through emitter +0x0c.
struct FxChanHolder {
    char _00[0x2c];
    void* m_2c; // +0x2c the DirectDraw channel surface
};
struct FxHolder {
    char _00[0x10];
    FxChanHolder* m_10; // +0x10
    FxChanHolder* m_14; // +0x14
    FxChanHolder* m_18; // +0x18
};
struct FxResource {
    char _00[0x04];
    FxHolder* m_04; // +0x04
    char _08[0x14];
    i32 m_1c; // +0x1c gate field (must be set to proceed)
};

class CSoundFxEmitter {
public:
    i32 Method_fa410(i32 a1, i32 a2, i32 a3, i32 a4); // 0xfa410
    i32 Method_fa550(i32 a1, i32 a2, i32 a3, i32 a4); // 0xfa550
    i32 Method_fa790(i32 a1, i32 a2, i32 a3);         // 0xfa790
    i32 Method_fa8f0(i32 a1, i32 a2, i32 a3, i32 a4); // 0xfa8f0
    i32 Method_faa60(i32 a1, i32 a2, i32 a3);         // 0xfaa60

    char _00[0x04];
    CGruntzMgr* m_04; // +0x04 sound/bank manager
    char _08[0x04];
    FxResource* m_0c; // +0x0c resource chain root
    CFaderMgr* m_10;  // +0x10 fader manager
};

#endif // GRUNTZ_CSOUNDFXEMITTER_H
