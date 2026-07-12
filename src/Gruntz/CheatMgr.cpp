// CheatMgr.cpp - the cheat-code dictionary (C:\Proj\Gruntz\, the 0x22bxx region).
//
// Four __thiscall methods of CCheatMgr, reconstructed in ascending-RVA order. The
// class embeds a CMapStringToPtr (at +0x04) keyed by each cheat's obfuscated code
// string; map values are heap CheatEntry {commandId, flag} pairs. Empty() and the
// destructor each build a destructible CString local, forcing the /GX EH frame, so
// this unit uses the "eh" flag profile. The destructor (0x85e60, 0x4a B) calls
// Empty() then the compiler emits the embedded m_map's ~CMapStringToPtr from the
// epilogue - the same wrapper shape as CGruntSpawnConfig's 0x85df0 dtor.
//
// LoadCheatConfig (0x22e60) is this class's bute/registry config loader; it lives
// in another TU (the engine_label_stubs backlog) so the RegisterCheats tail call
// reloc-masks. The MFC map helpers (Lookup/operator[]/GetNextAssoc/RemoveAll/the
// ~CMapStringToPtr base) and the CRT operator new/delete come from <Mfc.h> and are
// reloc-masked (NAFXCW / CRT, no body here).
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing. See include/Gruntz/CheatMgr.h for the recovered layout.
#include <Gruntz/CheatMgr.h>

#include <rva.h>

#include <Bute/ButeMgr.h>

// The global empty C string the default CString temp copies from (0x6293f4).
extern "C" char g_emptyString[];

// The global CButeMgr instance (the config tree LoadCheatConfig reads cheats from).
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The 19 built-in cheat-code string buffers (obfuscated .data byte arrays passed
// to AddCheat by address as the map key). Bound to their real .data RVAs here (not
// via DATA() in the header: a header DATA() is invisible to labels.py's per-.cpp
// text scan, and clang mangles a char[] extern differently than cl5, so @data-symbol
// names the exact cl5 symbol `?s_cheat_<rva>@@3PADA`).
// @data-symbol: ?s_cheat_20c920@@3PADA 0x0020c920
// @data-symbol: ?s_cheat_20c918@@3PADA 0x0020c918
// @data-symbol: ?s_cheat_20c90c@@3PADA 0x0020c90c
// @data-symbol: ?s_cheat_20c900@@3PADA 0x0020c900
// @data-symbol: ?s_cheat_20c8f0@@3PADA 0x0020c8f0
// @data-symbol: ?s_cheat_20c8e0@@3PADA 0x0020c8e0
// @data-symbol: ?s_cheat_20c8d4@@3PADA 0x0020c8d4
// @data-symbol: ?s_cheat_20c8c4@@3PADA 0x0020c8c4
// @data-symbol: ?s_cheat_20c8b8@@3PADA 0x0020c8b8
// @data-symbol: ?s_cheat_20c8ac@@3PADA 0x0020c8ac
// @data-symbol: ?s_cheat_20c8a4@@3PADA 0x0020c8a4
// @data-symbol: ?s_cheat_20c89c@@3PADA 0x0020c89c
// @data-symbol: ?s_cheat_20c894@@3PADA 0x0020c894
// @data-symbol: ?s_cheat_20c884@@3PADA 0x0020c884
// @data-symbol: ?s_cheat_20c878@@3PADA 0x0020c878
// @data-symbol: ?s_cheat_20c868@@3PADA 0x0020c868
// @data-symbol: ?s_cheat_20c85c@@3PADA 0x0020c85c
// @data-symbol: ?s_cheat_20c84c@@3PADA 0x0020c84c
// @data-symbol: ?s_cheat_20c838@@3PADA 0x0020c838

// The cheat-config tag/key string literals (objdiff matches these .data relocs by
// value). The obfuscated code stored under each "Cheat%i" group's "Text" key is
// recovered by CheckCode by uppercasing then adding 0x3d to every byte.
#define s_Cheatz "Cheatz"
#define s_NumCheatz "NumCheatz"
#define s_Cheat_i "Cheat%i"
#define s_ExpMonth "ExpMonth"
#define s_ExpYear "ExpYear"
#define s_Text "Text"
#define s_NonCheat "NonCheat"
#define s_Value "Value"

// ===========================================================================
// CCheatMgr::Init  (0x22ad0)
// ===========================================================================
// Seed the +0 owner/count field from the arg and clear the scalar state
// (m_flag / m_120 / m_124); returns TRUE. The non-iterating counterpart to
// Empty() - same scalar clear, but it stores the arg into +0 instead of zeroing
// it and never walks the map (so no CString temp / no /GX frame).
RVA(0x00022ad0, 0x1f)
BOOL CCheatMgr::Init(i32 owner) {
    m_count = owner;
    m_flag = 0;
    m_120 = 0;
    m_124 = 0;
    return TRUE;
}

// ===========================================================================
// CCheatMgr::Empty  (0x22b00)
// ===========================================================================
// Iterate the map freeing every CheatEntry payload (operator delete), RemoveAll()
// the map, then clear the scalar state (m_count / m_flag / m_120 / m_124). The
// per-iteration CString key + the throwing map calls force the /GX EH frame.
RVA(0x00022b00, 0xaf)
void CCheatMgr::Empty() {
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    CString key;
    if (pos != (POSITION)0) {
        do {
            void* value = 0;
            m_map.GetNextAssoc(pos, key, value);
            if (value != 0) {
                delete (CheatEntry*)value;
            }
        } while (pos != (POSITION)0);
    }
    m_map.RemoveAll();
    m_count = 0;
    m_flag = 0;
    m_120 = 0;
    m_124 = 0;
}

// ===========================================================================
// CCheatMgr::AddCheat  (0x22be0)
// ===========================================================================
// Reject a code already present (Lookup returns true with a non-null value); else
// heap-allocate an 8-byte CheatEntry {cmdId, flag} and insert it into the map.
// Returns FALSE on duplicate or allocation failure, TRUE on insert.
//
// The duplicate guard `(Lookup() && value)` is the branchless mask-AND idiom
// `(Lookup ? -1 : 0) & (int)value` (retail: neg/sbb/and eax,edx; test eax,eax) -
// the named `found` local is what makes our cl emit the EXPLICIT `and` + test
// (vs the fused `test reg,reg` a plain inline expression yields).
// @early-stop
// regalloc/instruction-selection wall (98.57%) - complete & correct: the whole
// body byte-matches except the 1-region duplicate guard, where our cl spills the
// masked `found` to its stack slot (`mov [esp+N],eax`) + uses ecx for the value,
// while retail keeps it register-only (`and eax,edx; test eax,eax`, no spill).
// Same compiler; no source spelling reproduced the no-spill explicit AND (the
// inline forms all fold to a fused `test reg,reg`, scoring lower at 97.76%). See
// docs/patterns/branchless-mask-and-explicit-vs-fused-test.md.
RVA(0x00022be0, 0x71)
BOOL CCheatMgr::AddCheat(const char* code, i32 cmdId, i32 flag) {
    void* existing = 0;
    i32 found = (m_map.Lookup(code, existing) ? -1 : 0) & (i32)existing;
    if (found != 0) {
        return FALSE;
    }
    CheatEntry* entry = new CheatEntry;
    if (entry == 0) {
        return FALSE;
    }
    entry->commandId = cmdId;
    entry->flag = flag;
    m_map[code] = entry;
    return TRUE;
}

// ===========================================================================
// CCheatMgr::RegisterCheats  (0x22c80)
// ===========================================================================
// Seed the 19 built-in cheat codes (each -> a WM_COMMAND id) then load the
// per-config cheats from the bute/registry via LoadCheatConfig.
RVA(0x00022c80, 0x173)
void CCheatMgr::RegisterCheats() {
    AddCheat(s_cheat_20c920, 0x804b, 1);
    AddCheat(s_cheat_20c918, 0x804c, 1);
    AddCheat(s_cheat_20c90c, 0x804d, 1);
    AddCheat(s_cheat_20c900, 0x804e, 1);
    AddCheat(s_cheat_20c8f0, 0x806e, 1);
    AddCheat(s_cheat_20c8e0, 0x807a, 1);
    AddCheat(s_cheat_20c8d4, 0x807b, 1);
    AddCheat(s_cheat_20c8c4, 0x807b, 1);
    AddCheat(s_cheat_20c8b8, 0x803b, 1);
    AddCheat(s_cheat_20c8ac, 0x803b, 1);
    AddCheat(s_cheat_20c8a4, 0x807b, 1);
    AddCheat(s_cheat_20c89c, 0x8086, 1);
    AddCheat(s_cheat_20c894, 0x8086, 1);
    AddCheat(s_cheat_20c884, 0x8086, 1);
    AddCheat(s_cheat_20c878, 0x8086, 1);
    AddCheat(s_cheat_20c868, 0x8087, 1);
    AddCheat(s_cheat_20c85c, 0x816f, 1);
    AddCheat(s_cheat_20c84c, 0x80be, 1);
    AddCheat(s_cheat_20c838, 0x8175, 1);
    LoadCheatConfig();
}

// ===========================================================================
// CCheatMgr::LoadCheatConfig  (0x22e60)
// ===========================================================================
// Walk the "Cheatz"/"NumCheatz" config groups ("Cheat1".."CheatN"); for each that
// is not date-expired and carries a "Text" payload, register its de-obfuscated
// code -> "Value" WM_COMMAND (flag 1 when "NonCheat"==1, else 0). The two
// destructible CString temps force the /GX EH frame.
// @early-stop
// CString/EH + SYSTEMTIME-WORD wall: logic + control-flow + every bute/AddCheat
// extern correct; the residual is the /GX EH-state plateau plus the wYear/wMonth
// WORD reads (retail `mov dword + and 0xffff` vs cl's movzx). Not source-steerable.
RVA(0x00022e60, 0x1be)
void CCheatMgr::LoadCheatConfig() {
    CString defStr((const char*)g_emptyString);
    CString group;
    SYSTEMTIME now;
    GetLocalTime(&now);

    i32 i = 1;
    if (g_buteMgr.GetIntDef(s_Cheatz, s_NumCheatz, 0) >= 1) {
        do {
            group.Format(s_Cheat_i, i);
            const char* grp = (const char*)group;
            i32 expMonth = g_buteMgr.GetIntDef(grp, s_ExpMonth, 0);
            i32 expYear = g_buteMgr.GetIntDef((const char*)group, s_ExpYear, 0);
            if (expMonth == 0 || expYear == 0 || expYear > now.wYear || expMonth > now.wMonth) {
                if (g_buteMgr.Exists((const char*)group, s_Text)) {
                    if (g_buteMgr.GetIntDef((const char*)group, s_NonCheat, 0) == 1) {
                        const char* code = (const char*)*g_buteMgr
                                               .GetStringDef((const char*)group, s_Text, &defStr);
                        AddCheat(code, g_buteMgr.GetIntDef((const char*)group, s_Value, 0x807b), 1);
                    } else {
                        const char* code = (const char*)*g_buteMgr
                                               .GetStringDef((const char*)group, s_Text, &defStr);
                        AddCheat(code, g_buteMgr.GetIntDef((const char*)group, s_Value, 0x807b), 0);
                    }
                }
            }
            i++;
        } while (i <= g_buteMgr.GetIntDef(s_Cheatz, s_NumCheatz, 0));
    }
}

// ===========================================================================
// CCheatMgr::CheckCode  (0x23090)
// ===========================================================================
// A player-typed code (passed by value) is uppercased then de-obfuscated (+0x3d
// per byte) to match the stored keys; on a map hit with a positive command id,
// post the WM_COMMAND to the owner (m_count) and update the armed-cheat state.
// The by-value CString forces the /GX EH frame.
// @early-stop
// CString/EH plateau: logic + the branchless Lookup mask-AND idiom + the SetAt
// de-obfuscation loop are byte-exact; the residual is the /GX EH-state tracking
// around the by-value CString teardown. Not source-steerable.
RVA(0x00023090, 0xfc)
BOOL CCheatMgr::CheckCode(CString code) {
    code.MakeUpper();
    for (i32 i = 0; i < code.GetLength(); i++) {
        code.SetAt(i, (char)(((const char*)code)[i] + 0x3d));
    }

    void* value = 0;
    CheatEntry* found =
        (CheatEntry*)((m_map.Lookup((const char*)code, value) ? -1 : 0) & (i32)value);
    if (found != 0) {
        if (found->commandId > 0) {
            PostMessageA((HWND)m_count, 0x111, found->commandId, 0);
            if ((found->flag & 1) == 0) {
                m_124 = 1;
            }
            m_flag = 0;
            m_120 = 0;
        }
    }
    return TRUE;
}

// ===========================================================================
// CCheatMgr::~CCheatMgr  (0x85e60)
// ===========================================================================
// Empty() the dictionary, then the embedded m_map's ~CMapStringToPtr fires from
// the destructor epilogue (a reloc-masked NAFXCW base call). The destructible
// member forces the /GX EH frame.
RVA(0x00085e60, 0x4a)
CCheatMgr::~CCheatMgr() {
    Empty();
}
