// CheckpointSwitchBuild.cpp - CCheckpointTriggerSwitchLogic slot-1 builder
// (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp (mis-labeled there as
// CStatzTabSmall::BuildSmall).
//
// Vtable-proven: 0x112a50 is slot 1 (+0x4) of ??_7CCheckpointTriggerSwitchLogic@@6B@,
// adjacent to the class ctor at 0x1127f0 (TileTriggerDerivedCtors.cpp). Modeled here
// with a local non-polymorphic view (slot 0 = the reloc-masked base builder) so its
// codegen is preserved; the ctor's real polymorphic model lives in the ctor TU. Only
// offsets / code bytes are load-bearing; helpers are reloc-masked externals.
#include <rva.h>

SIZE_UNKNOWN(CStatzRect60);
struct CStatzRect60 {
    i32 d[0x18]; // 0x60 bytes
};
struct CStatzSprite;
SIZE_UNKNOWN(CStatzSpriteFactory);
struct CStatzSpriteFactory {
    // FUN_001597b0 __thiscall, ret 0x18: build the named sprite.
    CStatzSprite* CreateSprite(i32 kind, i32 px, i32 py, i32 hint, void* name, i32 flags);
};
SIZE_UNKNOWN(CStatzFactoryHolder);
struct CStatzFactoryHolder {
    char m_pad0[0x8];
    CStatzSpriteFactory* m_8; // +0x08
};
struct CStatzGameReg {
    char m_pad0[0x30];
    CStatzFactoryHolder* m_world; // +0x30
};
DATA(0x0024556c)
extern CStatzGameReg* g_statzGameReg; // *0x64556c
SIZE_UNKNOWN(CStatzSpriteInitVtbl);
struct CStatzSpriteInitVtbl {
    void* m_slot0[4];                   // slots 0..3
    void(__cdecl* Init)(CStatzSprite*); // slot 4 (+0x10)
};
SIZE_UNKNOWN(CStatzSprite);
struct CStatzSprite {
    char m_pad0[0x7c];
    CStatzSpriteInitVtbl* m_7c; // +0x7c
    char m_pad80[0x198 - 0x80];
    i32 m_198;                        // +0x198
    void Configure(void* tag, i32 a); // FUN_001504d0 __thiscall
};
DATA(0x0020aa34)
extern char g_statzTabSpriteName[]; // CreateSprite name buffer
DATA(0x0020f928)
extern char g_statzTabCfgTag[]; // Configure tag global

SIZE_UNKNOWN(CCheckpointTriggerSwitchLogic);
struct CCheckpointTriggerSwitchLogic {
    virtual i32 BaseBuild(i32, i32, i32, i32, i32, i32, i32, i32); // slot 0 (reloc-masked)
    i32
    BuildSmall(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, CStatzRect60* a6, i32 a7, i32 a8, i32 a9);
    char m_pad04[0x20 - 0x4];
    i32 m_20; // +0x20  already-built gate
    char m_pad24[0x2c - 0x24];
    CStatzRect60 m_2c; // +0x2c  rect block
};

// @early-stop
// regalloc/tail-merge wall (~62%): instruction selection, calls and constants are
// byte-correct, but retail pins arg3->ebp / arg4->ebx via prologue-interleaved
// arg-loads and tail-merges the early `return 0` exits into the shared post-BaseBuild
// epilogue; our /O2 build pins different args + emits inline epilogues, cascading the
// whole register allocation. Logic complete; not source-steerable. See
// docs/patterns/identical-return-epilogue-tailmerge.md + pin-local-for-callee-saved-reg.md.
RVA(0x00112a50, 0xdd)
i32 CCheckpointTriggerSwitchLogic::
    BuildSmall(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, CStatzRect60* a6, i32 a7, i32 a8, i32 a9) {
    if (m_20 != 0) {
        return 0;
    }
    if (a2 == 4 && a6->d[0] == 0) {
        return 0;
    }
    m_2c = *a6;
    if (!BaseBuild(a1, a2, a3, a4, a5, a7, a8, a9)) {
        return 0;
    }
    i32 px = (a3 << 5) + 0x10;
    i32 py = (a4 << 5) + 0x10;
    if (a9 == 0) {
        return 1;
    }
    CStatzSprite* spr =
        g_statzGameReg->m_world->m_8->CreateSprite(0, px, py, 0, g_statzTabSpriteName, 0x40001);
    if (!spr) {
        return 0;
    }
    spr->m_7c->Init(spr);
    spr->Configure(g_statzTabCfgTag, a9);
    if (spr->m_198 == 0) {
        return 0;
    }
    return 1;
}
