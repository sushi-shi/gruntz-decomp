// AreaMgr.cpp - Gruntz CAreaMgr + the CSpawnList/CSpawnEntry methods of its
// retail TU band (0x99ba0..0x9b430), C:\Proj\Gruntz.
//
// CAreaMgr = {current-index word, embedded CSpawnList} (<Gruntz/AreaMgr.h> /
// <Gruntz/SpawnList.h>). The ctor folds the member list's inline ctor; the /GX
// dtor clears the index then inline-folds the member ~CSpawnList (EH state 1).
// The list/record methods here (FindEntry/FindByName/ClearFlags/DeleteAllEntries/
// GetName/GetTail) were previously scattered as per-TU views (CSpawnEntryN/
// CSpawnNode/local CSpawnList here, C99ba0/C9a420 in BoundaryLowerMethods,
// EmptyVoiceList/~CSpawnEntry in GruntSpawnConfig, Obj09a260) - all folded onto
// the two canonical classes (see SpawnList.h's unification proof).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Mfc.h>
#include <Gruntz/AreaMgr.h>

#include <string.h> // inline strcmp

// The name-match helper (__cdecl(entry-name, search-name, len), 0x120440).
extern "C" i32 SpawnNameCmp(const char* a, const char* b, i32 n); // 0x120440

// ---------------------------------------------------------------------------
// CAreaMgr::CAreaMgr  (0x099ba0)
// The member CSpawnList's inline ctor folds in: CObList(block size 10), scan
// cursor = 0, last-picked = -1; then the index word clears (body, last).
// ---------------------------------------------------------------------------
RVA(0x00099ba0, 0x29)
CAreaMgr::CAreaMgr() {
    m_currentAreaIndex = 0;
}

// ---------------------------------------------------------------------------
// CSpawnList::~CSpawnList  (0x099ca0)
// DeleteAllEntries, then the embedded CObList member dtor frees its blocks (the
// trailing ~CObList under the /GX frame). Defined INLINE in this TU (its retail
// home) so it folds into ~CAreaMgr's member-teardown below exactly as retail
// (call DeleteAllEntries + call ~CObList, EH states 1 -> -1); other TUs see only
// the SpawnList.h declaration and emit the retail extern call (e.g.
// CGruntSpawnConfig::Clear's explicit dtor + RezFree). The standalone COMDAT
// copy is forced by the depth-0 forcer and pinned by mangled name:
// @rva-symbol: ??1CSpawnList@@QAE@XZ 0x00099ca0 0x49
// ---------------------------------------------------------------------------
inline CSpawnList::~CSpawnList() {
    DeleteAllEntries();
}
static CSpawnList* volatile g_forceDtorSink;
#pragma inline_depth(0)
void ForceEmitSpawnListDtor() {
    g_forceDtorSink->~CSpawnList();
}
#pragma inline_depth()

// ---------------------------------------------------------------------------
// CAreaMgr::~CAreaMgr  (0x099c20)
// The /GX destructor: clears the current-index word (Reset, trylevel 0) then the
// member CSpawnList's dtor inline-folds (DeleteAllEntries, trylevel 1, then the
// member ~CObList, trylevel -1).
// ---------------------------------------------------------------------------
RVA(0x00099c20, 0x5f)
CAreaMgr::~CAreaMgr() {
    Reset();
    // m_spawnEntryList (the CSpawnList) is torn down here by the compiler-emitted
    // /GX member teardown: the inline ~CSpawnList above folds in.
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
    m_currentAreaIndex = index;
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
    m_currentAreaIndex = 0;
}

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
CSpawnEntry* CSpawnList::FindEntry(CString name, i32 useHash) {
    for (CSpawnNode* n = (CSpawnNode*)m_list.GetHeadPosition(); n != 0; n = n->m_next) {
        CSpawnEntry* e = n->m_entry;
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
// CSpawnEntry::GetName  (0x09a260)
// Return the record name by value (NRVO into the hidden return slot): a single
// CString copy-ctor (0x1b9ba3) from m_name into *retptr. Re-homed from the
// Obj09a260 placeholder TU - and 66.7% -> 100% here: the "dead 4-byte stack
// local" residual was the /GX EH machinery this retail TU compiles with (the
// old obj09a260 unit was flags=base), not a codegen artifact.
// ---------------------------------------------------------------------------
RVA(0x0009a260, 0x1d)
CString CSpawnEntry::GetName() {
    return m_name;
}

// ---------------------------------------------------------------------------
// CSpawnList::FindByName (0x09a290) - like FindEntry but the search key arrives
// by reference (retail pushes the CString's ADDRESS - the old areamgr Extract
// (char*) signature was refuted by the caller's `lea ecx,[esp+0x10]; push ecx`);
// per node it inline-compares, and on a miss re-checks through SpawnNameCmp
// against an empty CString before moving on. Was also declared as
// CObjResBuilder::FindAdd by the LoadObject* reconcilers - same one function.
// @early-stop
// /GX CString-temp EH wall: same family as FindEntry; logic faithful, EH-state +
// CString temp layout is the byte residual (retail's key build is an operator+
// against a global char - 0x1b9f81 with g_dat60b588 - not a plain copy; part of
// the parked residual).
// ---------------------------------------------------------------------------
RVA(0x0009a290, 0x138)
CSpawnEntry* CSpawnList::FindByName(const CString& name) {
    CString key(name);
    for (CSpawnNode* n = (CSpawnNode*)m_list.GetHeadPosition(); n != 0; n = n->m_next) {
        CSpawnEntry* e = n->m_entry;
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

// ---------------------------------------------------------------------------
// CSpawnList::ClearFlags  (0x09a420)
// Walk the node chain and zero every entry's "wanted" mark (re-homed from the
// BoundaryLowerMethods C9a420 view; the LoadObject* reconcilers' reset pass).
// ---------------------------------------------------------------------------
RVA(0x0009a420, 0x1c)
void CSpawnList::ClearFlags() {
    CSpawnNode* p = (CSpawnNode*)m_list.GetHeadPosition();
    if (p == 0) {
        return;
    }
    do {
        CSpawnNode* node = p;
        p = node->m_next;
        CSpawnEntry* e = node->m_entry;
        if (e != 0) {
            e->m_flag = 0;
        }
    } while (p != 0);
}

// ---------------------------------------------------------------------------
// CSpawnList::DeleteAllEntries  (0x09a450)
// Walk the CObList node chain; `delete` each held CSpawnEntry (the implicit
// ~CString + the engine operator delete = RezFree), then m_list.RemoveAll().
// No destructible local, so no /GX frame even under eh flags. (Was
// CSpawnEntry::EmptyVoiceList / AreaPtrList::RemoveAllNodes.)
// ---------------------------------------------------------------------------
RVA(0x0009a450, 0x36)
void CSpawnList::DeleteAllEntries() {
    CSpawnNode* node = (CSpawnNode*)m_list.GetHeadPosition();
    while (node != 0) {
        CSpawnNode* cur = node;
        node = node->m_next;
        CSpawnEntry* e = cur->m_entry;
        if (e != 0) {
            delete e;
        }
    }
    m_list.RemoveAll();
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
    if (len == 0) {
        return tmp;
    }
    if (len > 8) {
        tmp = (const char*)m_name + 8;
    }
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
// callee-saved esi at entry (push esi upfront) and computes the current index group first.
// Not source-steerable.  See docs/patterns/shrink-wrapped-callee-save-push.md
RVA(0x0009b430, 0x49)
i32 CAreaMgr::SameGroup(i32 a) {
    if (a <= 0) {
        return 0;
    }
    i32 ga = (a - 1) % 36 / 4 + 1;
    i32 gc = (m_currentAreaIndex - 1) % 36 / 4 + 1;
    return gc == ga;
}

// ---------------------------------------------------------------------------
// The area/zone dialog's OnInitDialog (0x0c2cb0), re-homed from the ApiCaller
// stubs: CAreaMgr::Dispatch (0x099d40) drives this dialog, whose HWND lives at
// +0x1c (the CWnd::m_hWnd slot of a CDialog subclass). It chains the base
// CDialog::OnInitDialog (0x1bac5e, reloc-masked) then arms a 50 ms repaint
// timer. A CDialog-derived class whose concrete identity is not yet recovered;
// modeled minimally (offsets + code bytes load-bearing).
struct AreaTimerDlg {
    char m_pad0[0x1c];
    HWND m_hWnd;            // +0x1c  CWnd::m_hWnd
    i32 OnInitDialog();     // 0x0c2cb0
    i32 BaseOnInitDialog(); // 0x1bac5e (CDialog::OnInitDialog)
};
SIZE_UNKNOWN(AreaTimerDlg);
RVA(0x000c2cb0, 0x1f)
i32 AreaTimerDlg::OnInitDialog() {
    BaseOnInitDialog();
    SetTimer(m_hWnd, 1, 0x32, 0);
    return 1;
}
