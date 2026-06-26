// AreaMgr.cpp - Gruntz CAreaMgr (C:\Proj\Gruntz).
//
// A small global area/zone state object (current-index word + embedded CPtrList +
// two ints).  Reset clears the index; Dispatch records a 1..40 index then calls
// the matching per-area handler; SameGroup is the level-loader's mod-36
// group-of-four adjacency test; the /GX dtor tears the CPtrList down.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Mfc.h>
#include <Gruntz/AreaMgr.h>

// CSpawnEntry - a spawn-point record held (by pointer) in CAreaMgr's +0x04 list.
// Its leading member is the entry name CString (+0x00); the class is non-
// polymorphic (no vtable, the CString is genuinely first). Only GetTail is
// reconstructed here, so the minimal layout suffices (single-TU helper).
class CSpawnEntry {
public:
    CString GetTail(); // 0x09a830
    CString m_name;    // +0x00  entry name ("GGGGNNNN..."); the tail is past col 8
};

// ---------------------------------------------------------------------------
// CAreaMgr::~CAreaMgr  (0x099c20)
// The /GX destructor: clears the current-index word (the +0x00 member teardown,
// trylevel 0) then auto-destroys the embedded CPtrList (trylevel 1 -> -1, its
// RemoveAllNodes + ~CPtrList).
// ---------------------------------------------------------------------------
RVA(0x00099c20, 0x5f)
CAreaMgr::~CAreaMgr() {
    Reset();
    // m_04 (the CPtrList) is auto-destroyed here by the compiler-emitted /GX
    // member teardown: RemoveAllNodes then ~CPtrList.
}

// ---------------------------------------------------------------------------
// CAreaMgr::Dispatch  (0x099d40)
// Guard the 1..40 range, reset the index word, record the index, then dispatch to
// the matching per-area handler (40-way jump table).
// ---------------------------------------------------------------------------
// @early-stop
// epilogue-tailmerge + jump-table wall (~79%): the 40 case bodies + dispatch logic
// are byte-exact; the residual is (a) retail hoists the default `xor eax,eax` before
// the jump table (index then in ecx, not eax) while the recompile keeps the index
// in eax with no pre-zero, and (b) retail emits the range-guard zero-return and the
// switch-default zero-return as two separate epilogues where the recompile
// tail-merges them.  Result-var idiom didn't help (regresses the tail-return cases).
// See docs/patterns/identical-return-epilogue-tailmerge.md +
// docs/patterns/switch-pointer-default-result-var.md + jumptable-data-overlap.md
RVA(0x00099d40, 0x21c)
i32 CAreaMgr::Dispatch(i32 index) {
    if (index <= 0 || index > 0x28) {
        return 0;
    }
    Reset();
    m_00 = index;
    switch (index) {
        case 1:
            return H01();
        case 2:
            return H02();
        case 3:
            return H03();
        case 4:
            return H04();
        case 5:
            return H05();
        case 6:
            return H06();
        case 7:
            return H07();
        case 8:
            return H08();
        case 9:
            return H09();
        case 10:
            return H10();
        case 11:
            return H11();
        case 12:
            return H12();
        case 13:
            return H13();
        case 14:
            return H14();
        case 15:
            return H15();
        case 16:
            return H16();
        case 17:
            return H17();
        case 18:
            return H18();
        case 19:
            return H19();
        case 20:
            return H20();
        case 21:
            return H21();
        case 22:
            return H22();
        case 23:
            return H23();
        case 24:
            return H24();
        case 25:
            return H25();
        case 26:
            return H26();
        case 27:
            return H27();
        case 28:
            return H28();
        case 29:
            return H29();
        case 30:
            return H30();
        case 31:
            return H31();
        case 32:
            return H32();
        case 33:
            return H33();
        case 34:
            return H34();
        case 35:
            return H35();
        case 36:
            return H36();
        case 37:
            return H37();
        case 38:
            return H38();
        case 39:
            return H39();
        case 40:
            return H40();
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// CAreaMgr::Reset  (0x09a0b0)
// Clear the current-index word.
// ---------------------------------------------------------------------------
RVA(0x0009a0b0, 0x7)
void CAreaMgr::Reset() {
    m_00 = 0;
}

// ---------------------------------------------------------------------------
// CSpawnEntry::GetTail  (0x09a830)  (/GX EH frame)
// Return the entry name past its 8-char group prefix; an empty string when the
// name is empty or 8 chars or shorter. NRV: build a local, copy-construct it
// into the caller's return slot, destruct the local (the /GX frame).
// ---------------------------------------------------------------------------
// @early-stop
// zero-register-pinning wall (~66%, docs/patterns/zero-register-pinning.md,
// topic:wall topic:regalloc): the branch structure + every CString call/operand
// is byte-faithful, but retail pins the 0 and 1 constants in callee-saved ebx/edi
// (push ebx;push esi;push edi; xor ebx,ebx; mov edi,1) and reuses them for the
// len==0 compare + the EH-state =0/=1 stores; the recompile keeps only esi and
// emits immediates, so the extra pushes phase-shift every stack offset. Not
// source-steerable. Deferred to the final sweep.
RVA(0x0009a830, 0xa4)
CString CSpawnEntry::GetTail() {
    CString tmp;
    i32 len = m_name.GetLength();
    if (len == 0)
        return tmp;
    if (len > 8)
        tmp = (const char*)m_name + 8;
    return tmp;
}

// ---------------------------------------------------------------------------
// CAreaMgr::SameGroup  (0x09b430)
// Whether `a` (1-based) and the current index fall in the same group-of-four
// within mod-36 index space: group(n) = ((n - 1) % 36) / 4 + 1.  Returns 0 for
// a <= 0.
// ---------------------------------------------------------------------------
// @early-stop
// shrink-wrapped callee-save-push wall (~58%): the dual idiv-group math is logically
// identical, but retail keeps `a` in eax for the `if(a<=0)` guard and DEFERS the
// `push esi` past it, computing the arg group first; the recompile commits `a` to
// callee-saved esi at entry (push esi upfront) and computes the m_00 group first.
// Not source-steerable.  See docs/patterns/shrink-wrapped-callee-save-push.md
RVA(0x0009b430, 0x49)
i32 CAreaMgr::SameGroup(i32 a) {
    if (a <= 0) {
        return 0;
    }
    i32 ga = (a - 1) % 36 / 4 + 1;
    i32 gc = (m_00 - 1) % 36 / 4 + 1;
    return gc == ga;
}

// ===========================================================================
// CSpawnList - the CPtrList-derived list of CSpawnEntry* (m_pNodeHead @+0x04)
// that CAreaMgr hangs spawn points off.  FindEntry / Extract walk it comparing
// each entry's name against a search key, building a CString temp per node and
// tearing it down (the /GX CString-temp EH that drives both functions' frames).
//
// Field names are placeholders; only the OFFSETS + emitted code bytes matter.
// ===========================================================================
#include <string.h> // inline strcmp

// Each entry exposes its name as a CString by value (NRV, 0x2a1d thunk).
class CSpawnEntryN {
public:
    CString GetName(); // 0x2a1d
};

struct CSpawnNode {
    CSpawnNode* m_next; // +0x00
    void* m_pad04;
    CSpawnEntryN* m_8; // +0x08 the entry
};

class CSpawnList {
public:
    CSpawnEntryN* FindEntry(CString name, i32 useHash); // 0x09a0d0
    CSpawnEntryN* Extract(char* name);                  // 0x09a290

    void* m_vptr;       // +0x00 CPtrList vptr
    CSpawnNode* m_head; // +0x04 m_pNodeHead
};

// The name-match helper (__cdecl(entry-name, search-name, len), 0x120440).
extern "C" i32 SpawnNameCmp(const char* a, const char* b, i32 n); // 0x120440

// ---------------------------------------------------------------------------
// CSpawnList::FindEntry (0x09a0d0) - find an entry by name, either via the
// SpawnNameCmp hash helper (useHash) or an inline strcmp.  The search key is a
// by-value CString (destroyed on exit); each node yields a CString name temp.
// @early-stop
// /GX CString-temp EH wall: the list walk, the per-node GetName temp + its
// teardown, the hash / inline-strcmp branches and the by-value-param destruction
// are logically faithful, but the EH-state frame + CString temp spill layout do
// not reproduce byte-for-byte (the documented CString-temp-in-loop residual).
// ---------------------------------------------------------------------------
RVA(0x0009a0d0, 0x133)
CSpawnEntryN* CSpawnList::FindEntry(CString name, i32 useHash) {
    for (CSpawnNode* n = m_head; n != 0; n = n->m_next) {
        CSpawnEntryN* e = n->m_8;
        if (e == 0) {
            continue;
        }
        if (useHash != 0) {
            CString nm = e->GetName();
            if (SpawnNameCmp(nm, name, nm.GetLength()) == 0) {
                return e;
            }
        } else {
            CString nm = e->GetName();
            if (strcmp(name, nm) == 0) {
                return e;
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CSpawnList::Extract (0x09a290) - like FindEntry but the search key arrives as a
// raw char* wrapped into a CString up front; per node it inline-compares, and on
// a miss re-checks through SpawnNameCmp against an empty CString before moving on.
// @early-stop
// /GX CString-temp EH wall: same family as FindEntry; logic faithful, EH-state +
// CString temp layout is the byte residual.
// ---------------------------------------------------------------------------
RVA(0x0009a290, 0x138)
CSpawnEntryN* CSpawnList::Extract(char* name) {
    CString key(name);
    for (CSpawnNode* n = m_head; n != 0; n = n->m_next) {
        CSpawnEntryN* e = n->m_8;
        if (e == 0) {
            continue;
        }
        CString nm = e->GetName();
        if (strcmp(key, nm) == 0) {
            return e;
        }
        CString empty;
        if (SpawnNameCmp(nm, empty, nm.GetLength()) == 0) {
            return e;
        }
    }
    return 0;
}
