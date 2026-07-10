// SoundFxEmitter.h - the owning class of five sibling sound-effect/transition
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

#include <Gruntz/FaderMgr.h>   // CFaderMgr::Add / Remove + the minimal CFader
#include <Gruntz/FxModeDesc.h> // CFxModeT2 / CFxModeT3 transition descriptors

// Gate global (VA 0x6455c4 = RVA 0x2455c4): nonzero => apply the channel op
// directly this frame; zero => defer it through the freshly-allocated fader.
DATA(0x002455c4)
extern i32 g_fxDirectGate;

// Reloc-masked engine externs (real mangled names so the relocs pair by symbol;
// no body - defined in their own TUs):

// The DirectDraw channel surface ops (Fill @0x13e760 / Blt @0x13ee60) come from the
// canonical single-source CDDSurface header; reloc-masked engine callees.
#include <DDrawMgr/DDSurface.h>

// The game-manager singleton (0x64556c): the emitter's +0x04 holds the CGruntzMgr,
// reached here through its Win32-safe canonical view (this is a DirectDraw TU, so
// it cannot pull the MFC CGruntzMgr). StopBankIfActive/StopBank0IfActive are the
// reloc-masked bank-stop methods on the singleton.
#include <Gruntz/GameRegistry.h>

// The shared DDraw worker manager (FxResource +0x04): its Method_158d20 "worker
// ready" predicate + the front/back/overlay surface pairs at +0x10/+0x14/+0x18.
#include <DDrawMgr/DDrawSurfacePair.h> // the ONE CDDrawSurfacePair shape (m_surface @+0x2c)
#include <DDrawMgr/DDrawSubMgrPages.h>

// The engine busy-wait (plain file-scope free function; Utils/DebugTiming.cpp).
void ActiveWait(u32 milliseconds); // 0x13dfe0 busy-wait

// CFaderMgr::Add(1|2, ..) returns the new fader as CFader* (RunFade @0x17e620 is a
// non-virtual CFader method), so the emitter holds the result directly as CFader*
// (no FaderRun view). The descriptor-side (CFader*)&t reinterpret is Add's API: the
// CFxModeDesc family is a distinct non-polymorphic root, so it cannot derive CFader.

// The DDraw surface pair CDDrawSubMgrPages holds at +0x10/+0x14/+0x18 (front/back/
// overlay). Only its +0x2c channel surface (m_surface) is read here; the ONE
// CDDrawSurfacePair shape now comes from <DDrawMgr/DDrawSurfacePair.h> above.

// The resource chain reached through emitter +0x0c: the DDraw worker manager
// (+0x04, whose surface pairs carry the channels) plus a gate flag (+0x1c).
struct FxResource {
    char _00[0x04];
    CDDrawSubMgrPages* m_worker; // +0x04 the shared DDraw worker manager
    char _08[0x14];
    i32 m_gate; // +0x1c gate field (must be set to proceed)
};

class CSoundFxEmitter {
public:
    i32 Method_fa410(i32 a1, i32 a2, i32 a3, i32 a4); // 0xfa410
    i32 Method_fa550(i32 a1, i32 a2, i32 a3, i32 a4); // 0xfa550
    i32 Method_fa790(i32 a1, i32 a2, i32 a3);         // 0xfa790
    i32 Method_fa8f0(i32 a1, i32 a2, i32 a3, i32 a4); // 0xfa8f0
    i32 Method_faa60(i32 a1, i32 a2, i32 a3);         // 0xfaa60

    char _00[0x04];
    CGameRegistry* m_gameMgr; // +0x04 sound/bank manager (the CGruntzMgr singleton)
    char _08[0x04];
    FxResource* m_resChain; // +0x0c resource chain root
    CFaderMgr* m_faderMgr;  // +0x10 fader manager
};

#endif // GRUNTZ_CSOUNDFXEMITTER_H
