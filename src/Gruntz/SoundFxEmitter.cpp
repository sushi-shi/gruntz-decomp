// SoundFxEmitter.cpp - five sibling sound-effect/screen-transition emitters
// (0xfa410, 0xfa550, 0xfa790, 0xfa8f0, 0xfaa60). See CSoundFxEmitter.h for the
// recovered class/chain layout. Each: gate on the resource chain, fill a
// CFxModeT2/T3 transition descriptor on the stack, register it with the CFaderMgr,
// then - per g_fxDirectGate - apply the channel op now or defer it through the new
// fader, and finally Remove the fader. All callees are reloc-masked externs.
#include <Gruntz/SoundFxEmitter.h>
#include <Gruntz/Fader.h>

// ---------------------------------------------------------------------------
// 0xfa410: single-channel type-2 emitter (4 args).
RVA(0x000fa410, 0xf5)
i32 CSoundFxEmitter::Method_fa410(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chan = m_resChain->m_worker->m_frontPair->m_surface;
    if (chan == 0) {
        return 0;
    }

    CFxModeT2 t;
    t.m_10 = 1;
    t.m_18 = a1;
    t.m_1c = a2;
    t.m_04 = (i32)chan;
    t.m_08 = 0;
    CFader* f = mgr->Add(1, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_fxDirectGate != 0) {
        Utils::WinAPI::ActiveWait(a3);
        m_resChain->m_worker->m_frontPair->m_surface->Fill(0);
    } else {
        f->RunFade(a3, a4, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// ---------------------------------------------------------------------------
// 0xfa550: two-channel type-2 emitter (4 args). Blt channel A onto channel B.
// @early-stop
// 98.7% - logic byte-faithful. Residual is store/arg scheduling: cl hoists the
// m_18=a1 temp store before the Add-arg push where retail hoists m_1c=a2, and the
// deferred winapi_17e620 branch picks ecx/edx vs retail's eax/ecx for the two arg
// temporaries. Both are /O2 instruction-scheduling choices, not source-steerable.
RVA(0x000fa550, 0x10c)
i32 CSoundFxEmitter::Method_fa550(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDSurface* chanB = m_resChain->m_worker->m_backPair->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT2 t;
    t.m_1c = a2;
    t.m_10 = 0;
    t.m_18 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(1, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_fxDirectGate != 0) {
        Utils::WinAPI::ActiveWait(a3);
        m_resChain->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a3, a4, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// ---------------------------------------------------------------------------
// 0xfa790: two-channel type-3 emitter (3 args).
// @early-stop
// 99.1% - logic byte-faithful. Residual is the chanA/chanB esi<->edi regalloc
// swap (docs/patterns/zero-register-pinning.md family): retail gives the
// longer-lived cached channel (chanB) the preferred callee-saved esi and the
// re-derived channel (chanA) edi, while cl's greedy allocator assigns them the
// other way round; identical structure, a few push/mov reg bytes differ. Not
// source-steerable (computation order is pinned by the chain walk).
RVA(0x000fa790, 0x104)
i32 CSoundFxEmitter::Method_fa790(i32 a1, i32 a2, i32 a3) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDSurface* chanB = m_resChain->m_worker->m_backPair->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 0;
    t.m_10 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_fxDirectGate != 0) {
        Utils::WinAPI::ActiveWait(a2);
        m_resChain->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a2, a3, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// ---------------------------------------------------------------------------
// 0xfa8f0: two-channel type-3 emitter (4 args); channel B chosen via a4 +
// CDDrawWorkerMgr::Method_158d20. No bank-stop bracketing on this variant.
// @early-stop
// 98.4% - logic byte-faithful. Same chanA/chanB esi<->edi regalloc swap as
// 0xfa790 plus the deferred-branch arg-temp register choice (see those notes);
// /O2 scheduling/regalloc, not source-steerable.
RVA(0x000fa8f0, 0x118)
i32 CSoundFxEmitter::Method_fa8f0(i32 a1, i32 a2, i32 a3, i32 a4) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chanA = m_resChain->m_worker->m_frontPair->m_surface;
    if (chanA == 0) {
        return 0;
    }
    CDDrawSurfacePair* holderB;
    if (a4 != 0 && m_resChain->m_worker->Method_158d20() != 0) {
        holderB = m_resChain->m_worker->m_overlayPair;
    } else {
        holderB = m_resChain->m_worker->m_backPair;
    }
    CDDSurface* chanB = holderB->m_surface;
    if (chanB == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 0;
    t.m_10 = a1;
    t.m_04 = (i32)chanA;
    t.m_08 = (i32)chanB;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    if (g_fxDirectGate != 0) {
        Utils::WinAPI::ActiveWait(a2);
        m_resChain->m_worker->m_frontPair->m_surface->Blt(chanB);
    } else {
        f->RunFade(a2, a3, 0);
    }
    mgr->Remove(f);
    return 1;
}

// ---------------------------------------------------------------------------
// 0xfaa60: single-channel type-3 emitter (3 args).
RVA(0x000faa60, 0xed)
i32 CSoundFxEmitter::Method_faa60(i32 a1, i32 a2, i32 a3) {
    CFaderMgr* mgr = m_faderMgr;
    if (mgr == 0) {
        return 0;
    }
    if (m_resChain->m_gate == 0) {
        return 0;
    }
    CDDSurface* chan = m_resChain->m_worker->m_frontPair->m_surface;
    if (chan == 0) {
        return 0;
    }

    CFxModeT3 t;
    t.m_0c = 1;
    t.m_10 = a1;
    t.m_04 = (i32)chan;
    t.m_08 = 0;
    CFader* f = mgr->Add(2, (CFader*)&t);
    if (f == 0) {
        return 0;
    }

    m_gameMgr->StopBankIfActive();
    if (g_fxDirectGate != 0) {
        Utils::WinAPI::ActiveWait(a2);
        m_resChain->m_worker->m_frontPair->m_surface->Fill(0);
    } else {
        f->RunFade(a2, a3, 0);
    }
    m_gameMgr->StopBank0IfActive();
    mgr->Remove(f);
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CSoundFxEmitter);
SIZE_UNKNOWN(CDDrawSurfacePair);
SIZE_UNKNOWN(FxResource);
