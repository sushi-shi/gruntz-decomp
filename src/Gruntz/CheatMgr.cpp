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
// CCheatMgr::~CCheatMgr  (0x85e60)
// ===========================================================================
// Empty() the dictionary, then the embedded m_map's ~CMapStringToPtr fires from
// the destructor epilogue (a reloc-masked NAFXCW base call). The destructible
// member forces the /GX EH frame.
RVA(0x00085e60, 0x4a)
CCheatMgr::~CCheatMgr() {
    Empty();
}
