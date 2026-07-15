#include <rva.h>
#include <Gruntz/GameRegistry.h> // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/String.h>
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory + CSpriteListNode shape
#include <Gruntz/UserLogic.h>     // CGameObject (the created sprite) + AnimWorkerObj
#include <Gruntz/PickupType.h>    // the shared object/pickup/grunt-kind type id space
#include <Bute/ButeMgr.h>

#include <stdlib.h> // rand (reloc-masked CRT PRNG)
// IconLoaders.cpp - the in-game-icon / powerup / explosion / camera / booty-perfect
// sprite loaders (C:\Proj\Gruntz). Each builds a named sprite-set key, asks the
// global HUD sprite factory (g_gameReg->m_world->m_8->CreateSprite) for the sprite,
// then caches/forwards something off it through the shared sprite-resource leaves
// (CGruntSprite::CacheFirstFrame, CGruntAnimPlayer::ApplyLookupGeometry - both in
// the spriteresource unit, reloc-masked). Only offsets / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.

// ---------------------------------------------------------------------------
// Shared engine objects, modeled minimally (mirroring SpriteLoaders.cpp's idiom).
// ---------------------------------------------------------------------------
// CSpriteFactory + CSpriteListNode live in the shared <Gruntz/SpriteFactory.h>; the
// created sprite is the shared CGameObject (<Gruntz/UserLogic.h>, included above).

// The four sprite loaders formerly here (LoadCameraSprite 0x78960,
// LoadToyBoxIcon 0x7a3f0, LoadExplosionSprites 0x7b330, LoadPowerupIconSprites
// 0x7c620) are merged into src/Gruntz/TriggerMgr.cpp - the iconloaders unit's
// fns in [0x77f80..0x7d7ca] belong to that ONE original TU (dossier 10b,
// docs/exe-map/interval-dossiers.md). The EngineLabelBacklog holder view they
// share now lives in <Gruntz/IconLoaderViews.h>.
#include <Gruntz/IconLoaderViews.h>
#include <Gruntz/GameMode.h> // CBootyState - the REAL owner of BuildBootyPerfectAnimation

// The game-manager singleton (g_gameReg, *0x64556c), canonical view.
extern "C" CGameRegistry* g_gameReg;

// The attribute manager (butemgr unit): g_buteMgr singleton from <Bute/ButeMgr.h>.

// ===========================================================================
// BuildBootyPerfectAnimation @0x01c070
// ===========================================================================
//
// Builds the "BOOTY_PERFECT" celebration animation: CreateSprite a SimpleAnimation
// at a fixed screen position, cache its first frame, then resolve its cycle
// geometry. __thiscall (this @ esi). Stores the sprite at this+0x2f8.

RVA(0x0001c070, 0x59)
i32 CBootyState::BuildBootyPerfectAnimation() {
    CGameObject* spr =
        g_gameReg->m_world->m_8->CreateSprite(0, (i32)0xffffff7e, 0xf0, 0x64, "SimpleAnimation", 3);
    m_bootyPerfectSprite = spr;
    if (!spr) {
        return 0;
    }
    spr->ApplyName("BOOTY_PERFECT");
    m_bootyPerfectSprite->ApplyLookupGeometry("GAME_CYCLE100", 0);
    return 1;
}

// ---------------------------------------------------------------------------
// BuildPowerupIconKeys (0x1e720) - seeds the "GAME_INGAMEICONZ" icon-key group,
// then appends one tool/toy/powerup key string selected by `key` (a sparse switch;
// out-of-range / gap keys fall through to POWERUPZ_COIN). Sibling of the
// LoadPowerupIconSprites loader above (same GAME_INGAMEICONZ subsystem); re-homed
// from src/Stub/Backlog.cpp. The registry object (arg1=esi): SetGroup() seeds the
// group key, Add() appends one entry (both __thiscall, reloc-masked externals).
// @early-stop
// jump-table-data-overlap scoring wall (~53%): code bytes byte-exact vs retail
// (verified by raw byte-compare; only the index-table/jump-table base reloc
// operands + $L labels differ). See docs/patterns/jumptable-data-overlap.md.
SIZE_UNKNOWN(PowerupKeyRegistry);
class PowerupKeyRegistry {
public:
    void SetGroup(char* key); // FUN_001b9e74 __thiscall
    void Add(char* key);      // FUN_001ba0c8 __thiscall
};

RVA(0x0001e720, 0x2fe)
void __stdcall BuildPowerupIconKeys(PowerupKeyRegistry* reg, i32 key) {
    reg->SetGroup("GAME_INGAMEICONZ");
    switch (key) {
        case 1:
            reg->Add("TOOLZ_BOMBZ");
            return;
        case 2:
            reg->Add("TOOLZ_BOOMERANGZ");
            return;
        case 3:
            reg->Add("TOOLZ_BRICKZ");
            return;
        case 4:
            reg->Add("TOOLZ_CLUBZ");
            return;
        case 5:
            reg->Add("TOOLZ_GAUNTLETZ");
            return;
        case 6:
            reg->Add("TOOLZ_GLOVEZ");
            return;
        case 7:
            reg->Add("TOOLZ_GOOBERZ");
            return;
        case 8:
            reg->Add("TOOLZ_GRAVITYBOOTZ");
            return;
        case 9:
            reg->Add("TOOLZ_GUNHATZ");
            return;
        case 10:
            reg->Add("TOOLZ_NERFGUNZ");
            return;
        case 11:
            reg->Add("TOOLZ_ROCKZ");
            return;
        case 12:
            reg->Add("TOOLZ_SHIELDZ");
            return;
        case 13:
            reg->Add("TOOLZ_SHOVELZ");
            return;
        case 14:
            reg->Add("TOOLZ_SPRINGZ");
            return;
        case 15:
            reg->Add("TOOLZ_SPYZ");
            return;
        case 16:
            reg->Add("TOOLZ_SWORDZ");
            return;
        case 17:
            reg->Add("TOOLZ_TIMEBOMBZ");
            return;
        case 18:
            reg->Add("TOOLZ_TOOBZ");
            return;
        case 19:
            reg->Add("TOOLZ_WANDZ");
            return;
        case 20:
            reg->Add("TOOLZ_WARPSTONEZ1");
            return;
        case 21:
            reg->Add("TOOLZ_WELDERZ");
            return;
        case 22:
            reg->Add("TOOLZ_WINGZ");
            return;
        case 23:
            reg->Add("TOYZ_BABYWALKERZ");
            return;
        case 24:
            reg->Add("TOYZ_BEACHBALLZ");
            return;
        case 25:
            reg->Add("TOYZ_BIGWHEELZ");
            return;
        case 26:
            reg->Add("TOYZ_GOKARTZ");
            return;
        case 27:
            reg->Add("TOYZ_JACKINTHEBOXZ");
            return;
        case 28:
            reg->Add("TOYZ_JUMPROPEZ");
            return;
        case 29:
            reg->Add("TOYZ_POGOSTICKZ");
            return;
        case 30:
            reg->Add("TOYZ_SCROLLZ");
            return;
        case 31:
            reg->Add("TOYZ_SQUEAKTOYZ");
            return;
        case 32:
            reg->Add("TOYZ_YOYOZ");
            return;
        case 50:
            reg->Add("POWERUPZ_MEGAPHONEZ");
            return;
        case 54:
            reg->Add("POWERUPZ_GHOST");
            return;
        case 55:
            reg->Add("POWERUPZ_SUPERSPEED");
            return;
        case 56:
            reg->Add("POWERUPZ_INVULNERABILITY");
            return;
        case 57:
            reg->Add("POWERUPZ_CONVERSION");
            return;
        case 58:
            reg->Add("POWERUPZ_DEATHTOUCH");
            return;
        case 59:
            reg->Add("POWERUPZ_ROIDZ");
            return;
        case 60:
            reg->Add("POWERUPZ_REACTIVEARMOR");
            return;
        case 61:
            reg->Add("POWERUPZ_RANDOMCOLORZ");
            return;
        case 62:
            reg->Add("POWERUPZ_SCREENSHAKE");
            return;
        case 63:
            reg->Add("POWERUPZ_BLACKSCREEN");
            return;
        case 64:
            reg->Add("POWERUPZ_MINICAM");
            return;
        default:
            reg->Add("POWERUPZ_COIN");
            return;
    }
}
