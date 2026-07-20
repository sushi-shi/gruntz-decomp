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
        case 1:
            *reg += "TOOLZ_BOMBZ";
            return;
        case 2:
            *reg += "TOOLZ_BOOMERANGZ";
            return;
        case 3:
            *reg += "TOOLZ_BRICKZ";
            return;
        case 4:
            *reg += "TOOLZ_CLUBZ";
            return;
        case 5:
            *reg += "TOOLZ_GAUNTLETZ";
            return;
        case 6:
            *reg += "TOOLZ_GLOVEZ";
            return;
        case 7:
            *reg += "TOOLZ_GOOBERZ";
            return;
        case 8:
            *reg += "TOOLZ_GRAVITYBOOTZ";
            return;
        case 9:
            *reg += "TOOLZ_GUNHATZ";
            return;
        case 10:
            *reg += "TOOLZ_NERFGUNZ";
            return;
        case 11:
            *reg += "TOOLZ_ROCKZ";
            return;
        case 12:
            *reg += "TOOLZ_SHIELDZ";
            return;
        case 13:
            *reg += "TOOLZ_SHOVELZ";
            return;
        case 14:
            *reg += "TOOLZ_SPRINGZ";
            return;
        case 15:
            *reg += "TOOLZ_SPYZ";
            return;
        case 16:
            *reg += "TOOLZ_SWORDZ";
            return;
        case 17:
            *reg += "TOOLZ_TIMEBOMBZ";
            return;
        case 18:
            *reg += "TOOLZ_TOOBZ";
            return;
        case 19:
            *reg += "TOOLZ_WANDZ";
            return;
        case 20:
            *reg += "TOOLZ_WARPSTONEZ1";
            return;
        case 21:
            *reg += "TOOLZ_WELDERZ";
            return;
        case 22:
            *reg += "TOOLZ_WINGZ";
            return;
        case 23:
            *reg += "TOYZ_BABYWALKERZ";
            return;
        case 24:
            *reg += "TOYZ_BEACHBALLZ";
            return;
        case 25:
            *reg += "TOYZ_BIGWHEELZ";
            return;
        case 26:
            *reg += "TOYZ_GOKARTZ";
            return;
        case 27:
            *reg += "TOYZ_JACKINTHEBOXZ";
            return;
        case 28:
            *reg += "TOYZ_JUMPROPEZ";
            return;
        case 29:
            *reg += "TOYZ_POGOSTICKZ";
            return;
        case 30:
            *reg += "TOYZ_SCROLLZ";
            return;
        case 31:
            *reg += "TOYZ_SQUEAKTOYZ";
            return;
        case 32:
            *reg += "TOYZ_YOYOZ";
            return;
        case 50:
            *reg += "POWERUPZ_MEGAPHONEZ";
            return;
        case 54:
            *reg += "POWERUPZ_GHOST";
            return;
        case 55:
            *reg += "POWERUPZ_SUPERSPEED";
            return;
        case 56:
            *reg += "POWERUPZ_INVULNERABILITY";
            return;
        case 57:
            *reg += "POWERUPZ_CONVERSION";
            return;
        case 58:
            *reg += "POWERUPZ_DEATHTOUCH";
            return;
        case 59:
            *reg += "POWERUPZ_ROIDZ";
            return;
        case 60:
            *reg += "POWERUPZ_REACTIVEARMOR";
            return;
        case 61:
            *reg += "POWERUPZ_RANDOMCOLORZ";
            return;
        case 62:
            *reg += "POWERUPZ_SCREENSHAKE";
            return;
        case 63:
            *reg += "POWERUPZ_BLACKSCREEN";
            return;
        case 64:
            *reg += "POWERUPZ_MINICAM";
            return;
        default:
            *reg += "POWERUPZ_COIN";
            return;
    }
}
