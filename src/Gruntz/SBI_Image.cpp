#define SBI_DTOR_CHAIN     // enable the inline base-dtor bodies (see StatusBarItem.h)
#define SBI_OWN_IMAGE_DTOR // this TU supplies the out-of-line ~CSBI_Image (0x100870)
#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SbiConfig.h> // canonical config-host family (one shape)
#include <Gruntz/SBI_Image.h> // canonical frameless CSBI_Image (: CSBI_RectOnly : CStatusBarItem)
#include <DDrawMgr/DDrawWorkerRegistry.h> // AnyValueMatches_155630 (SerializeChain's reverse lookup)
#include <Gruntz/GameRegistry.h>          // canonical g_gameReg singleton
#include <Gruntz/ResMgr.h>        // canonical g_gameReg->m_world view (CResMgr + CImageRegistry)
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Image/CImage.h>         // the resolved frame record (TickRenderCurrent's blit)
#include <string.h>               // strlen / memset (inline repne-scas / rep-stos)
// SBI_Image.cpp - Gruntz CSBI_Image (C:\Proj\Gruntz), the frameless methods.
// RTTI .?AVCSBI_Image@@; in the SBI family
//   CSBI_Image : CSBI_RectOnly : CStatusBarItem  (CSBI_ImageSet derives from this).
// Vtable @0x5eac0c. The /GX chain destructor (0x100870) is defined below - the
// former SBI_ImageEh.cpp companion split is collapsed (retail's one TU was /GX).
//
// These are concrete virtual-slot methods modeled with the SBI family's
// manual-vtable-stamp device (no real `virtual`); sibling/engine callees are
// ILT-reloc-masked.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call through them is reloc-masked).

// The config host + its lookup map + record now come from the shared canonical
// family (<Gruntz/SbiConfig.h>): CSbiConfigHost / CSbiConfigMap / CSbiConfigRecord.

// CSBI_Image (+ its CSBI_RectOnly intermediate) now come from the canonical
// frameless header <Gruntz/SBI_Image.h>. SetupImage is defined below.

// vtable slot 11 (0xe6c80): store the live config args into the base-region
// fields, then (if a key is supplied) look up the config record through the host
// map and latch its value into m_30. Returns whether a non-zero value was latched.
// @early-stop
// ~65% (zero-register-pinning INVERSE wall): logic + every field store/value/guard
// is correct, but with four `== 0` null tests and two `field = 0` stores MSVC5 here
// PINS 0 in edi (extra push edi/pop edi + `cmp edi,reg` everywhere) while retail
// uses `test reg,reg` + immediate `mov [field],0` stores. Documented coin-flip
// regalloc wall (docs/patterns/zero-register-pinning.md, inverse case) - "no
// init-list/assignment/reorder lever flips it". Also a reloc-masked `call Lookup`.
// Deferred to the final sweep.
RVA(0x000e6c80, 0xc3)
i32 CSBI_Image::SetupImage(
    CStatusBarMgr* owner,
    CSbiConfigHost* host,
    i32 a3,
    i32 a4,
    SbRect rc,
    const char* key,
    i32 a10,
    i32 a11
) {
    if (host == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    m_2c = (i32)owner;
    m_10 = a4;
    m_24 = (i32)host;
    m_28 = 0;
    m_4 = 0;
    m_rect14.m_0 = rc.left;
    m_rect14.m_4 = rc.top;
    m_rect14.m_8 = rc.right;
    m_rect14.m_c = rc.bottom;
    m_c = a3;
    if (key == 0) {
        m_30 = 0;
        return 0 != 0;
    }
    CSbiConfigRecord* rec = 0;
    ((CMapStringToPtr*)&host->m_10->m_10map)->Lookup(key, (void*&)rec);
    if (rec == 0 || rec->m_64 > 1 || rec->m_68 < 1) {
        m_30 = 0;
        return 0 != 0;
    }
    i32 val = rec->m_14[1];
    m_30 = val;
    return val != 0;
}

// The g_gameReg singleton (*0x24556c) + the per-serialize round counter.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;
DATA(0x00229ad0)
extern i32 g_serialCounter;

// ---------------------------------------------------------------------------
// CSBI_Image::ClearFrame (0xe6d90): drop the resolved frame. The vtable slot-3 body
// AND the chain-dtor member teardown (one retail body). Re-attributed from
// CSBI_MenuItem (dossier #16: vtbl 0x1eac0c slot [3] thunk 0x1b59).
RVA(0x000e6d90, 0x8)
void CSBI_Image::ClearFrame() {
    m_30 = 0;
}

// ---------------------------------------------------------------------------
// CSBI_Image::TickRenderCurrent (0xe6dd0, vtable slot 5): one play step that renders
// the CURRENT resolved frame (no table re-lookup): while cycles remain, consume one
// and blit m_30 at the rect base + frame anchor. Returns 1. Ex CAniPlayer view.
// @early-stop
// 74% - all operations byte-exact (verified via --diff); residual is pure register
// scheduling: retail keeps `this` in eax to load both rect halves up front and defers
// the g_gameReg->m_world->m_drawTarget surface-context chain to last, where cl loads the
// global mid-sequence. Neither local nor inlined surfaceCtx flips it (RenderFrame
// arg-eval-order/regalloc wall shared by the whole slot-5 render family). Final sweep.
RVA(0x000e6dd0, 0x45)
i32 CSBI_Image::TickRenderCurrent_0e6dd0() {
    if (m_28 > 0) {
        m_28--;
        CImage* cel = (CImage*)m_30;
        if (cel != 0) {
            cel->RenderFrame(
                (void*)(i32)((CResMgr*)g_gameReg->m_world)->m_drawTarget->m_14,
                (void*)(m_rect14.m_0 + cel->m_anchorX),
                (void*)(m_rect14.m_4 + cel->m_anchorY),
                (void*)0
            );
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CSBI_Image::SerializeChain (0xe6e40, vtable slot 1) - the CSBI_Image leg of the
// family serialize: transfer the resolved frame's registry name + index (read ->
// Lookup + frame range probe; write -> reverse name lookup), then tail-chain into
// the base leg (CStatusBarItem::SerializeFields). Re-attributed from CSBI_MenuItem
// (dossier #16: vtbl 0x1eac0c slot [1] thunk 0x2077).
// @early-stop
// ~85% regalloc + stack-packing wall (CTimer::Serialize family): the switch, the
// vtable transfer, the strlen/Lookup/frame-range probe + the memset/reverse-lookup are
// byte-correct. The residual is a register-naming coin-flip (retail pins ar in
// ebx + this in esi; the recompile swaps them) plus the dead-spill-slot packing
// (retail's temps share a slot, shifting the esp+ frame offsets by 4). Deferred.
RVA(0x000e6e40, 0x17c)
i32 CSBI_Image::SerializeChain(void* arP, i32 kind, i32 a, i32 b) {
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
                CObject* r_ob = 0;
                mgr->m_10->m_10map.Lookup(name, r_ob);
                CSprite* r = (CSprite*)r_ob;
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
            // Write leg: reverse-look-up the resolved frame's registry name + index
            // (the registry's reverse name->id helper, 0x155630, on mgr->m_10).
            idx = 0;
            g_serialCounter++;
            memset(name, 0, sizeof(name));
            if (m_30) {
                ((CDDrawWorkerRegistry*)mgr->m_10)
                    ->AnyValueMatches_155630(m_30, (i32)name, (i32)&idx);
            }
            ar->Write(name, 0x80);
            ar->Write(&idx, 4);
            break;
    }
    return SerializeFields(ar, kind, a, b) != 0;
}

// ---------------------------------------------------------------------------
// ~CSBI_Image (0x100870): the /GX chain destructor - stamp ??_7CSBI_Image, run
// ClearFrame (0xe6d90, the slot-3 body above), then MSVC folds the
// two inline base dtors in (??_7CSBI_RectOnly + DtorRect, ??_7CStatusBarItem +
// DtorStatus - the SBI_DTOR_CHAIN device; this TU owns ~CSBI_Image itself via
// SBI_OWN_IMAGE_DTOR) behind the /GX SEH frame with 0/1/-1 trylevels. Collapsed
// from SBI_ImageEh.cpp (3-level case of
// docs/patterns/eh-dtor-multilevel-polymorphic-chain.md).
RVA(0x00100870, 0x6a)
CSBI_Image::~CSBI_Image() {
    ClearFrame();
}
