#include <Gruntz/CheatMgr.h>
#include <EmptyString.h> // g_emptyString

#include <rva.h>

#include <Bute/ButeMgr.h>

// The 19 built-in cheat-code string buffers - obfuscated .data byte arrays passed
// to AddCheat by address as the map key. Each holds a cheat code obfuscated as
// (uppercase-ASCII + 0x3d) per byte (CheckCode uppercases + adds 0x3d to the typed
// code before Lookup, so the stored key matches). Recovered from the retail .data
// bytes at each rva; de-obfuscated (byte-0x3d) plaintext shown in each comment.
// Defined here (not extern-only) so the bytes land in cheatmgr.obj; the array
// storage class is `char[]` (mutable), so cl5 mangles `?s_cheat_<rva>@@3PADA`
// which a header DATA() (clang's mangledName) would miss - @data-symbol names the
// exact cl5 symbol and is authority-checked against the base obj.
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
//
// Definitions in ascending-rva order (the retail .data layout). The explicit array
// size = the retail symbol stride; the string literal's implicit NUL + zero-fill
// reproduce the trailing NUL padding exactly (verified byte-for-byte vs retail).
// clang-format off
char s_cheat_20c838[20] = "\x8a\x8d\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e\x94\x7e"; // "MPWAWAWAWAWAWA"
char s_cheat_20c84c[16] = "\x8a\x8d\x94\x86\x89\x81\x94\x7e\x80\x88\x96";             // "MPWILDWACKY"
char s_cheat_20c85c[12] = "\x8a\x8d\x7f\x92\x86\x89\x81";                             // "MPBUILD"
char s_cheat_20c868[16] = "\x8a\x8d\x81\x82\x93\x85\x82\x7e\x81\x90";                 // "MPDEVHEADS"
char s_cheat_20c878[12] = "\x8a\x8c\x8b\x8c\x89\x86\x91\x85";                         // "MONOLITH"
char s_cheat_20c884[16] = "\x8a\x8d\x8a\x8c\x8b\x8c\x89\x86\x91\x85";                 // "MPMONOLITH"
char s_cheat_20c894[8] = "\x8a\x8d\x89\x8c\x84\x8c";                                  // "MPLOGO"
char s_cheat_20c89c[8] = "\x8a\x8d\x89\x86\x91\x85";                                  // "MPLITH"
char s_cheat_20c8a4[8] = "\x8a\x8d\x80\x85\x8c\x8d";                                  // "MPCHOP"
char s_cheat_20c8ac[12] = "\x8a\x8d\x90\x80\x8c\x8f\x8d\x86\x8c";                     // "MPSCORPIO"
char s_cheat_20c8b8[12] = "\x8a\x8d\x84\x8c\x7f\x89\x82";                             // "MPGOBLE"
char s_cheat_20c8c4[16] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91\x86\x7e\x8b";         // "MPLAMBERTIAN"
char s_cheat_20c8d4[12] = "\x8a\x8d\x89\x7e\x8a\x7f\x82\x8f\x91";                     // "MPLAMBERT"
char s_cheat_20c8e0[16] = "\x8a\x8d\x85\x8c\x89\x8c\x84\x8f\x7e\x8a";                 // "MPHOLOGRAM"
char s_cheat_20c8f0[16] = "\x8a\x8d\x90\x91\x8c\x8d\x94\x7e\x91\x80\x85";             // "MPSTOPWATCH"
char s_cheat_20c900[12] = "\x8a\x8d\x8b\x8c\x86\x8b\x83\x8c";                         // "MPNOINFO"
char s_cheat_20c90c[12] = "\x8a\x8d\x8c\x7f\x87\x82\x80\x91\x90";                     // "MPOBJECTS"
char s_cheat_20c918[8] = "\x8a\x8d\x8d\x8c\x90";                                      // "MPPOS"
char s_cheat_20c920[8] = "\x8a\x8d\x83\x8d\x90";                                      // "MPFPS"

RVA(0x00022ad0, 0x1f)
BOOL CCheatMgr::Init(i32 owner) {
    m_count = owner;
    m_flag = 0;
    m_120 = 0;
    m_124 = 0;
    return TRUE;
}

RVA(0x00022b00, 0xaf)
void CCheatMgr::Empty() {
    POSITION pos = reinterpret_cast<POSITION>((m_map.GetCount() != 0 ? -1 : 0));
    CString key;
    if (pos != static_cast<POSITION>(0)) {
        do {
            void* value = 0;
            m_map.GetNextAssoc(pos, key, value);
            if (value != 0) {
                delete static_cast<CheatEntry*>(value);
            }
        } while (pos != static_cast<POSITION>(0));
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
    i32 found = (m_map.Lookup(code, existing) ? -1 : 0) & reinterpret_cast<i32>(existing);
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
        code.SetAt(i, static_cast<char>(((static_cast<const char*>(code))[i] + 0x3d)));
    }

    void* value = 0;
    CheatEntry* found =
        reinterpret_cast<CheatEntry*>(((m_map.Lookup(static_cast<const char*>(code), value) ? -1 : 0) & reinterpret_cast<i32>(value)));
    if (found != 0) {
        if (found->commandId > 0) {
            PostMessageA(reinterpret_cast<HWND>(m_count), 0x111, found->commandId, 0);
            if ((found->flag & 1) == 0) {
                m_124 = 1;
            }
            m_flag = 0;
            m_120 = 0;
        }
    }
    return TRUE;
}

RVA(0x00085e60, 0x4a)
CCheatMgr::~CCheatMgr() {
    Empty();
}
