#include <rva.h>

#include <Gruntz/GameText.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/Attract.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/Play.h>
#include <Gruntz/WarlordOwner.h>
#include <Rez/RezSync.h>

static CString g_worldName[8] = {
    "Rocky Roadz",
    "Gruntziclez",
    "Trouble in the Tropicz",
    "High on Sweetz",
    "High Rollerz",
    "Honey, I Shrunk the Gruntz!",
    "The Miniature Masterz",
    "Gruntz in Space",
};

DATA(0x002451a8)
CWinApp g_gruntzWinApp("Gruntz");

DATA(0x00245270)
GruntDeathType g_areaPitDeath;

DATA(0x002453d8)
CButeMgr g_buteMgr;

DATA(0x00245508)
i32 g_panMinX;
DATA(0x0024550c)
i32 g_panMaxX;

DATA(0x00245524)
CString g_brickText1;

DATA(0x00245528)
CString g_brickText2;

DATA(0x0024552c)
CString g_str64552c;

DATA(0x00245530)
CString g_str645530;

DATA(0x00245514)
CString g_str645514;

DATA(0x00245518)
CString g_str645518;

DATA(0x0024551c)
CString g_str64551c;

DATA(0x00245520)
CString g_str645520;

DATA(0x00245534)
i32 g_attractStateCount = 0;
DATA(0x00245538)
i32 g_dlgVal_645538;
DATA(0x0024553c)
GruntDeathType g_areaHazardDeath = DEATH_DROP;

DATA(0x00245540)
FreeNodePool g_coordPool;

static CString g_statLabel[8] = {
    "Time:",
    "Survivorz:",
    "Deathz:",
    "Toolz:",
    "Toyz:",
    "Powerupz:",
    "Coinz:",
    "Secretz:",
};

RVA(0x0001ec20, 0xa0)
CString CMultiBootyState::GetWarlordName(i32 id) {
    switch (static_cast<WarlordOwner>(id)) {
        case WARLORDZ_KING:
            return CString("KING");
        case WARLORDZ_NAPOLEAN:
            return CString("NAPOLEAN");
        case WARLORDZ_PATTON:
            return CString("PATTON");
        case WARLORDZ_VIKING:
            return CString("VIKING");
        default:
            return CString("");
    }
}
