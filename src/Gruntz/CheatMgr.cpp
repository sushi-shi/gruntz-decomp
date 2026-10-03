#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/CheatMgr.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Utils/MapTyped.h>

#include <stddef.h>

char g_cheatWaWa[20] = "\x8a\x8d\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e";

char g_cheatWildWacky[16] = "\x8a\x8d\x94\x86\x89\x81\x94\x7e\x80\x88\x96";

char g_cheatBuild[12] = "\x8a\x8d\x7f\x92\x86\x89\x81";

char g_cheatDevHeads[16] = "\x8a\x8d\x81\x82\x93\x85\x82\x7e\x81\x90";

char g_cheatMonolithBare[12] = "\x8a\x8c\x8b\x8c\x89\x86\x91\x85";

char g_cheatMonolith[16] = "\x8a\x8d\x8a\x8c\x8b\x8c\x89\x86\x91\x85";

char g_cheatLogo[8] = "\x8a\x8d\x89\x8c\x84\x8c";

char g_cheatLith[8] = "\x8a\x8d\x89\x86\x91\x85";

char g_cheatChop[8] = "\x8a\x8d\x80\x85\x8c\x8d";

char g_cheatScorpio[12] = "\x8a\x8d\x90\x80\x8c\x8f\x8d\x86\x8c";

char g_cheatGoble[12] = "\x8a\x8d\x84\x8c\x7f\x89\x82";

char g_cheatLambertian[16] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91\x86\x7e\x8b";

char g_cheatLambert[12] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91";

char g_cheatHologram[16] = "\x8a\x8d\x85\x8c\x89\x8c\x84\x8f\x7e\x8a";

char g_cheatStopwatch[16] = "\x8a\x8d\x90\x91\x8c\x8d\x94\x7e\x91\x80\x85";

char g_cheatNoInfo[12] = "\x8a\x8d\x8b\x8c\x86\x8b\x83\x8c";

char g_cheatObjects[12] = "\x8a\x8d\x8c\x7f\x87\x82\x80\x91\x90";

char g_cheatPos[8] = "\x8a\x8d\x8d\x8c\x90";

char g_cheatFps[8] = "\x8a\x8d\x83\x8d\x90";

BOOL CCheatMgr::Init(HWND owner) {
    m_owner = owner;
    m_flag = false;
    m_pendingCodeLength = 0;
    m_cheatsUsed = false;
    return true;
}

void CCheatMgr::Empty() {
    std::map<std::string, CheatEntry*>::iterator pos = m_map.begin();
    std::string key;
    if (pos != m_map.end()) {
        do {
            CheatEntry* value = NULL;
            (key = pos->first, value = pos->second, ++pos);
            if (value != NULL) {
                delete value;
            }
        } while (pos != m_map.end());
    }
    m_map.clear();
    m_owner = NULL;
    m_flag = false;
    m_pendingCodeLength = 0;
    m_cheatsUsed = false;
}

BOOL CCheatMgr::AddCheat(const char* code, i32 cmdId, i32 flag) {
    CheatEntry* hit = FindCheat(code);
    if (hit != NULL) {
        return false;
    }
    CheatEntry* entry = new CheatEntry;
    if (entry == NULL) {
        return false;
    }
    entry->m_commandId = cmdId;
    entry->m_flag = flag;
    m_map[code] = entry;
    return true;
}

void CCheatMgr::RegisterCheats() {
    AddCheat(g_cheatFps, IDX(CHEAT_FRAME_RATE_DISPLAY), 1);
    AddCheat(g_cheatPos, IDX(CHEAT_WORLD_POSITION_DISPLAY), 1);
    AddCheat(g_cheatObjects, IDX(CHEAT_OBJECT_COUNT_DISPLAY), 1);
    AddCheat(g_cheatNoInfo, IDX(CHEAT_DEBUG_FLAG20), 1);
    AddCheat(g_cheatStopwatch, IDX(CHEAT_ELAPSED_TIME_DISPLAY), 1);
    AddCheat(g_cheatHologram, IDX(CHEAT_KEVIN_LAMBERT), 1);
    AddCheat(g_cheatLambert, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(g_cheatLambertian, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(g_cheatGoble, IDX(CHEAT_PROGRAMMING_GOD), 1);
    AddCheat(g_cheatScorpio, IDX(CHEAT_PROGRAMMING_GOD), 1);
    AddCheat(g_cheatChop, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(g_cheatLith, IDX(CHEAT_MONOLITH), 1);
    AddCheat(g_cheatLogo, IDX(CHEAT_MONOLITH), 1);
    AddCheat(g_cheatMonolith, IDX(CHEAT_MONOLITH), 1);
    AddCheat(g_cheatMonolithBare, IDX(CHEAT_MONOLITH), 1);
    AddCheat(g_cheatDevHeads, IDX(CHEAT_NO_OP), 1);
    AddCheat(g_cheatBuild, IDX(CHEAT_DEBUG_FLAG400), 1);
    AddCheat(g_cheatWildWacky, IDX(CHEAT_WILD_WACKY), 1);
    AddCheat(g_cheatWaWa, IDX(CHEAT_WAWA), 1);
    LoadCheatConfig();
}

void CCheatMgr::LoadCheatConfig() {
    std::string defStr(static_cast<const char*>(""));
    std::string group;
    SYSTEMTIME now;
    GetLocalTime(&now);

    for (i32 i = 1; i <= g_buteMgr.GetInt("Cheatz", "NumCheatz", 0); i++) {
        group = formatText("Cheat%i", i);
        const char* grp = (group).c_str();
        i32 expMonth = g_buteMgr.GetInt(grp, "ExpMonth", 0);
        i32 expYear = g_buteMgr.GetInt((group).c_str(), "ExpYear", 0);
        if (expMonth == 0 || expYear == 0 || expYear > now.wYear || expMonth > now.wMonth) {
            if (g_buteMgr.Exist((group).c_str(), "Text")) {
                if (g_buteMgr.GetInt((group).c_str(), "NonCheat", 0) == 1) {
                    const char* code = (*g_buteMgr.GetString(
                        (group).c_str(), "Text", &defStr)).c_str();
                    i32 value =
                        g_buteMgr.GetInt((group).c_str(), "Value", 0x807b);
                    AddCheat(code, value, 1);
                } else {
                    const char* code = (*g_buteMgr.GetString(
                        (group).c_str(), "Text", &defStr)).c_str();
                    AddCheat(code,
                             g_buteMgr.GetInt((group).c_str(), "Value", 0x807b),
                             0);
                }
            }
        }
    }
}

BOOL CCheatMgr::CheckCode(std::string code) {
    std::transform((code).begin(), (code).end(), (code).begin(), asciiUpper);
    for (i32 i = 0; i < static_cast<i32>((code).size()); i++) {
        (code)[i] = static_cast<char>((((code).c_str())[i] + 0x3d));
    }

    CheatEntry* found = FindCheat((code).c_str());
    if (found == NULL) {
        return false;
    }
    if (found->m_commandId > 0) {
        PostMessageA(m_owner, WM_COMMAND, found->m_commandId, 0);
        if ((found->m_flag & 1) == 0) {
            m_cheatsUsed = true;
        }
        m_flag = false;
        m_pendingCodeLength = 0;
    }
    return true;
}

CCheatMgr::~CCheatMgr() {
    Empty();
}
