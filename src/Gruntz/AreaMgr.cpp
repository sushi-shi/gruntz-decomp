// AreaMgr.cpp - Gruntz CAreaMgr: the area/spawn manager TU (C:\Proj\Gruntz),
// retail band 0x99b60..0x9b4f0 (init frag + code + tail frag).
//
// ONE original TU (wave2-H merge): the former areamgr + loadobjectresources +
// mgrtokenquery units were a WOVEN single interval (TU_MIGRATION 0x099ba0, weave
// 0.42) - the LoadObject{Image,Sound,Anim}Resources reconcilers interleave with
// the CSpawnList methods, and the mgrtokenquery pocket (frag @0x99b60 +
// TokenMgrReset @0x99b80 + QueryToken @0x99d10) is PROVEN in-TU: the singleton's
// atexit/dtor-thunk blob (0x99be0..0x99c1f) sits BETWEEN CAreaMgr's ctor
// (0x99ba0) and dtor (0x99c20) - impossible for two first-link objs.
//
// The singleton at 0x6459b0 (ex "g_tokenMgr"/CTokenMgr view) IS the CAreaMgr
// instance: its "Reset"/"Dispatch" were CAreaMgr::Reset/Dispatch by RVA, so the
// placeholder type is folded onto the real class (TokenMgr.h deleted).
//
// CAreaMgr = {current-index word, embedded CSpawnList} (<Gruntz/AreaMgr.h> /
// <Gruntz/SpawnList.h>). Field names are placeholders (m_<hexoffset>); only the
// OFFSETS + the emitted code bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Mfc.h>
#include <Gruntz/AreaMgr.h>
#include <Bute/SymTab.h>

#include <stdio.h>  // sprintf (reloc-masked engine CRT)
#include <string.h> // inline strcmp / strncmp

// The name-match helper (__cdecl(entry-name, search-name, len), 0x120440).
extern "C" i32 SpawnNameCmp(const char* a, const char* b, i32 n); // 0x120440

// The sound/anim object registries dispatch each op to a distinct real class (verified by RVA);
// TU-local decls with the exact pointer arg types (load-bearing for the mangled names).
#include <Dsndmgr/SoundResMap.h> // canonical CSoundResMap (RemoveByValue @0x157b00) + CSoundRes
class DirNode;
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (ScanTree_157ee0)
class CCatalogNode;
class CDDrawSubMgrLeaf {
public:
    void RemoveValue_152660(CCatalogNode* n); // 0x152660
};
class CDDrawSubMgrAni {
public:
    i32 ScanTree_152ad0(CSymTab* t, const char* key, const char* g); // 0x152ad0
};

// ---------------------------------------------------------------------------
// The manager singleton @0x6459b0 (ex ?g_tokenMgr@@3UCTokenMgr@@A - the
// CTokenMgr view was a placeholder over this very class; folded). Its $E init
// frag (0x99b60), init body (0x99b80), atexit registration (0x99be0) and dtor
// thunk (0x99c00) are the file-scope-definition machinery of THIS TU.
// ---------------------------------------------------------------------------
DATA(0x002459b0)
extern CAreaMgr g_areaMgr;

// TokenMgrReset99b80 @0x099b80 - the singleton-init body the $E frag @0x99b60 calls:
// retail tail-jmps the CAreaMgr ctor @0x99ba0 (through ILT 0x3bac) to re-construct
// the singleton in place. @early-stop / reloc-defect: modeling this as a guardless
// direct-ctor tail-call needs the compiler's own dynamic-initializer shape - placement
// new here adds a null-guard (test/je) retail lacks; `Reset()` keeps the byte-exact
// 10-byte `mov ecx,&g_areaMgr; jmp <tgt>` shape but binds the jmp to Reset@0x9a0b0
// instead of the ctor. Deferred: dynamic-initializer / guardless-ctor modeling.
RVA(0x00099b80, 0xa)
void TokenMgrReset99b80() {
    g_areaMgr.Reset();
}

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

// QueryToken @0x099d10 - __cdecl free helper over the singleton: reset, dispatch
// the caller's token, return whether the dispatch produced a nonzero result (the
// int->bool neg/sbb/neg normalize). Reached from the level-load path (0x0ca200
// via thunk 0x0039a4).
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

// CSpawnEntry::GetName (0x0009a260) is now an inline member in the header.

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
            // CSpawnEntry's only destructible member is m_name (CString @+0x00); retail
            // emits the trivial-forwarding entry dtor as a direct ~CString @0x1b9cde +
            // operator delete, so spell the delete through the CString subobject.
            delete (CString*)e;
        }
    }
    m_list.RemoveAll();
}

// ===========================================================================
// LoadObject{Image,Sound,Anim}Resources (0x9a510/0x9a910/0x9ac20) - the three
// near-identical per-spawn-entry asset reconcilers for the OBJECTZ_ namespace
// tree.  __thiscall(this=tree, SpawnEntry* entry, ResRegistry* src):
//   1. ClearFlags() resets every existing child entry's +4 "wanted" flag to 0.
//   2. Walk the source registry's CMapString map (embedded at entry->reg+0x10),
//      and for each "OBJECTZ_"-prefixed key: FindAdd it in this tree; if it already
//      exists mark its flag, else queue the source value in a local CObList of new
//      objects.
//   3. Drain that CObList, handing each new source object to the registry's
//      ProcessNew (Image: a vtable slot; Sound/Anim: a direct method).
//   4. Re-scan the child entries; for each still-unwanted (flag==0) one, build the
//      "{IMAGEZ|SOUNDZ|ANIZ}_<name>" key, ResolvePath it in src, and Install it
//      through the registry, then mark the flag.  The Image arm brackets the
//      install with the g_resourceInstallActive tile-counter gate.
// The local CObList carries a destructor -> the /GX exception frame.  Only offsets
// / code bytes are load-bearing; helpers are reloc-masked engine externs.
// ===========================================================================

DATA(0x002bf37c)
extern i32 g_resourceInstallActive; // ?g_resourceInstallActive@@3HA (Image install bracket)

// (The old CObjResNode / CObjResListNode / CObjResBuilder / CObjResTree views
// were folded onto the canonicals: the "child entry" is CSpawnEntry (GetSpriteName
// == GetTail @0x9a830, GetAssetName == GetName @0x9a260, the +4 wanted flag ==
// m_flag), the "builder" is CSpawnList (FindAdd == FindByName @0x9a290, ClearFlags
// @0x9a420, head/cursor == m_list/m_cursor), and the "tree" is CAreaMgr itself
// (the builder at +0x04 is m_spawnEntryList; same retail TU band). See
// <Gruntz/SpawnList.h> for the unification proof.)

// The Image registry (entry->m_10). This is a FOREIGN engine class: its ??_7 and
// the intermediate slots are unreconstructed engine code, so the honest model names
// only the TWO dispatched slots - Install at vtable slot 18 (+0x48) and ProcessNew
// at slot 20 (+0x50), both __thiscall. Its CMapStringToOb source map is embedded at
// +0x10. Real polymorphic view: 18 fillers precede Install.
struct ObjImageRegistry {
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual void Install(void* h, char* name, const char* g); // slot 18 (+0x48)
    virtual void Slot19();
    virtual void ProcessNew(CObject* val); // slot 20 (+0x50)
    char m_pad04[0x10 - 0x4];
    CMapStringToOb m_map; // +0x10  source map (CString -> CImage*)
};

// The Sound registry (entry->m_28): concrete; CMapStringToPtr source map at +0x10,
// ProcessNew/Install are direct __thiscall methods.
struct ObjSoundRegistry {
    // ProcessNew @0x157b00 = CSoundResMap::RemoveByValue, Install @0x157ee0 =
    // CDDrawSubMgrLeafScan::ScanTree_157ee0; cast at each call.
    char m_pad04[0x10];
    CMapStringToPtr m_map; // +0x10
};

// The Anim registry (entry->m_2c): concrete; same shape as Sound, different methods.
struct ObjAnimRegistry {
    // ProcessNew @0x152660 = CDDrawSubMgrLeaf::RemoveValue_152660, Install @0x152ad0 =
    // CDDrawSubMgrAni::ScanTree_152ad0; cast at each call.
    char m_pad04[0x10];
    CMapStringToPtr m_map; // +0x10
};

// The per-spawn entry (arg1): the three registry slots.
struct ObjSpawnEntry {
    char m_pad00[0x10];
    ObjImageRegistry* m_10; // +0x10
    char m_pad14[0x28 - 0x14];
    ObjSoundRegistry* m_28; // +0x28
    ObjAnimRegistry* m_2c;  // +0x2c
};

// The resolution source (arg2): ResolvePath looks a namespaced key up, returning a
// handle (0 if absent).

// CAreaMgr::LoadObjectResources (0x9a4c0) - the composite entry that reconciles all
// three asset namespaces for one spawn entry: gate on a null entry, then chain the
// image/sound/anim reconcilers on `this`.
RVA(0x0009a4c0, 0x3e)
i32 CAreaMgr::LoadObjectResources(ObjSpawnEntry* entry, CSymTab* src) {
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
// CObList drain via the registry's ProcessNew, the IMAGEZ_%s sprintf + CSymTab
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
i32 CAreaMgr::LoadObjectImageResources(ObjSpawnEntry* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToOb* srcMap = &entry->m_10->m_map;
    if (srcMap == 0) {
        return 0;
    }

    CObList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        CObject* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp((const char*)(LPCTSTR)key, "OBJECTZ_", 8) == 0) {
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
        CObject* obj = toAdd.GetNext(dp);
        entry->m_10->ProcessNew(obj);
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = (CSpawnNode*)b->m_list.GetHeadPosition();
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
            sprintf(buf, "IMAGEZ_%s", (const char*)(LPCTSTR)e->GetTail());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            entry->m_10->Install(handle, (char*)(LPCTSTR)e->GetName(), "");
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
        tmp = (const char*)m_name + 8;
    }
    return tmp;
}

// @source: decomp-xref
// @early-stop
// ~88.3%: complete + correct Image sibling (SOUNDZ_ key, CMapStringToPtr source,
// the concrete entry->m_28 ProcessNew/Install, no install-gate bracket). Same phantom
// guarded-CString +8 frame-shift wall documented on the Image arm above.
RVA(0x0009a910, 0x261)
i32 CAreaMgr::LoadObjectSoundResources(ObjSpawnEntry* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* srcMap = &entry->m_28->m_map;
    if (srcMap == 0) {
        return 0;
    }

    CObList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        void* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp((const char*)(LPCTSTR)key, "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != 0) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail((CObject*)val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        CObject* obj = toAdd.GetNext(dp);
        ((CSoundResMap*)entry->m_28)->RemoveByValue((CSoundRes*)obj);
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = (CSpawnNode*)b->m_list.GetHeadPosition();
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
            sprintf(buf, "SOUNDZ_%s", (const char*)(LPCTSTR)e->GetTail());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            ((CDDrawSubMgrLeafScan*)entry->m_28)
                ->ScanTree_157ee0((DirNode*)handle, (char*)(LPCTSTR)e->GetName(), "");
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
// ~88.3%: complete + correct Sound sibling (ANIZ_ key, the concrete entry->m_2c
// ProcessNew/Install). Same phantom guarded-CString +8 frame-shift wall as above.
RVA(0x0009ac20, 0x261)
i32 CAreaMgr::LoadObjectAnimResources(ObjSpawnEntry* entry, CSymTab* src) {
    if (entry == 0) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* srcMap = &entry->m_2c->m_map;
    if (srcMap == 0) {
        return 0;
    }

    CObList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        void* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp((const char*)(LPCTSTR)key, "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != 0) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail((CObject*)val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        CObject* obj = toAdd.GetNext(dp);
        ((CDDrawSubMgrLeaf*)entry->m_2c)->RemoveValue_152660((CCatalogNode*)obj);
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = (CSpawnNode*)b->m_list.GetHeadPosition();
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
            sprintf(buf, "ANIZ_%s", (const char*)(LPCTSTR)e->GetTail());
            void* handle = src->ResolvePath(buf);
            if (handle == 0) {
                return 0;
            }
            ((CDDrawSubMgrAni*)entry->m_2c)
                ->ScanTree_152ad0((CSymTab*)handle, (char*)(LPCTSTR)e->GetName(), "");
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

SIZE_UNKNOWN(ObjAnimRegistry);
SIZE_UNKNOWN(ObjImageRegistry);
SIZE_UNKNOWN(ObjSoundRegistry);
SIZE_UNKNOWN(ObjSpawnEntry);

// --- vtable catalog ---
