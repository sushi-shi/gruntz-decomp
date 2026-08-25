#include <rva.h>

#include <Gruntz/CheatMgr.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Utils/MapTyped.h>

#include <stddef.h>

// clang-format off
DATA(0x0020c838)
char s_cheatWaWa[20] = "\x8a\x8d\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e";
DATA(0x0020c84c)
char s_cheatWildWacky[16] = "\x8a\x8d\x94\x86\x89\x81\x94\x7e\x80\x88\x96";
DATA(0x0020c85c)
char s_cheatBuild[12] = "\x8a\x8d\x7f\x92\x86\x89\x81";
DATA(0x0020c868)
char s_cheatDevHeads[16] = "\x8a\x8d\x81\x82\x93\x85\x82\x7e\x81\x90";
DATA(0x0020c878)
char s_cheatMonolithBare[12] = "\x8a\x8c\x8b\x8c\x89\x86\x91\x85";
DATA(0x0020c884)
char s_cheatMonolith[16] = "\x8a\x8d\x8a\x8c\x8b\x8c\x89\x86\x91\x85";
DATA(0x0020c894)
char s_cheatLogo[8] = "\x8a\x8d\x89\x8c\x84\x8c";
DATA(0x0020c89c)
char s_cheatLith[8] = "\x8a\x8d\x89\x86\x91\x85";
DATA(0x0020c8a4)
char s_cheatChop[8] = "\x8a\x8d\x80\x85\x8c\x8d";
DATA(0x0020c8ac)
char s_cheatScorpio[12] = "\x8a\x8d\x90\x80\x8c\x8f\x8d\x86\x8c";
DATA(0x0020c8b8)
char s_cheatGoble[12] = "\x8a\x8d\x84\x8c\x7f\x89\x82";
DATA(0x0020c8c4)
char s_cheatLambertian[16] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91\x86\x7e\x8b";
DATA(0x0020c8d4)
char s_cheatLambert[12] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91";
DATA(0x0020c8e0)
char s_cheatHologram[16] = "\x8a\x8d\x85\x8c\x89\x8c\x84\x8f\x7e\x8a";
DATA(0x0020c8f0)
char s_cheatStopwatch[16] = "\x8a\x8d\x90\x91\x8c\x8d\x94\x7e\x91\x80\x85";
DATA(0x0020c900)
char s_cheatNoInfo[12] = "\x8a\x8d\x8b\x8c\x86\x8b\x83\x8c";
DATA(0x0020c90c)
char s_cheatObjects[12] = "\x8a\x8d\x8c\x7f\x87\x82\x80\x91\x90";
DATA(0x0020c918)
char s_cheatPos[8] = "\x8a\x8d\x8d\x8c\x90";
DATA(0x0020c920)
char s_cheatFps[8] = "\x8a\x8d\x83\x8d\x90";

RVA(0x00022ad0, 0x1f)
BOOL CCheatMgr::Init(HWND owner) {
    m_owner = owner;
    m_flag = 0;
    m_pendingCodeLength = 0;
    m_cheatsUsed = 0;
    return true;
}

RVA(0x00022b00, 0xaf)
void CCheatMgr::Empty() {
    POSITION pos = m_map.GetStartPosition();
    CString key;
    if (pos != static_cast<POSITION>(0)) {
        do {
            CheatEntry* value = NULL;
            MapGetNext(m_map, pos, key, value);
            if (value != NULL) {
                delete value;
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
    CheatEntry* existing = NULL;
    CheatEntry* hit = MapLookup(m_map, code, existing) ? existing : NULL;
    if (hit != NULL) {
        return false;
    }
    CheatEntry* entry = new CheatEntry;
    if (entry == NULL) {
        return false;
    }
    entry->commandId = cmdId;
    entry->flag = flag;
    m_map[code] = entry;
    return true;
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
    CString defStr(static_cast<const char*>(""));
    CString group;
    SYSTEMTIME now;
    GetLocalTime(&now);

    for (i32 i = 1; i <= g_buteMgr.GetIntDef("Cheatz", "NumCheatz", 0); i++) {
        group.Format("Cheat%i", i);
        const char* grp = static_cast<const char*>(group);
        i32 expMonth = g_buteMgr.GetIntDef(grp, "ExpMonth", 0);
        i32 expYear = g_buteMgr.GetIntDef(static_cast<const char*>(group), "ExpYear", 0);
        if (expMonth == 0 || expYear == 0 || expYear > now.wYear || expMonth > now.wMonth) {
            if (g_buteMgr.Exists(static_cast<const char*>(group), "Text")) {
                if (g_buteMgr.GetIntDef(static_cast<const char*>(group), "NonCheat", 0) == 1) {
                    const char* code = static_cast<const char*>(*g_buteMgr.GetStringDef(
                        static_cast<const char*>(group), "Text", &defStr));
                    i32 value =
                        g_buteMgr.GetIntDef(static_cast<const char*>(group), "Value", 0x807b);
                    AddCheat(code, value, 1);
                } else {
                    const char* code = static_cast<const char*>(*g_buteMgr.GetStringDef(
                        static_cast<const char*>(group), "Text", &defStr));
                    AddCheat(code,
                             g_buteMgr.GetIntDef(static_cast<const char*>(group), "Value", 0x807b),
                             0);
                }
            }
        }
    }
}








// @early-stop
RVA(0x00023090, 0xfc)
BOOL CCheatMgr::CheckCode(CString code) {
    code.MakeUpper();
    for (i32 i = 0; i < code.GetLength(); i++) {
        code.SetAt(i, static_cast<char>(((static_cast<const char*>(code))[i] + 0x3d)));
    }



    CheatEntry* value = NULL;
    CheatEntry* found = MapLookup(m_map, static_cast<const char*>(code), value) ? value : NULL;
    if (found == NULL) {
        return false;
    }
    if (found->commandId > 0) {
        PostMessageA(m_owner, WM_COMMAND, found->commandId, 0);
        if ((found->flag & 1) == 0) {
            m_cheatsUsed = 1;
        }
        m_flag = 0;
        m_pendingCodeLength = 0;
    }
    return true;
}

RVA(0x00085e60, 0x4a)
CCheatMgr::~CCheatMgr() {
    Empty();
}
