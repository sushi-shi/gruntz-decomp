#include <rva.h>
#include <Image/CImage.h> // g_resourceInstallActive
#include <Mfc.h>
#include <Gruntz/AreaMgr.h>
#include <Bute/SymTab.h>

#include <stdio.h>  // sprintf (reloc-masked engine CRT)
#include <string.h> // inline strcmp / strncmp


#include <DDrawMgr/DDrawSubMgrLeaf.h> // canonical CDDrawSubMgrLeaf (incl. the ANI set) + CAniElement
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (ScanTree)
#include <DDrawMgr/DDrawWorkerRegistry.h> // the canonical image/worker registry (CDDrawWorkerRegistry)
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawSurfaceMgr (the per-spawn registry holder)

DATA(0x002459b0)
CAreaMgr g_areaMgr;

RVA(0x00099b80, 0xa)
void TokenMgrReset() {
    g_areaMgr.CAreaMgr::CAreaMgr();
}

RVA(0x00099ba0, 0x29)
CAreaMgr::CAreaMgr() {
    m_currentAreaIndex = 0;
}

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

RVA(0x00099c20, 0x5f)
CAreaMgr::~CAreaMgr() {
    Reset();
    // m_spawnEntryList (the CSpawnList) is torn down here by the compiler-emitted
    // /GX member teardown: the inline ~CSpawnList above folds in.
}

// ---------------------------------------------------------------------------
// CSpawnList::~CSpawnList  (0x099ca0)
// DeleteAllEntries, then the embedded CPtrList member dtor frees its blocks (the
// trailing ~CPtrList under the /GX frame). Defined INLINE in this TU (its retail
// home) so it folds into ~CAreaMgr's member-teardown below exactly as retail
// (call DeleteAllEntries + call ~CPtrList, EH states 1 -> -1); other TUs see only
// the SpawnList.h declaration and emit the retail extern call (e.g.
// CGruntSpawnConfig::Clear's explicit dtor + RezFree). The standalone COMDAT
// copy is forced by the depth-0 forcer and pinned by mangled name:
RVA_COMPGEN(0x00099ca0, 0x49, ??1CSpawnList@@QAE@XZ)

RVA(0x00099d10, 0x20)
i32 QueryToken(i32 arg) {
    g_areaMgr.Reset();
    return g_areaMgr.Dispatch(arg) != 0;
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
    for (CSpawnNode* n = reinterpret_cast<CSpawnNode*>(m_list.GetHeadPosition()); n != 0; n = n->m_next) {
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
    for (CSpawnNode* n = reinterpret_cast<CSpawnNode*>(m_list.GetHeadPosition()); n != 0; n = n->m_next) {
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

RVA(0x0009a420, 0x1c)
void CSpawnList::ClearFlags() {
    CSpawnNode* p = reinterpret_cast<CSpawnNode*>(m_list.GetHeadPosition());
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

RVA(0x0009a450, 0x36)
void CSpawnList::DeleteAllEntries() {
    CSpawnNode* node = reinterpret_cast<CSpawnNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CSpawnNode* cur = node;
        node = node->m_next;
        CSpawnEntry* e = cur->m_entry;
        if (e != 0) {
            // CSpawnEntry's only destructible member is m_name (CString @+0x00); retail
            // emits the trivial-forwarding entry dtor as a direct ~CString @0x1b9cde +
            // operator delete, so spell the delete through the CString subobject.
            delete reinterpret_cast<CString*>(e);
        }
    }
    m_list.RemoveAll();
}

RVA(0x0009a4c0, 0x3e)
i32 CAreaMgr::LoadObjectResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    LoadObjectImageResources(entry, src);
    LoadObjectSoundResources(entry, src);
    LoadObjectAnimResources(entry, src);
    return 1;
}

// @source: decomp-xref
// @early-stop
// ~88.7%: complete + correct (the OBJECTZ_ GetNextAssoc scan, FindAdd reconcile,
// CPtrList drain via the registry's ProcessNew, the IMAGEZ_%s sprintf + CSymTab
// ResolvePath + the polymorphic vtable Install, and the g_resourceInstallActive bracket
// are all byte-faithful, strings/relocs aligned). Residual: retail's frame is 0xb4
// vs this build's 0xac because retail reserves a guarded CString cleanup slot
// ([esp+0x24] with its construction guard at [esp+0x1c]) that it NEVER constructs
// nor sets in the normal path - dead EH-cleanup scaffolding the optimizer emitted
// for an elided by-value name temp - plus the matching conditional ~CString block.
// That phantom +8 shifts every [esp+N] operand, capping the fuzzy score though the
// instruction selection is identical. Not source-steerable without reproducing the
// elided-but-scaffolded temp. Reused for the Sound/Anim siblings below.
RVA(0x0009a510, 0x275)
i32 CAreaMgr::LoadObjectImageResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToOb* srcMap = &entry->m_imageRegistry->m_10map;
    if (srcMap == 0) {
        return 0;
    }

    CPtrList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        CObject* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp(static_cast<const char*>(static_cast<LPCTSTR>(key)), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != 0) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        CDDrawWorker* obj = static_cast<CDDrawWorker*>(toAdd.GetNext(dp)); // the pooled map values ARE workers
        entry->m_imageRegistry->RemoveWorker(obj);
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = reinterpret_cast<CSpawnNode*>(b->m_list.GetHeadPosition());
    CSpawnEntry* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CSpawnNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_entry;
    }
    while (e != 0) {
        if (e->m_flag == 0) {
            char buf[0x80];
            g_resourceInstallActive = 1;
            sprintf(buf, "IMAGEZ_%s", static_cast<const char*>(static_cast<LPCTSTR>(e->GetTail())));
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_imageRegistry->InstallTree(handle, const_cast<char*>(static_cast<LPCTSTR>(e->GetName())), "");
            g_resourceInstallActive = 0;
            e->m_flag = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CSpawnNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_entry;
        }
    }
    return 1;
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
        tmp = static_cast<const char*>(m_name) + 8;
    }
    return tmp;
}

// @source: decomp-xref
// @early-stop
// ~88.3%: complete + correct Image sibling (SOUNDZ_ key, CMapStringToPtr source,
// the concrete entry->m_soundRegistry ProcessNew/Install, no install-gate bracket). Same phantom
// guarded-CString +8 frame-shift wall documented on the Image arm above.
RVA(0x0009a910, 0x261)
i32 CAreaMgr::LoadObjectSoundResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* srcMap = &entry->m_soundRegistry->m_10;
    if (srcMap == 0) {
        return 0;
    }

    CPtrList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        void* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp(static_cast<const char*>(static_cast<LPCTSTR>(key)), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != 0) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        void* obj = toAdd.GetNext(dp);
        entry->m_soundRegistry->RemoveByValue(static_cast<LeafCue*>(obj));
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = reinterpret_cast<CSpawnNode*>(b->m_list.GetHeadPosition());
    CSpawnEntry* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CSpawnNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_entry;
    }
    while (e != 0) {
        if (e->m_flag == 0) {
            char buf[0x80];
            sprintf(buf, "SOUNDZ_%s", static_cast<const char*>(static_cast<LPCTSTR>(e->GetTail())));
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_soundRegistry->ScanTree(static_cast<CSymTab*>(handle), const_cast<char*>(static_cast<LPCTSTR>(e->GetName())), "");
            e->m_flag = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CSpawnNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_entry;
        }
    }
    return 1;
}

// @source: decomp-xref
// @early-stop
// ~88.3%: complete + correct Sound sibling (ANIZ_ key, the concrete entry->m_animRegistry
// ProcessNew/Install). Same phantom guarded-CString +8 frame-shift wall as above.
RVA(0x0009ac20, 0x261)
i32 CAreaMgr::LoadObjectAnimResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* srcMap = &entry->m_animRegistry->m_10;
    if (srcMap == 0) {
        return 0;
    }

    CPtrList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        void* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp(static_cast<const char*>(static_cast<LPCTSTR>(key)), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != 0) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        void* obj = toAdd.GetNext(dp);
        entry->m_animRegistry->RemoveValue(static_cast<CAniElement*>(obj)); // m_animRegistry is CDDrawSubMgrLeaf*
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = reinterpret_cast<CSpawnNode*>(b->m_list.GetHeadPosition());
    CSpawnEntry* e;
    if (b->m_cursor == 0) {
        e = 0;
    } else {
        CSpawnNode* n = b->m_cursor;
        b->m_cursor = n->m_next;
        e = n->m_entry;
    }
    while (e != 0) {
        if (e->m_flag == 0) {
            char buf[0x80];
            sprintf(buf, "ANIZ_%s", static_cast<const char*>(static_cast<LPCTSTR>(e->GetTail())));
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_animRegistry->ScanTree(static_cast<CSymTab*>(handle), const_cast<char*>(static_cast<LPCTSTR>(e->GetName())), "");
            e->m_flag = 1;
        }
        if (b->m_cursor == 0) {
            e = 0;
        } else {
            CSpawnNode* n = b->m_cursor;
            b->m_cursor = n->m_next;
            e = n->m_entry;
        }
    }
    return 1;
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
// @interleaver CAreaMgr::SameGroup emitted-in <boundary: foreign ILT thunk-table
// FUN_0049afXX..b410 @0x9af30-0x9b410 (before) + StateDispatch.cpp StateDispatch @0x9b770
// (after)>. A /Gy first-use COMDAT the linker placed past the thunk table, isolated from
// this TU's body run.
RVA(0x0009b430, 0x49)
i32 CAreaMgr::SameGroup(i32 a) {
    if (a <= 0) {
        return 0;
    }
    i32 ga = (a - 1) % 36 / 4 + 1;
    i32 gc = (m_currentAreaIndex - 1) % 36 / 4 + 1;
    return gc == ga;
}
