// PowerupIcons.cpp - BuildPowerupIconKeys, the powerup-id-to-sprite-name mapper.
// BuildPowerupIconKeys @0x01e720 (766 B) - __stdcall, ret 8 (2 args). Takes a
// CString output reference and a powerup/tool/toy ID (1..64), sets the string
// to "GAME_INGAMEICONZ_" + the specific sprite name for that ID. Uses a dense
// two-level switch/jump-table for dispatch (64 entries, byte-index table +
// dword case-address table). Plain /O2 /MT (no /GX): leaf, no C++ temps/EH.

// The MFC CString (chained via operator= then operator+=). Only the two
// specific methods used here are declared; the exact mangled names are in
// the NAFXCW library and are reached through incremental-link thunks (the
// call displacements are reloc-masked in objdiff).
class CString {
public:
    void operator=(const char *src);      // @0x1b9e74 (thiscall, 1 arg)
    void operator+=(const char *src);     // @0x1ba0c8 (thiscall, 1 arg)
};

// ---------------------------------------------------------------------------
// BuildPowerupIconKeys  @ RVA 0x01e720  (__stdcall, ret 8 - 2 stack args)
//
// out   - CString& to fill with the icon sprite name
// id    - 1-based powerup/tool/toy ID (1..63 valid; >=64 maps to default)
//
// @address: 0x01e720
// @size:    0x2fe
// ---------------------------------------------------------------------------
void __stdcall BuildPowerupIconKeys(CString &out, int id)
{
    out = "GAME_INGAMEICONZ_";

    switch (id) {
    case 1:  out += "TOOLZ_BOMBZ";          break;
    case 2:  out += "TOOLZ_BOOMERANGZ";     break;
    case 3:  out += "TOOLZ_BRICKZ";         break;
    case 4:  out += "TOOLZ_CLUBZ";          break;
    case 5:  out += "TOOLZ_GAUNTLETZ";      break;
    case 6:  out += "TOOLZ_GLOVEZ";         break;
    case 7:  out += "TOOLZ_GOOBERZ";        break;
    case 8:  out += "TOOLZ_GRAVITYBOOTZ";   break;
    case 9:  out += "TOOLZ_GUNHATZ";        break;
    case 10: out += "TOOLZ_NERFGUNZ";       break;
    case 11: out += "TOOLZ_ROCKZ";          break;
    case 12: out += "TOOLZ_SHIELDZ";        break;
    case 13: out += "TOOLZ_SHOVELZ";        break;
    case 14: out += "TOOLZ_SPRINGZ";        break;
    case 15: out += "TOOLZ_SPYZ";           break;
    case 16: out += "TOOLZ_SWORDZ";         break;
    case 17: out += "TOOLZ_TIMEBOMBZ";      break;
    case 18: out += "TOOLZ_TOOBZ";          break;
    case 19: out += "TOOLZ_WANDZ";          break;
    case 20: out += "TOOLZ_WARPSTONEZ1";    break;
    case 21: out += "TOOLZ_WELDERZ";        break;
    case 22: out += "TOOLZ_WINGZ";          break;
    case 23: out += "TOYZ_BABYWALKERZ";     break;
    case 24: out += "TOYZ_BEACHBALLZ";      break;
    case 25: out += "TOYZ_BIGWHEELZ";       break;
    case 26: out += "TOYZ_GOKARTZ";         break;
    case 27: out += "TOYZ_JACKINTHEBOXZ";   break;
    case 28: out += "TOYZ_JUMPROPEZ";       break;
    case 29: out += "TOYZ_POGOSTICKZ";      break;
    case 30: out += "TOYZ_SCROLLZ";         break;
    case 31: out += "TOYZ_SQUEAKTOYZ";      break;
    case 32: out += "TOYZ_YOYOZ";           break;
    case 33: out += "POWERUPZ_MEGAPHONEZ";  break;
    case 34: out += "POWERUPZ_GHOST";       break;
    case 35: out += "POWERUPZ_SUPERSPEED";  break;
    case 36: out += "POWERUPZ_INVULNERABILITY"; break;
    case 37: out += "POWERUPZ_CONVERSION";  break;
    case 38: out += "POWERUPZ_DEATHTOUCH";  break;
    case 39: out += "POWERUPZ_ROIDZ";       break;
    case 40: out += "POWERUPZ_REACTIVEARMOR"; break;
    case 41: out += "POWERUPZ_RANDOMCOLORZ"; break;
    case 42: out += "POWERUPZ_SCREENSHAKE"; break;
    case 43: out += "POWERUPZ_BLACKSCREEN"; break;
    case 44: out += "POWERUPZ_MINICAM";     break;
    default:
    case 45: out += "POWERUPZ_COIN";        break;
    }
}
