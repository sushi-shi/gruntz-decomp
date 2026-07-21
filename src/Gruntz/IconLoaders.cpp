#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GameRegistry.h> // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/String.h>
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup + CDDrawGroupNode shape
#include <Gruntz/UserLogic.h>         // CGameObject (the created sprite) + AnimWorkerObj
#include <Gruntz/PickupType.h>        // the shared object/pickup/grunt-kind type id space
#include <Bute/ButeMgr.h>

#include <stdlib.h> // rand (reloc-masked CRT PRNG)

#include <Gruntz/GameMode.h> // CBootyState - the REAL owner of BuildBootyPerfectAnimation

RVA(0x0001c070, 0x59)
i32 CBootyState::BuildBootyPerfectAnimation() {
    CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup
                           ->CreateSprite(0, static_cast<i32>(0xffffff7e), 0xf0, 0x64, "SimpleAnimation", 3);
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
// from src/Stub/Backlog.cpp. The "registry" object (arg1=esi) IS a real MFC
// ::CString (FID: the seed is ??4CString@@QAEABV0@PBD@Z @0x1b9e74 operator= and
// the append is ??YCString@@QAEABV0@PBD@Z @0x1ba0c8 operator+=) - the fn builds
// the "GAME_INGAMEICONZ<KEY>" lookup key in the caller's CString. The ex
// "PowerupKeyRegistry" view of it is dissolved.
// @early-stop
// jump-table-data-overlap scoring wall (~53%): code bytes byte-exact vs retail
// (verified by raw byte-compare; only the index-table/jump-table base reloc
// operands + $L labels differ). See docs/patterns/jumptable-data-overlap.md.
RVA(0x0001e720, 0x2fe)
void __stdcall BuildPowerupIconKeys(CString* reg, i32 key) {
    *reg = "GAME_INGAMEICONZ";
    switch (key) {
        case PICKUP_BOMB:
            *reg += "TOOLZ_BOMBZ";
            return;
        case PICKUP_BOOMERANG:
            *reg += "TOOLZ_BOOMERANGZ";
            return;
        case PICKUP_BRICK:
            *reg += "TOOLZ_BRICKZ";
            return;
        case PICKUP_CLUB:
            *reg += "TOOLZ_CLUBZ";
            return;
        case PICKUP_GAUNTLETZ:
            *reg += "TOOLZ_GAUNTLETZ";
            return;
        case PICKUP_GLOVEZ:
            *reg += "TOOLZ_GLOVEZ";
            return;
        case PICKUP_GOOBER:
            *reg += "TOOLZ_GOOBERZ";
            return;
        case PICKUP_GRAVITYBOOTZ:
            *reg += "TOOLZ_GRAVITYBOOTZ";
            return;
        case PICKUP_GUNHAT:
            *reg += "TOOLZ_GUNHATZ";
            return;
        case PICKUP_NERFGUN:
            *reg += "TOOLZ_NERFGUNZ";
            return;
        case PICKUP_ROCK:
            *reg += "TOOLZ_ROCKZ";
            return;
        case PICKUP_SHIELD:
            *reg += "TOOLZ_SHIELDZ";
            return;
        case PICKUP_SHOVEL:
            *reg += "TOOLZ_SHOVELZ";
            return;
        case PICKUP_SPRING:
            *reg += "TOOLZ_SPRINGZ";
            return;
        case PICKUP_SPY:
            *reg += "TOOLZ_SPYZ";
            return;
        case PICKUP_SWORD:
            *reg += "TOOLZ_SWORDZ";
            return;
        case PICKUP_TIMEBOMB:
            *reg += "TOOLZ_TIMEBOMBZ";
            return;
        case PICKUP_TOOB:
            *reg += "TOOLZ_TOOBZ";
            return;
        case PICKUP_WAND:
            *reg += "TOOLZ_WANDZ";
            return;
        case PICKUP_WARPSTONE:
            *reg += "TOOLZ_WARPSTONEZ1";
            return;
        case PICKUP_WELDER:
            *reg += "TOOLZ_WELDERZ";
            return;
        case PICKUP_WINGZ:
            *reg += "TOOLZ_WINGZ";
            return;
        case PICKUP_BABYWALKER:
            *reg += "TOYZ_BABYWALKERZ";
            return;
        case PICKUP_BEACHBALL:
            *reg += "TOYZ_BEACHBALLZ";
            return;
        case PICKUP_BIGWHEEL:
            *reg += "TOYZ_BIGWHEELZ";
            return;
        case PICKUP_GOKART:
            *reg += "TOYZ_GOKARTZ";
            return;
        case PICKUP_JACKINTHEBOX:
            *reg += "TOYZ_JACKINTHEBOXZ";
            return;
        case PICKUP_JUMPROPE:
            *reg += "TOYZ_JUMPROPEZ";
            return;
        case PICKUP_POGOSTICK:
            *reg += "TOYZ_POGOSTICKZ";
            return;
        case PICKUP_SCROLL:
            *reg += "TOYZ_SCROLLZ";
            return;
        case PICKUP_SQUEAKTOY:
            *reg += "TOYZ_SQUEAKTOYZ";
            return;
        case PICKUP_YOYO:
            *reg += "TOYZ_YOYOZ";
            return;
        case PICKUP_MEGAPHONE:
            *reg += "POWERUPZ_MEGAPHONEZ";
            return;
        case PICKUP_GHOST:
            *reg += "POWERUPZ_GHOST";
            return;
        case PICKUP_SUPERSPEED:
            *reg += "POWERUPZ_SUPERSPEED";
            return;
        case PICKUP_INVULNERABILITY:
            *reg += "POWERUPZ_INVULNERABILITY";
            return;
        case PICKUP_CONVERSION:
            *reg += "POWERUPZ_CONVERSION";
            return;
        case PICKUP_DEATHTOUCH:
            *reg += "POWERUPZ_DEATHTOUCH";
            return;
        case PICKUP_ROIDZ:
            *reg += "POWERUPZ_ROIDZ";
            return;
        case PICKUP_REACTIVEARMOR:
            *reg += "POWERUPZ_REACTIVEARMOR";
            return;
        case PICKUP_RANDOMCOLORZ:
            *reg += "POWERUPZ_RANDOMCOLORZ";
            return;
        case PICKUP_SCREENSHAKE:
            *reg += "POWERUPZ_SCREENSHAKE";
            return;
        case PICKUP_BLACKSCREEN:
            *reg += "POWERUPZ_BLACKSCREEN";
            return;
        case PICKUP_MINICAM:
            *reg += "POWERUPZ_MINICAM";
            return;
        default:
            *reg += "POWERUPZ_COIN";
            return;
    }
}
