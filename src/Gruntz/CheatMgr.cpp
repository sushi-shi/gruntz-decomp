#include <rva.h>

#include <Gruntz/CheatMgr.h>

#include <Bute/ButeMgr.h>
#include <EmptyString.h>
#include <Gruntz/GruntzCommandId.h>

#include <stddef.h>

// The built-in developer cheat table. Codes are stored plaintext + 0x3d,
// because CheckCode uppercases the typed string and adds 0x3d before the map
// lookup (@0x23090). The decoded code is on each line; docs/formats/
// game-data-strings.md carries the table and its corroboration.
// clang-format off
DATA(0x0020c838)
char s_cheatWaWa[20] = "\x8a\x8d\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e"; // MPWAWAWAWAWAWA
DATA(0x0020c84c)
char s_cheatWildWacky[16] = "\x8a\x8d\x94\x86\x89\x81\x94\x7e\x80\x88\x96"; // MPWILDWACKY
DATA(0x0020c85c)
char s_cheatBuild[12] = "\x8a\x8d\x7f\x92\x86\x89\x81"; // MPBUILD
DATA(0x0020c868)
char s_cheatDevHeads[16] = "\x8a\x8d\x81\x82\x93\x85\x82\x7e\x81\x90"; // MPDEVHEADS
DATA(0x0020c878)
char s_cheatMonolithBare[12] = "\x8a\x8c\x8b\x8c\x89\x86\x91\x85"; // MONOLITH
DATA(0x0020c884)
char s_cheatMonolith[16] = "\x8a\x8d\x8a\x8c\x8b\x8c\x89\x86\x91\x85"; // MPMONOLITH
DATA(0x0020c894)
char s_cheatLogo[8] = "\x8a\x8d\x89\x8c\x84\x8c"; // MPLOGO
DATA(0x0020c89c)
char s_cheatLith[8] = "\x8a\x8d\x89\x86\x91\x85"; // MPLITH
DATA(0x0020c8a4)
char s_cheatChop[8] = "\x8a\x8d\x80\x85\x8c\x8d"; // MPCHOP
DATA(0x0020c8ac)
char s_cheatScorpio[12] = "\x8a\x8d\x90\x80\x8c\x8f\x8d\x86\x8c"; // MPSCORPIO
DATA(0x0020c8b8)
char s_cheatGoble[12] = "\x8a\x8d\x84\x8c\x7f\x89\x82"; // MPGOBLE
DATA(0x0020c8c4)
char s_cheatLambertian[16] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91\x86\x7e\x8b"; // MPLAMBERTIAN
DATA(0x0020c8d4)
char s_cheatLambert[12] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91"; // MPLAMBERT
DATA(0x0020c8e0)
char s_cheatHologram[16] = "\x8a\x8d\x85\x8c\x89\x8c\x84\x8f\x7e\x8a"; // MPHOLOGRAM
DATA(0x0020c8f0)
char s_cheatStopwatch[16] = "\x8a\x8d\x90\x91\x8c\x8d\x94\x7e\x91\x80\x85"; // MPSTOPWATCH
DATA(0x0020c900)
char s_cheatNoInfo[12] = "\x8a\x8d\x8b\x8c\x86\x8b\x83\x8c"; // MPNOINFO
DATA(0x0020c90c)
char s_cheatObjects[12] = "\x8a\x8d\x8c\x7f\x87\x82\x80\x91\x90"; // MPOBJECTS
DATA(0x0020c918)
char s_cheatPos[8] = "\x8a\x8d\x8d\x8c\x90"; // MPPOS
DATA(0x0020c920)
char s_cheatFps[8] = "\x8a\x8d\x83\x8d\x90"; // MPFPS

RVA(0x00022ad0, 0x1f)
BOOL CCheatMgr::Init(HWND owner) {
    m_owner = owner;
    m_flag = 0;
    m_pendingCodeLength = 0;
    m_cheatsUsed = 0;
    return TRUE;
}

RVA(0x00022b00, 0xaf)
void CCheatMgr::Empty() {
    POSITION pos = m_map.GetStartPosition();
    CString key;
    if (pos != static_cast<POSITION>(0)) {
        do {
            void* value = 0;
            m_map.GetNextAssoc(pos, key, value);
            if (value != NULL) {
                delete static_cast<CheatEntry*>(value);
            }
        } while (pos != static_cast<POSITION>(0));
    }
    m_map.RemoveAll();
    m_owner = NULL;
    m_flag = 0;
    m_pendingCodeLength = 0;
    m_cheatsUsed = 0;
}












// @early-stop
RVA(0x00022be0, 0x71)
BOOL CCheatMgr::AddCheat(const char* code, i32 cmdId, i32 flag) {
    void* existing = 0;

    void* hit = m_map.Lookup(code, existing) ? existing : NULL;
    if (hit != NULL) {
        return FALSE;
    }
    CheatEntry* entry = new CheatEntry;
    if (entry == NULL) {
        return FALSE;
    }
    entry->commandId = cmdId;
    entry->flag = flag;
    m_map[code] = entry;
    return TRUE;
}

RVA(0x00022c80, 0x173)
void CCheatMgr::RegisterCheats() {
    AddCheat(s_cheatFps, IDX(CHEAT_FRAME_RATE_DISPLAY), 1);
    AddCheat(s_cheatPos, IDX(CHEAT_WORLD_POSITION_DISPLAY), 1);
    AddCheat(s_cheatObjects, IDX(CHEAT_OBJECT_COUNT_DISPLAY), 1);
    AddCheat(s_cheatNoInfo, IDX(CHEAT_DEBUG_FLAG20), 1);
    AddCheat(s_cheatStopwatch, IDX(CHEAT_ELAPSED_TIME_DISPLAY), 1);
    AddCheat(s_cheatHologram, IDX(CHEAT_KEVIN_LAMBERT), 1);
    AddCheat(s_cheatLambert, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(s_cheatLambertian, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(s_cheatGoble, IDX(CHEAT_PROGRAMMING_GOD), 1);
    AddCheat(s_cheatScorpio, IDX(CHEAT_PROGRAMMING_GOD), 1);
    AddCheat(s_cheatChop, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(s_cheatLith, IDX(CHEAT_MONOLITH), 1);
    AddCheat(s_cheatLogo, IDX(CHEAT_MONOLITH), 1);
    AddCheat(s_cheatMonolith, IDX(CHEAT_MONOLITH), 1);
    AddCheat(s_cheatMonolithBare, IDX(CHEAT_MONOLITH), 1);
    AddCheat(s_cheatDevHeads, IDX(CHEAT_NO_OP), 1);
    AddCheat(s_cheatBuild, IDX(CHEAT_DEBUG_FLAG400), 1);
    AddCheat(s_cheatWildWacky, IDX(CHEAT_WILD_WACKY), 1);
    AddCheat(s_cheatWaWa, IDX(CHEAT_WAWA), 1);
    LoadCheatConfig();
}








// @early-stop
RVA(0x00022e60, 0x1be)
void CCheatMgr::LoadCheatConfig() {
    CString defStr(static_cast<const char*>(g_emptyString));
    CString group;
    SYSTEMTIME now;
    GetLocalTime(&now);

    i32 i = 1;
    if (g_buteMgr.GetIntDef("Cheatz", "NumCheatz", 0) >= 1) {
        do {
            group.Format("Cheat%i", i);
            const char* grp = static_cast<const char*>(group);
            i32 expMonth = g_buteMgr.GetIntDef(grp, "ExpMonth", 0);
            i32 expYear = g_buteMgr.GetIntDef(static_cast<const char*>(group), "ExpYear", 0);
            if (expMonth == 0 || expYear == 0 || expYear > now.wYear || expMonth > now.wMonth) {
                if (g_buteMgr.Exists(static_cast<const char*>(group), "Text")) {
                    if (g_buteMgr.GetIntDef(static_cast<const char*>(group), "NonCheat", 0) == 1) {
                        const char* code = static_cast<const char*>(*g_buteMgr
                                               .GetStringDef(static_cast<const char*>(group), "Text", &defStr));
                        AddCheat(code, g_buteMgr.GetIntDef(static_cast<const char*>(group), "Value", 0x807b), 1);
                    } else {
                        const char* code = static_cast<const char*>(*g_buteMgr
                                               .GetStringDef(static_cast<const char*>(group), "Text", &defStr));
                        AddCheat(code, g_buteMgr.GetIntDef(static_cast<const char*>(group), "Value", 0x807b), 0);
                    }
                }
            }
            i++;
        } while (i <= g_buteMgr.GetIntDef("Cheatz", "NumCheatz", 0));
    }
}








// @early-stop
RVA(0x00023090, 0xfc)
BOOL CCheatMgr::CheckCode(CString code) {
    code.MakeUpper();
    for (i32 i = 0; i < code.GetLength(); i++) {
        code.SetAt(i, static_cast<char>(((static_cast<const char*>(code))[i] + 0x3d)));
    }



    void* value = 0;
    CheatEntry* found = m_map.Lookup(static_cast<const char*>(code), value)
                            ? static_cast<CheatEntry*>(value)
                            : 0;
    if (found == NULL) {
        return FALSE;
    }
    if (found->commandId > 0) {
        PostMessageA(m_owner, WM_COMMAND, found->commandId, 0);
        if ((found->flag & 1) == 0) {
            m_cheatsUsed = 1;
        }
        m_flag = 0;
        m_pendingCodeLength = 0;
    }
    return TRUE;
}

RVA(0x00085e60, 0x4a)
CCheatMgr::~CCheatMgr() {
    Empty();
}
