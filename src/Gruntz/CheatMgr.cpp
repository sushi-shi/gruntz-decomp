#include <rva.h>

#include <Gruntz/CheatMgr.h>

#include <Bute/ButeMgr.h>
#include <EmptyString.h>
#include <Gruntz/GruntzCommandId.h>

#include <stddef.h>

// clang-format off
wchar_t* s_cheat_1 = L"MPWAWAWAWAWAWA";

DATA(0x0020c838)
char s_cheat_20c838[20] = "\x8a\x8d\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e";
DATA(0x0020c84c)
char s_cheat_20c84c[16] = "\x8a\x8d\x94\x86\x89\x81\x94\x7e\x80\x88\x96";
DATA(0x0020c85c)
char s_cheat_20c85c[12] = "\x8a\x8d\x7f\x92\x86\x89\x81";
DATA(0x0020c868)
char s_cheat_20c868[16] = "\x8a\x8d\x81\x82\x93\x85\x82\x7e\x81\x90";
DATA(0x0020c878)
char s_cheat_20c878[12] = "\x8a\x8c\x8b\x8c\x89\x86\x91\x85";
DATA(0x0020c884)
char s_cheat_20c884[16] = "\x8a\x8d\x8a\x8c\x8b\x8c\x89\x86\x91\x85";
DATA(0x0020c894)
char s_cheat_20c894[8] = "\x8a\x8d\x89\x8c\x84\x8c";
DATA(0x0020c89c)
char s_cheat_20c89c[8] = "\x8a\x8d\x89\x86\x91\x85";
DATA(0x0020c8a4)
char s_cheat_20c8a4[8] = "\x8a\x8d\x80\x85\x8c\x8d";
DATA(0x0020c8ac)
char s_cheat_20c8ac[12] = "\x8a\x8d\x90\x80\x8c\x8f\x8d\x86\x8c";
DATA(0x0020c8b8)
char s_cheat_20c8b8[12] = "\x8a\x8d\x84\x8c\x7f\x89\x82";
DATA(0x0020c8c4)
char s_cheat_20c8c4[16] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91\x86\x7e\x8b";
DATA(0x0020c8d4)
char s_cheat_20c8d4[12] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91";
DATA(0x0020c8e0)
char s_cheat_20c8e0[16] = "\x8a\x8d\x85\x8c\x89\x8c\x84\x8f\x7e\x8a";
DATA(0x0020c8f0)
char s_cheat_20c8f0[16] = "\x8a\x8d\x90\x91\x8c\x8d\x94\x7e\x91\x80\x85";
DATA(0x0020c900)
char s_cheat_20c900[12] = "\x8a\x8d\x8b\x8c\x86\x8b\x83\x8c";
DATA(0x0020c90c)
char s_cheat_20c90c[12] = "\x8a\x8d\x8c\x7f\x87\x82\x80\x91\x90";
DATA(0x0020c918)
char s_cheat_20c918[8] = "\x8a\x8d\x8d\x8c\x90";
DATA(0x0020c920)
char s_cheat_20c920[8] = "\x8a\x8d\x83\x8d\x90";

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
    AddCheat(s_cheat_20c920, IDX(CHEAT_FRAME_RATE_DISPLAY), 1);
    AddCheat(s_cheat_20c918, IDX(CHEAT_WORLD_POSITION_DISPLAY), 1);
    AddCheat(s_cheat_20c90c, IDX(CHEAT_OBJECT_COUNT_DISPLAY), 1);
    AddCheat(s_cheat_20c900, IDX(CHEAT_DEBUG_FLAG20), 1);
    AddCheat(s_cheat_20c8f0, IDX(CHEAT_ELAPSED_TIME_DISPLAY), 1);
    AddCheat(s_cheat_20c8e0, IDX(CHEAT_KEVIN_LAMBERT), 1);
    AddCheat(s_cheat_20c8d4, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(s_cheat_20c8c4, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(s_cheat_20c8b8, IDX(CHEAT_PROGRAMMING_GOD), 1);
    AddCheat(s_cheat_20c8ac, IDX(CHEAT_PROGRAMMING_GOD), 1);
    AddCheat(s_cheat_20c8a4, IDX(CHEAT_KEVIN_LAMBERT_ALT), 1);
    AddCheat(s_cheat_20c89c, IDX(CHEAT_MONOLITH), 1);
    AddCheat(s_cheat_20c894, IDX(CHEAT_MONOLITH), 1);
    AddCheat(s_cheat_20c884, IDX(CHEAT_MONOLITH), 1);
    AddCheat(s_cheat_20c878, IDX(CHEAT_MONOLITH), 1);
    AddCheat(s_cheat_20c868, IDX(CHEAT_NO_OP), 1);
    AddCheat(s_cheat_20c85c, IDX(CHEAT_DEBUG_FLAG400), 1);
    AddCheat(s_cheat_20c84c, 0x80be, 1);
    AddCheat(s_cheat_20c838, IDX(CHEAT_WAWA), 1);
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
