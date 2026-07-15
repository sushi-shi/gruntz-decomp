// DDrawWorkerRegistry.cpp - the 0x1549d0-0x155833 original TU (wave4-L dossier
// #15, block E; the leading 0x1549d0 CResolveNode COMDAT pocket rides this obj's
// contribution and stays in ResolveNode.cpp): the CDDrawWorkerRegistry keyed
// worker registry - the four DispatchKeyed*/Forward* pairs, the tree
// install/load-namespace walkers, the map scans/teardowns, ~CDDrawWorker, and the
// ??_GCDDrawSubMgr far-sibling scalar dtor. Held at the dossier-#9 boundary 1
// (0x155840); the registry's tiny virtuals in the G obj + its DestroyAll/
// FindKeyOfValue in the T obj live in those hosts.
//
// original TU: filename unknown (@identity-TODO - no __FILE__ anchor).
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine).
#include <rva.h>
#include <Bute/SymTab.h>
#include <Image/ImageSet.h>

#include <Gruntz/StateId.h> // StateId (GetStateId return type)
// <Mfc.h> brings real MFC afxcoll: CObject / CByteArray / CMapStringToOb / POSITION.
#include <Mfc.h>
#include <string.h> // strncpy (the StringCopy leaf, reloc-masked)
#include <stdio.h>  // sprintf ("%s%s%s" path builder in InstallTree / LoadNamespace)
#include <Globals.h>
// The canonical CDDrawWorkerRegistry (real polymorphic; the ex CWorkerVtableView /
// CDDrawRegistryDtorHost / CWorkerValue views are folded onto it + CDDrawWorker).
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawWorker.h> // the ONE canonical keyed worker (vtable 0x1efbe8)
#include <Gruntz/String.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/Loadable.h> // the ONE canonical CLoadable base

inline void* operator new(u32, void* p) {
    return p;
}

// The keyed worker is the ONE canonical CDDrawWorker (<DDrawMgr/DDrawWorker.h>,
// CLoadable-derived, 17-slot vtable @0x1efbe8). This TU owns the registry-side
// slot bodies (IsLoaded @0x155750, GetClassId @0x155770, SetKey @0x155810, the
// dtor @0x1557a0); the frame-collection slots live in the S1 obj
// (wwdgameobject). `new CDDrawWorker` makes cl auto-emit ??_7CDDrawWorker and
// the two-phase vptr stamps. (Was a TU-local duplicate CDDrawWorker definition +
// the RegWorkerValue reduced dispatch view - both dissolved onto the canonical.)
SIZE(CDDrawWorker, 0x6c);

// operator delete + the engine free.
void operator delete(void*);

// IDENTITY: CDDrawSubMgrFar is the linker-kept COMDAT-copy pair of CLoadable's
// (A)-form inline dtor - ??_G @0x155720 (this obj's span) calling ??1 @0xd5d70
// (CImage.cpp's span). It cannot be spelled on CLoadable itself while the canonical
// dtor is inline (<Gruntz/Loadable.h>; one-definition rule) - the scaffold name
// survives until the family's (B)-form explicit-ScalarDtor flip. Modeled with its
// ScalarDtor here so the ??_G call reloc binds to the real ??1 @0xd5d70.
class CDDrawSubMgrFar {
public:
    void* ScalarDtor(u32 flags); // 0x155720 (`??_G` scalar-deleting destructor)
    virtual ~CDDrawSubMgrFar();  // 0xd5d70 (member teardown, CImage.cpp)
};
SIZE_UNKNOWN(CDDrawSubMgrFar);

// (The former RegDirEntry tree-cursor view is DISSOLVED 2026-07-14: the walkers'
// FirstSub/NextSub children are Bute CSymTab scopes - m_name @+0x00, the same
// FirstSub/NextSub cursor ScanTree_157ee0 already walks as CSymTab - the DirNode
// precedent, <Bute/SymTab.h>.)

static inline i32 ReadRegistryField1c(const CDDrawWorkerRegistry* p) {
    return *(const i32*)((const char*)p + 0x1c);
}

static inline CDDrawWorker* MakeWorker(const CDDrawWorkerRegistry* parent) {
    // `new CDDrawWorker`: base ctor stamps 0x5efc30, the m_items CObArray member is
    // default-constructed, the derived ctor stamps 0x5efbe8 (cl-implicit vptr stores).
    CDDrawWorker* w = new CDDrawWorker;
    if (w != 0) {
        i32 field1c = ReadRegistryField1c(parent);
        i32 surfaceMgr = parent->m_0c;
        w->m_04 = field1c;
        w->m_08 = 0;
        w->m_0c = surfaceMgr;
        w->m_64 = 99999;
        w->m_68 = 0;
    }
    return w;
}

static inline CDDrawWorker* FindOrCreateWorker(CDDrawWorkerRegistry* parent, const char* key) {
    CObject* found = 0;
    parent->m_10map.Lookup(key, found);
    if (found == 0) {
        CDDrawWorker* worker = MakeWorker(parent);
        if (worker->SetKey_155810(key) == 0) {
            if (worker != 0) {
                delete worker;
            }
            return 0;
        }
        parent->m_10map[key] = (CObject*)worker;
        found = (CObject*)worker;
    }
    return (CDDrawWorker*)found;
}

// ---------------------------------------------------------------------------
// Slot 6 (the CLoadable-scheme IsReady override): clears the 25-dword blt-fx
// scratch table, seeds the first entry to 100, reports ready.
RVA(0x00154aa0, 0x20)
i32 CDDrawWorkerRegistry::IsReady() {
    for (i32 i = 0; i < 25; ++i) {
        g_bltFxScratch[i] = 0;
    }
    g_bltFxScratch[0] = 100;
    return 1;
}

// ---------------------------------------------------------------------------
// Slot 7 (the CLoadable-scheme Unload override): self-dispatch the slot-22 map
// teardown (+0x58, virtual on `this` - retail's `mov eax,[ecx]; call [eax+0x58]`),
// then clear the two install counters; the `xor eax,eax` doubles as the return 0.
RVA(0x00154ac0, 0x12)
i32 CDDrawWorkerRegistry::Unload() {
    MapTeardown_1552b0();
    g_resourceInstallActive = 0;
    g_surfaceColorKey = 0;
    return 0;
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot 14 (+0x38).
RVA(0x00154ae0, 0xfc)
i32 CDDrawWorkerRegistry::DispatchKeyed38(void* rec, const char* key, i32 a3, i32 a4) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->InsertFrame(rec, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot 13 (+0x34).
RVA(0x00154be0, 0xfc)
i32 CDDrawWorkerRegistry::DispatchKeyed34(i32 a1, const char* key, i32 a3, i32 a4) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->CreateFrame30(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot 12 (+0x30).
RVA(0x00154ce0, 0x101)
i32 CDDrawWorkerRegistry::DispatchKeyed30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->CreateFrame28(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot 11 (+0x2c).
RVA(0x00154df0, 0x101)
i32 CDDrawWorkerRegistry::DispatchKeyed2C(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->CreateFrame24(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Thin forwarders to worker slots 13/14/12/11 (+0x34/+0x38/+0x30/+0x2c).
RVA(0x00154f00, 0x1b)
i32 CDDrawWorkerRegistry::Forward34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4) {
    return worker->CreateFrame30(a1, a3, a4);
}

RVA(0x00154f20, 0x1b)
i32 CDDrawWorkerRegistry::Forward38(void* rec, CDDrawWorker* worker, i32 a3, i32 a4) {
    return worker->InsertFrame(rec, a3, a4);
}

RVA(0x00154f40, 0x20)
i32 CDDrawWorkerRegistry::Forward30(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5) {
    return worker->CreateFrame28(a1, a2, a4, a5);
}

RVA(0x00154f60, 0x20)
i32 CDDrawWorkerRegistry::Forward2C(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5) {
    return worker->CreateFrame24(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// 0x154f80: walk the directory tree under `dir`; for each entry build a path string
// ("<sub><prefix><name>" when sub is set, else just the name) into a 0x100 heap
// buffer and accumulate this->+0x48(entry, buf, prefix). Then, when sub is set,
// find-or-create the keyed worker, dispatch its +0x28(dir), and either run
// this->+0x54(sub) (worker inactive) or bump the count. /GX EH frame.
// @early-stop
// worker-ctor + regalloc wall: the directory walk / sprintf-vs-strcpy / +0x48
// dispatch / find-or-create / +0x28 / status branch are reproduced; retail's
// inline worker construction seeds fields before the CByteArray ctor + stamps the
// derived vtable last, and the buffer/entry register schedule differs.
// Reloc-masked EH-state + map/thunk names. Logic/CFG/offsets complete.
RVA(0x00154f80, 0x1d5)
i32 CDDrawWorkerRegistry::InstallTree(void* tree, const char* sub, const char* prefix) {
    CSymTab* dir = (CSymTab*)tree;
    char* buf = (char*)operator new(0x100);
    i32 count = 0;
    if (buf == 0) {
        return count;
    }
    buf[0] = 0;
    CSymTab* e = (CSymTab*)dir->FirstSub();
    while (e != 0) {
        if (sub != 0 && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        count += InstallTree(e, buf, prefix); // recursive slot-18 self-dispatch (+0x48)
        e = (CSymTab*)dir->NextSub(e);
    }
    if (sub != 0 && *sub != 0) {
        CDDrawWorker* w = FindOrCreateWorker(this, sub);
        if (w == 0) {
            return 0;
        }
        w->BuildFramesFromSymTab(dir);
        if (w->m_items.GetSize() == 0) {
            RemoveByKey(sub); // slot-21 self-dispatch (+0x54)
        } else {
            ++count;
        }
    }
    operator delete(buf);
    return count;
}

// ---------------------------------------------------------------------------
// 0x155160 LoadNamespace (slot 19): the read-side twin of InstallTree - walk the directory tree, build
// the same path string, and accumulate this->+0x4c(entry, buf, prefix) (a negative
// result aborts to -1). Then, when sub is set, Lookup it in the map; if present,
// dispatch its +0x3c(dir) (a -1 aborts to -1) and bump the count when the value's
// +0x18 is positive. No EH frame (plain /O2 leaf).
// @early-stop
// regalloc/loop-schedule wall: the walk / sprintf-vs-strcpy / +0x4c dispatch + <0
// abort / Lookup / +0x3c dispatch + -1 abort / +0x18 count are reproduced; the
// buffer/entry/count register schedule + reloc-masked thunk names are the residual.
RVA(0x00155160, 0x11e)
i32 CDDrawWorkerRegistry::LoadNamespace(void* tree, const char* sub, const char* prefix) {
    CSymTab* dir = (CSymTab*)tree;
    char* buf = (char*)operator new(0x100);
    i32 count = 0;
    CSymTab* e = (CSymTab*)dir->FirstSub();
    while (e != 0) {
        if (sub != 0 && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        i32 r = LoadNamespace(e, buf, prefix); // recursive slot-19 self-dispatch (+0x4c)
        if (r < 0) {
            operator delete(buf);
            return -1;
        }
        count += r;
        e = (CSymTab*)dir->NextSub(e);
    }
    if (sub != 0 && *sub != 0) {
        CObject* out = 0;
        m_10map.Lookup(sub, out);
        if (out != 0) {
            // Typed map-value retrieval: the stored values are CDDrawWorker.
            CDDrawWorker* w = (CDDrawWorker*)out;
            if (w->ValidateFramesFromSymTab(dir) == -1) {
                operator delete(buf);
                return -1;
            }
            if (w->m_items.GetSize() > 0) {
                ++count;
            }
        }
    }
    operator delete(buf);
    return count;
}

// ---------------------------------------------------------------------------
// Removes a non-null worker from the map by its key at +0x24, then destroys it.
RVA(0x00155280, 0x22)
void CDDrawWorkerRegistry::RemoveWorker(CDDrawWorker* worker) {
    if (worker != 0) {
        m_10map.RemoveKey(worker->m_key);
        delete worker;
    }
}

// ---------------------------------------------------------------------------
// Map teardown leaf (SEH): iterates m_10map via GetNextAssoc, destroys each
// CObject* value via its scalar-deleting dtor (vtbl+0x4 arg 1), then RemoveAll.
RVA(0x001552b0, 0xa2)
void CDDrawWorkerRegistry::MapTeardown_1552b0() {
    CObject* val = 0;
    POSITION pos = (POSITION)(m_10map.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_10map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete ((CDDrawWorker*)val); // the map values ARE the keyed workers
            }
        } while (pos != 0);
    }
    m_10map.RemoveAll();
}

// ---------------------------------------------------------------------------
// Map scan: remove every entry whose key strncmp-equals `str` (over its full
// length), destroying each removed value via its scalar dtor; returns the count.
RVA(0x00155360, 0xf8)
i32 CDDrawWorkerRegistry::RemoveKeysEqual_155360(const char* base, const char* str) {
    CString match(base);
    match = str;
    i32 len = match.GetLength();
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    i32 n = 0;
    while (pos != 0) {
        m_10map.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_10map.RemoveKey(key);
            if (val != 0) {
                delete ((CDDrawWorker*)val); // the map values ARE the keyed workers
            }
            ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// Map scan: sum each non-null value's GetMemoryUsage(a2) over the entries whose
// key strncmp-matches `str` (a null/empty `str` matches every entry). /GX frame.
// @early-stop
// zero-register-pin wall (~69%): the GetNextAssoc scan, match-all guard,
// strlen+strncmp compare, and per-value thiscall accumulation all reproduced.
// Residue: retail pins 0 in edi and holds a2 in ebx across the body where our
// cl uses test/immediate + a per-call a2 reload - regalloc coin-flip.
RVA(0x00155460, 0xe2)
i32 CDDrawWorkerRegistry::SumSizesEqual_155460(const char* str, i32 a2) {
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    i32 total = 0;
    if (pos != 0) {
        do {
            m_10map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                if (str == 0 || *str == 0 || strncmp(key, str, strlen(str)) == 0) {
                    total += ((CImageSet*)val)->GetMemoryUsage(a2);
                }
            }
        } while (pos != 0);
    }
    return total;
}

// ---------------------------------------------------------------------------
// Map scan: return 1 if any key strncmp-equals `str` over strlen(str), else 0.
RVA(0x00155550, 0xdc)
i32 CDDrawWorkerRegistry::HasKeyEqual_155550(const char* str) {
    i32 len = strlen(str);
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    while (pos != 0) {
        m_10map.GetNextAssoc(pos, key, val);
        if (strncmp(key, str, len) == 0) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Map scan: probe every non-null value via its FindFrame(a1,a2,a3); return 1
// on the first that returns nonzero, else 0. Returns 0 immediately if a1 is null.
// @early-stop
// regalloc wall (~75.9%) - complete & correct: a1==0 guard, the per-value
// FindFrame thiscall, the loop CFG all reproduced. Residue is the same
// val/loop-flag stack-slot swap + reloc-masked EH-state push as RemoveKeysEqual.
RVA(0x00155630, 0xc5)
i32 CDDrawWorkerRegistry::AnyValueMatches_155630(i32 a1, i32 a2, i32 a3) {
    if (a1 == 0) {
        return 0;
    }
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    while (pos != 0) {
        m_10map.GetNextAssoc(pos, key, val);
        if (val != 0 && ((CImageSet*)val)->FindFrame((CImage*)a1, (char*)a2, (int*)a3)) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x155720: CDDrawSubMgrFar's scalar-deleting destructor (retail placed this COMDAT
// in this obj's RVA span). Run the real member-teardown ~ (0xd5d70, CImage.cpp),
// then RezFree when the low deleting-flag bit is set; return this. Hand-written
// non-virtual + RVA pin (the CDDrawRegistryDtorHost::ScalarDtor pattern) so the
// member-dtor CALL reloc binds to ??1CDDrawSubMgrFar @0xd5d70 (reloc-fidelity).
RVA(0x00155720, 0x1e)
void* CDDrawSubMgrFar::ScalarDtor(u32 flags) {
    this->CDDrawSubMgrFar::~CDDrawSubMgrFar();
    if (flags & 1) {
        ::operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x155750 - CDDrawWorker::IsLoaded (slot 5 override): loaded iff the owner
// handle (m_0c) is set and the m_04 header word is not the -1 sentinel.
RVA(0x00155750, 0x16)
i32 CDDrawWorker::IsLoaded() {
    if (m_0c != 0 && m_04 != -1) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x155770 - CDDrawWorker::GetClassId (slot 8 override): the worker class tag.
RVA(0x00155770, 0x6)
i32 CDDrawWorker::GetClassId() {
    return CLASSID_WORKER;
}

// ===========================================================================
// 0x1557a0 - ~CDDrawWorker: stamp own vtable, run DeleteAll (the slot-7 Unload,
// devirtualized in the dtor to a direct call - the body lives in the S1 obj as
// CDDrawWorker::DeleteAll, reloc-masked), then the array member destructs and
// ~CLoadable folds in. /GX frame from the destructible base+member.
// ===========================================================================
// 100%: re-basing onto the canonical CLoadable : CWapObj : CObject resolved the
// grand-base vptr-stamp-position wall - the real CObject grand-base sinks the
// 0x5e8cb4 re-stamp after the m_04/m_08/m_0c resets exactly as retail.
// The cl-auto scalar-deleting destructor (vtable slot 1; generated from the
// virtual dtor below - @rva-symbol pairs the retail copy with the base COMDAT).
// @rva-symbol: ??_GCDDrawWorker@@UAEPAXI@Z 0x00155780 0x1e
RVA(0x001557a0, 0x68)
CDDrawWorker::~CDDrawWorker() {
    DeleteAll(); // retail's devirtualized slot-7 call == CDDrawWorker::DeleteAll (0x151eb0)
    // m_items.~::CObArray() (trylevel 0) + ~CLoadable() (field resets +
    // grand-base vtable stamp) fold here.
}

// ---------------------------------------------------------------------------
// 0x155810 - CDDrawWorker::SetKey (slot 9 override): copy at most 0x3F key bytes
// into the m_key buffer (+0x24), NUL-terminate at +0x63, return 1. (Was the
// misattributed CDDrawWorkerRegistry::StringCopy_155810 with raw this+0x24 casts;
// the buffer is the WORKER's key field.)
RVA(0x00155810, 0x23)
i32 CDDrawWorker::SetKey_155810(const char* src) {
    strncpy(m_key, src, 0x3f);
    m_key[0x3f] = 0;
    return 1;
}
