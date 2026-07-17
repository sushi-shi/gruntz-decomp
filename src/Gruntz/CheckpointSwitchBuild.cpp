// CheckpointSwitchBuild.cpp - CCheckpointTriggerSwitchLogic slot-1 builder
// (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp (mis-labeled there as
// CStatzTabSmall::BuildSmall).
//
// Vtable-proven: 0x112a50 is slot 1 (+0x4) of ??_7CCheckpointTriggerSwitchLogic@@6B@,
// adjacent to the class ctor at 0x1127f0.
//
// THE LOCAL VIEW IS GONE (2026-07-13). It used to derive from the real
// CTileTriggerSwitchLogic and then RE-DECLARE the base's fields on top of it
// (m_pad04 / m_20 / m_pad24 / m_2c), which appended them AFTER the 0xcc base: sizeof blew
// out to 0x154 and every access in BuildSmall moved by 0xc8 (m_20 -> +0xe8, the rect ->
// +0xf4). Retail says otherwise, in the bytes:
//     mov ecx,[eax+0x20]   <- the BASE m_20 gate
//     lea edi,[eax+0x2c]   <- the BASE m_block
//     mov ecx,0x18 ; rep movsd
// so the "CStatzRect60" IS m_block[24] (24 dwords at +0x2c), the class adds NO data, and
// sizeof = 0x8c - exactly what the allocation site pushes. All three reasons the old note
// gave for keeping the view are dead: (1) the base's slot-1 virtual carries BuildSmall's
// signature (that is how it is recovered - `sema class` says slot 1, origin
// CTileTriggerSwitchLogic); (2) the +0x2c region is NOT heterogeneous - both spellings are
// the same 24 dwords; (3) the "g_gameReg collision" cannot have been real - this file
// already includes <Gruntz/TileTriggerSwitchLogic.h>.
#include <string.h>                   // memcpy -> the /Oi `rep movsd` that copies rect into m_block
#include <Gruntz/GameRegPtr.h>
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created sprite) + AnimWorkerObj
#include <rva.h>
#include <Gruntz/TileTriggerSwitchLogic.h>

// (CStatzRect60 is gone: the 0x60 block is the base's m_block[24] at +0x2c.)
// The factory (m_world->m_childGroup) is the canonical CDDrawChildGroup (<Gruntz/SpriteFactory.h>);
// CreateSprite @0x1597b0 returns the created CGameObject (ApplyLookupSprite @0x1504d0
// configures it; the +0x7c AnimWorkerObj Init runs post-create; m_layer gates success).
// g_statzGameReg was a SECOND NAME for g_gameReg (0x24556c the game registry) - same
// address, so nothing ever defined it. Unified onto the canonical CGameRegistry; the
// dead CStatzFactoryHolder/CStatzGameReg local views (m_world->m_childGroup == the canonical
// CDDrawChildGroup) are dissolved.
// (g_statzTabSpriteName @0x20aa34 / g_statzTabCfgTag @0x20f928 were FICTIONS: invented
// names for cl's own folded `??_C@` string-literal COMDATs. Both rvas are the pooled
// literal, not a named char[] global -- proven below. They are now spelled inline.)

// The class itself now lives in <Gruntz/TileTriggerSwitchLogic.h> (real derived class, no
// data members, sizeof 0x8c). BuildSmall is its slot-1 override; the base's slot-0 "build"
// virtual (Setup) is the 8-arg builder it chains to.
// @early-stop
// tail-merge + prologue-scheduling wall (~62%). The LAYOUT is now byte-correct and verified
// against retail: `mov ecx,[this+0x20]` (the m_20 gate), `lea edi,[this+0x2c]` (m_block) and
// `mov ecx,0x18; rep movsd` all appear on BOTH sides of the objdiff. What is left is not
// source-steerable: retail tail-merges both early `return 0` exits into ONE shared epilogue
// (`jmp`), while our /O2 emits an inline `pop/pop/pop/pop; ret 0x24` at each, and it loads
// the ebx/ebp args inside the prologue push run. See
// docs/patterns/identical-return-epilogue-tailmerge.md.
// (The pre-existing @early-stop blamed the same wall but was measuring WRONG code: the old
// .cpp-local view shadowed the base's fields, so every access here was 0xc8 too high.)
RVA(0x00112a50, 0xdd)
i32 CCheckpointTriggerSwitchLogic::BuildSmall(
    CTileTriggerContainer* owner,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    const i32* rect,
    i32 a7,
    i32 a8,
    i32 a9
) {
    if (m_20 != 0) {
        return 0;
    }
    if (a2 == 4 && rect[0] == 0) {
        return 0;
    }
    memcpy(m_block, rect, sizeof(m_block)); // rep movsd, ecx=0x18 -> this+0x2c
    if (!Setup(owner, a2, a3, a4, a5, a7, a8, a9)) {
        return 0;
    }
    i32 px = (a3 << 5) + 0x10;
    i32 py = (a4 << 5) + 0x10;
    if (a9 == 0) {
        return 1;
    }
    CGameObject* spr =
        g_gameReg->m_world->m_childGroup->CreateSprite(0, px, py, 0, "BehindCandy", 0x40001);
    if (!spr) {
        return 0;
    }
    spr->m_7c->m_notify(spr);
    spr->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", a9);
    if (spr->m_layer == 0) {
        return 0;
    }
    return 1;
}
