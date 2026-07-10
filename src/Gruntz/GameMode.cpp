// GameMode.cpp - the CState base of the game-state ("mode") hierarchy + the
// CGameModeBase cleanup pair + the free menu/HUD helpers the base drives. The concrete
// leaf states were split into per-class TUs (per-class TU cut of the former god-TU):
//   CMenuState     -> src/Gruntz/MenuState.cpp
//   CCreditsState  -> src/Gruntz/CreditsState.cpp  (+ CCreditzOwner)
//   CBootyState + CMultiBootyState -> src/Gruntz/BootyStateActivate.cpp
// See GameMode.h for the hierarchy. Names are placeholders; only offsets + code bytes
// are load-bearing.
//
// Functions in this TU (ascending retail-RVA order):
//   GenMenuRandPos            @0x019cd0 - free __stdcall edge-spawn RNG helper.
//   CState::LoadGruntEffectSprites @0x01a040 - preload the in-game effect sprite set.
//   LevelMsgHudDriver         @0x01a700 - the level-message HUD + explosion driver (stub).
//   CState::~CState (??_G)     @0x08c710 - the slot-0 scalar-deleting dtor.
//   CState::CState()          @0x08c750 - the base ctor (scalar zero/seed).
//   CGameModeBase::ResetPreview @0x0de140 / ::Reset @0x0f9840 - the base cleanup pair.
#include <Bute/SymTab.h>                   // CSymTab (LoadGruntEffectSprites m_30 ResolvePath)
#include <DDrawMgr/DDrawSubMgrLeafScan.h>  // RemoveKeysEqual_157c70 (CGameModeBase::ResetPreview)
#include <Gruntz/SpriteRefTable.h>         // CSpriteRefTable (LoadGruntEffectSprites m_74 GetSel)
#include <Gruntz/GameMode.h>               // CState / CGameModeBase / CSpriteFactoryHolder
#include <Bute/ButeMgr.h>                  // CButeMgr g_buteMgr (SecretColor wormhole tint)
#include <Gruntz/SpriteFactory.h>          // CSpriteFactory (m_world->m_8 CreateSprite)
#include <Gruntz/UserLogic.h>              // CGameObject (the created effect sprites)
#include <Gruntz/WwdGameReg.h>             // g_gameReg (GenMenuRandPos Rand/RandRange)
#include <rva.h>

// The scalar-deleting dtor's `operator delete` (reached by the synthesized `??_G`);
// declare it so /GX tracks the EH state.
void operator delete(void*);

// The global game registry (canonical <Gruntz/WwdGameReg.h>): the Rand()/RandRange()
// __thiscall helpers GenMenuRandPos calls (all reloc-masked).
extern WwdGameReg* g_gameReg; // ?g_gameReg@@3PAUWwdGameReg@@A (reloc-masked)

// The glitter/effect factory chain (CGlitterMgr view of g_mgrSettings, *0x24556c): the
// selection source (m_74), the SecretColor->handle table (m_78), the sprite factory
// (m_world->m_8), the letter-count set (m_7c), the attract frame counter (m_80).
SIZE_UNKNOWN(CGlitterMgrM30);
struct CGlitterMgrM30 {
    char m_pad00[0x8];
    CSpriteFactory* m_8; // +0x08 the animation factory (CreateSprite @0x1597b0)
};
SIZE_UNKNOWN(CGlitterMgrSet);
struct CGlitterMgrSet {
    char m_pad00[0x4];
    i32 m_4; // +0x04 element count
};
SIZE_UNKNOWN(CGlitterColorTable);
struct CGlitterColorTable {
    char m_pad00[0x14];
    i32 m_arr14[1]; // +0x14  color->handle table
};
SIZE_UNKNOWN(CGlitterMgr);
struct CGlitterMgr {
    char m_pad00[0x30];
    CGlitterMgrM30* m_world; // +0x30
    char m_pad34[0x74 - 0x34];
    CSpriteRefTable* m_74;    // +0x74  selection source
    CGlitterColorTable* m_78; // +0x78  color->handle table
    CGlitterMgrSet* m_7c;     // +0x7c
    i32 m_80;                 // +0x80  attract frame counter (title rotation source)
};
DATA(0x0024556c)
extern "C" CGlitterMgr* g_mgrSettings;

// LoadGruntEffectSprites externs: the CButeMgr text-config singleton + the wormhole
// SecretColor bute tag + the go-kart install byte flag.
extern CButeMgr g_buteMgr;        // ?g_buteMgr@@3VCButeMgr@@A
extern char* g_wormholeSpawnKey;  // ?g_wormholeSpawnKey@@3PADA ("Wormhole" bute tag @0x60a7ac)
extern unsigned char g_dat60b588; // ?g_dat60b588@@3EA  (go-kart install byte flag)

// The geometry table (0x60b8fc, 0x10-byte rows): the effect sprites' x-position is
// (row.a + row.c) / 2. The loop init/bound relocs land on &row[0].c / &row[8].c.
SIZE_UNKNOWN(CEffGeomRow);
struct CEffGeomRow {
    i32 a;     // +0x00
    i32 pad4;  // +0x04 (417, unused)
    i32 c;     // +0x08
    i32 pad12; // +0x0c (245, unused)
};
DATA(0x0020b8fc)
extern CEffGeomRow g_effGeom[8]; // 0x60b8fc

// Typed self-view for the in-game effect loader. The +0x0c view is the shared
// CSpriteFactoryHolder; the effect-sprite members (m_224/m_2fc.. arrays) are reached by
// offset (a documented offset-reuse ambiguity vs CPlay's own layout).
SIZE_UNKNOWN(CEffLoaderSelf);
struct CEffLoaderSelf {
    char m_pad00[0xc];
    CSpriteFactoryHolder* m_c; // +0x0c  the shared view/render/resource context
    char m_pad10[0x30 - 0x10];
    CSymTab* m_30; // +0x30  image namespace
    char m_pad34[0x224 - 0x34];
    CGameObject* m_bomb[8];   // +0x224  bomb-grunt sprites
    CGameObject* m_gokart[8]; // +0x244  go-kart sprites
    CGameObject* m_expl[8];   // +0x264  explosion sprites
    char m_pad284[0x2fc - 0x284];
    CGameObject* m_2fc; // +0x2fc  stopwatch
    CGameObject* m_300; // +0x300  exit
    CGameObject* m_304; // +0x304  death twitch
    CGameObject* m_308; // +0x308  gauntletz
    CGameObject* m_30c; // +0x30c  beachballz
    CGameObject* m_310; // +0x310  roidz
    CGameObject* m_314; // +0x314  coin
    CGameObject* m_318; // +0x318  wormhole/teleporter
};

// ===========================================================================
// GenMenuRandPos (0x19cd0): a free __stdcall helper (no `this`). Generates a random
// {x,y} spawn position by edge, selected by `sel` (1..8). Rand() = signed game RNG;
// RandRange(0,N) = uniform [0,N).
// @early-stop
// regalloc coin-flip wall (~89%): all 8 cases + shared tails + idiv constants are
// byte-identical; the sole residual is outX/outY swapped between esi/edi (retail
// outX->edi/outY->esi, recompile outX->esi/outY->edi). A named-local pin
// (docs/patterns/pin-local-for-callee-saved-reg.md) did NOT flip it -> the pure
// allocator coin-flip that doc flags as the zero-register-pinning.md wall.
// ===========================================================================
RVA(0x00019cd0, 0x1df)
void __stdcall GenMenuRandPos(i32 sel, i32* outX, i32* outY) {
    if (!outX || !outY) {
        return;
    }
    switch (sel) {
        case 1:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = 0x1e0;
            return;
        case 5:
            *outX = g_gameReg->Rand() % 0x281;
            *outY = 0;
            return;
        case 3:
            *outX = 0;
            goto y_1e1;
        case 7:
            *outX = 0x280;
            goto y_1e1;
        y_1e1:
            *outY = g_gameReg->Rand() % 0x1e1;
            return;
        case 2:
            if (g_gameReg->Rand() % 2) {
                *outX = 0;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141;
            *outY = 0x1e0;
            return;
        case 8:
            if (g_gameReg->Rand() % 2) {
                *outX = 0x280;
                goto y_f1;
            }
            *outX = g_gameReg->Rand() % 0x141 + 0x140;
            *outY = 0x1e0;
            return;
        y_f1:
            *outY = g_gameReg->Rand() % 0xf1 + 0xf0;
            return;
        case 4:
            if (g_gameReg->Rand() % 2) {
                *outX = g_gameReg->RandRange(0, 0x140);
                *outY = 0;
                return;
            }
            *outX = 0;
            goto y_f0;
        case 6:
            if (g_gameReg->RandRange(0, 1)) {
                *outX = g_gameReg->RandRange(0, 0x140) + 0x140;
                *outY = 0;
                return;
            }
            *outX = 0x280;
            goto y_f0;
        y_f0:
            *outY = g_gameReg->RandRange(0, 0xf0);
            return;
    }
}

// ===========================================================================
// CState::LoadGruntEffectSprites (0x1a040): preload the in-game effect/icon animation
// set. Really a CPlay-layout method (the trace homed it on the CState base); it walks
// the g_mgrSettings->m_world->m_8 SimpleAnimation factory and stores ~15 named effect
// sprites into the +0x2fc.. block plus three parallel 8-element sprite arrays at
// +0x224/+0x244/+0x264, positioned from the geometry table.
// @confidence: med
// @source: string-xref
// @early-stop
// ~96.3%: complete + correct, dev-authentic shape (natural array indexing throughout).
// Residual is two scheduling walls: (1) the SecretColor block schedules the
// g_mgrSettings->m_78 load AFTER the GetIntDef call (retail hoists it before); (2) the
// (a+c)/2 geom pair loads a/c in the opposite eax/edx order (commutative). All
// externs/strings named.
// ===========================================================================
RVA(0x0001a040, 0x55e)
i32 CState::LoadGruntEffectSprites() {
    CEffLoaderSelf* self = (CEffLoaderSelf*)this;

    i32 handleA = g_mgrSettings->m_74->GetSel(0, 0);
    if (handleA == 0) {
        return 0;
    }
    i32 handleB = g_mgrSettings->m_74->GetSel(0, 1);

    void* img = self->m_30->ResolvePath("IMAGEZ_GOKARTGRUNT");
    if (img == 0) {
        return 0;
    }
    self->m_c->m_10->Install(img, "GRUNTZ_GOKARTGRUNT", (const char*)&g_dat60b588);

    CSpriteFactory* f = g_mgrSettings->m_world->m_8;

    CGameObject* sw = f->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_2fc = sw;
    if (sw == 0) {
        return 0;
    }
    sw->ApplyName("GAME_INGAMEICONZ_POWERUPZ_STOPWATCH");
    self->m_2fc->ApplyLookupGeometry("GAME_CYCLE100", 0);
    self->m_2fc->m_stateFlags |= 1;

    CGameObject* wh = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_318 = wh;
    if (wh == 0) {
        return 0;
    }
    i32 tint =
        g_mgrSettings->m_78->m_arr14[g_buteMgr.GetIntDef(g_wormholeSpawnKey, "SecretColor", 1)];
    self->m_318->ApplyName("GAME_WORMHOLE");
    self->m_318->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    CGameObject* p318 = self->m_318;
    p318->m_drawActive = 1;
    p318->m_drawFillCmd = 7;
    p318->m_drawFillArg = tint;

    CGameObject* ex = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_300 = ex;
    if (ex == 0) {
        return 0;
    }
    ex->ApplyName("GRUNTZ_EXITZ");
    self->m_300->ApplyLookupGeometry("GAME_GRUNTFLEX", 0);
    CGameObject* p300 = self->m_300;
    p300->m_drawActive = 1;
    p300->m_drawFillCmd = 0xa;
    p300->m_drawFillArg = handleA;
    self->m_300->m_stateFlags |= 1;

    CGameObject* dt = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_304 = dt;
    if (dt == 0) {
        return 0;
    }
    dt->ApplyName("GRUNTZ_NORMALGRUNT_DEATH");
    self->m_304->ApplyLookupGeometry("GAME_GRUNTTWITCH", 0);
    CGameObject* p304 = self->m_304;
    p304->m_drawActive = 1;
    p304->m_drawFillCmd = 0xa;
    p304->m_drawFillArg = handleA;
    self->m_304->m_stateFlags |= 1;

    CGameObject* gl = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_308 = gl;
    if (gl == 0) {
        return 0;
    }
    gl->ApplyName("GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ");
    self->m_308->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p308 = self->m_308;
    p308->m_drawActive = 1;
    p308->m_drawFillCmd = 0xa;
    p308->m_drawFillArg = handleA;
    self->m_308->m_stateFlags |= 1;

    CGameObject* bb = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_30c = bb;
    if (bb == 0) {
        return 0;
    }
    bb->ApplyName("GAME_INGAMEICONZ_TOYZ_BEACHBALLZ");
    self->m_30c->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p30c = self->m_30c;
    p30c->m_drawActive = 1;
    p30c->m_drawFillCmd = 0xa;
    p30c->m_drawFillArg = handleA;
    self->m_30c->m_stateFlags |= 1;

    CGameObject* rz = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_310 = rz;
    if (rz == 0) {
        return 0;
    }
    rz->ApplyName("GAME_INGAMEICONZ_POWERUPZ_ROIDZ");
    self->m_310->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p310 = self->m_310;
    p310->m_drawActive = 1;
    p310->m_drawFillCmd = 0xa;
    p310->m_drawFillArg = handleA;
    self->m_310->m_stateFlags |= 1;

    CGameObject* cn = g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 0, "SimpleAnimation", 3);
    self->m_314 = cn;
    if (cn == 0) {
        return 0;
    }
    cn->ApplyName("GAME_INGAMEICONZ_POWERUPZ_COIN");
    self->m_314->ApplyLookupGeometry("GAME_CYCLE100", 0);
    CGameObject* p314 = self->m_314;
    p314->m_drawActive = 1;
    p314->m_drawFillCmd = 0xa;
    p314->m_drawFillArg = handleA;
    self->m_314->m_stateFlags |= 1;

    // The three per-direction sprite arrays sit contiguously (bomb/go-kart/explosion),
    // positioned from the geometry table row's {a,c} midpoint; MSVC fuses the three
    // parallel array walks + the geom walk into single induction pointers.
    for (i32 i = 0; i < 8; i++) {
        CGameObject* b =
            g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        self->m_bomb[i] = b;
        if (b == 0) {
            return 0;
        }
        b->ApplyName("GRUNTZ_BOMBGRUNT_WEST_ITEM");
        self->m_bomb[i]->ApplyLookupGeometry("GAME_GRUNTBOMBSPRINT", 0);
        CGameObject* bp = self->m_bomb[i];
        bp->m_drawActive = 1;
        bp->m_drawFillCmd = 0xa;
        bp->m_drawFillArg = handleA;
        self->m_bomb[i]->m_screenX = 0x2c6;
        self->m_bomb[i]->m_screenY = (g_effGeom[i].a + g_effGeom[i].c) / 2;
        self->m_bomb[i]->m_stateFlags |= 1;

        CGameObject* e =
            g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        self->m_expl[i] = e;
        if (e == 0) {
            return 0;
        }
        e->ApplyName("GAME_EXPLOSION");
        self->m_expl[i]->m_stateFlags |= 1;

        CGameObject* g =
            g_mgrSettings->m_world->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        self->m_gokart[i] = g;
        if (g == 0) {
            return 0;
        }
        g->ApplyName("GRUNTZ_GOKARTGRUNT_EAST");
        self->m_gokart[i]->ApplyLookupGeometry("GAME_CYCLE100", 0);
        CGameObject* gp = self->m_gokart[i];
        gp->m_drawActive = 1;
        gp->m_drawFillCmd = 0xa;
        gp->m_drawFillArg = handleB;
        self->m_gokart[i]->m_screenX = -70;
        self->m_gokart[i]->m_screenY = (g_effGeom[i].a + g_effGeom[i].c) / 2;
        self->m_gokart[i]->m_stateFlags |= 1;
    }
    return 1;
}

// @confidence: low
// @source: winapi:CopyRect
// @early-stop
// LevelMsgHudDriver @0x1a700 - the level-message HUD + explosion eye-candy driver (GAME
// code, 1718 B). Deferred to the leaf-first final sweep: a >512B body over the CString
// array + sprite-create + sound callee set; a partial under-counts AND diverges its
// regalloc, so the return-0 normalization artifact is kept per the >512B REVERT rule.
RVA(0x0001a700, 0x6b6)
i32 LevelMsgHudDriver() {
    return 0;
}

// ===========================================================================
// CState - the base game-state class.
// ===========================================================================

// CState::CState(): store the vftable, then zero a flat list of scalar members in
// source-declaration order, seeding four time/budget fields to 0x40. NO embedded
// sub-object ctors and NO EH frame (plain /O2). This ctor (with the leaf dtors in the
// per-class TUs) anchors the CState vtable + inline-virtual emission.
RVA(0x0008c750, 0xa9)
CState::CState() {
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_levelBank = 0;
    m_2c = 0;
    m_14 = 0;
    m_18 = 0;
    m_38 = 0;
    m_ready = 0;
    m_4c = 0;
    m_24 = 0;
    m_160 = 0;
    m_164 = 0;
    m_168 = 0;
    m_170 = 0x40;
    m_16c = 0;
    m_174 = 0x40;
    m_178 = 0;
    m_180 = 0x40;
    m_17c = 0;
    m_184 = 0x40;
    m_188 = 0;
    m_190 = 0;
    m_18c = 0;
    m_194 = 0;
    m_198 = 0;
    m_1a0 = 0;
    m_19c = 0;
    m_1a4 = 0;
    m_cursorX = 0;
    m_cursorY = 0;
}

// CState::~CState() - the slot-0 scalar-deleting dtor `??_G` (0x8c710). Its body is
// defined INLINE in the header so MSVC folds it into the synth `??_G` thunk; the thunk
// has no source body, so pin its symbol by mangled name here.
// @rva-symbol: ??_GCState@@UAEPAXI@Z 0x0008c710 0x24

// CState::Update (0x0008c4b0) / Render (0x0008c4d0) are inline members in the header.

// The intervening vtable slots (1,2) - out-of-line stubs that anchor the vftable order
// so Update lands at slot 4 (+0x10) and Render at slot 5 (+0x14).
i32 CState::Vfunc1(i32, i32, i32) {
    return 0;
}
void CState::ReleaseResources() {}

// ===========================================================================
// CGameModeBase cleanup pair (the base the game-state classes chain their teardown to).
// Stop the owned sound (SoundStream::Stop), clear/prune the sub-manager map, then
// BaseCleanup. m_c->m_28 is re-read each statement (retail does not cache it).
// ===========================================================================

// 0x0de140 - ResetPreview: prune the PREVIEW-prefixed keys instead of clearing.
// @early-stop
// ~98.8% - m_28-intermediate regalloc wall (retail reuses eax->eax->ecx; cl picks fresh
// ecx/edx) - a 2-3 byte modrm micro-diff, not source-steerable.
extern char s_PREVIEW_6135e8[]; // "PREVIEW" (bound in Globals.cpp; reloc-masked)
RVA(0x000de140, 0x33)
void CGameModeBase::ResetPreview() {
    if (m_c->m_28->m_2c != 0) {
        ((SoundStream*)m_c->m_28->m_2c)->Stop();
    }
    m_c->m_28->RemoveKeysEqual_157c70(s_PREVIEW_6135e8, "_");
    BaseCleanup();
}

// 0x0f9840 - Reset: ClearMap the whole sub-manager map.
// @early-stop
// ~98.7% - same m_28-intermediate regalloc wall as ResetPreview.
RVA(0x000f9840, 0x29)
void CGameModeBase::Reset() {
    if (m_c->m_28->m_2c != 0) {
        ((SoundStream*)m_c->m_28->m_2c)->Stop();
    }
    m_c->m_28->ClearMap();
    BaseCleanup();
}
