// DDrawWorkerRegistry.cpp - the 0x1549d0-0x155833 original TU (wave4-L dossier
// #15, block E; the leading 0x1549d0 CResolveNode COMDAT pocket rides this obj's
// contribution and stays in ResolveNode.cpp): the CDDrawWorkerRegistry keyed
// worker registry - the four DispatchKeyed*/Forward* pairs, the tree
// insert/lookup walkers, the map scans/teardowns, ~CDDrawWorker, and the
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
#include <stdio.h>  // sprintf ("%s%s%s" path builder in InsertWorkerKey / LookupWorkerKey)
#include <Globals.h>
// The canonical CDDrawWorkerRegistry + its unified own-vtable view CWorkerVtableView
// + the CWorkerValue teardown view.
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/String.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/Loadable.h> // the ONE canonical CLoadable base

inline void* operator new(u32, void* p) {
    return p;
}

// A map value as seen by the scan helpers: it exposes a dword at +0x10 and a
// non-virtual probe (called from AnyValueMatches_155630).

// Real polymorphic two-level model (ALL-VTABLES mandate). CDDrawWorker derives the
// ONE canonical CLoadable. Ground truth: the real CLoadable vtable @0x1efc30 has
// [5] 0x155700 / [7] 0x155740 / [8] 0x154a00; CDDrawWorker's own vtable @0x1efbe8
// OVERRIDES them (IsLoaded->0x155750, Unload->DeleteAll 0x151eb0, GetClassId->
// 0x155770 = CLASSID_WORKER) and adds slots 9..16. `new CDDrawWorker` makes cl
// auto-emit ??_7CLoadable + ??_7CDDrawWorker and the two-phase vptr stamps.
// IDENTITY NOTE: this is the SAME retail class as <DDrawMgr/DDrawWorker.h>'s
// CDDrawWorker (whose frame-collection methods live in the S1 obj); this TU's
// view models the registry-side slots - a reconcile for the family-unification pass.
struct CDDrawWorker : public CLoadable {
    virtual ~CDDrawWorker() OVERRIDE;     // [1] 0x1557a0 (below); cl-auto-gen ??_G @0x155780
    virtual i32 IsLoaded() OVERRIDE;      // [5] 0x155750
    virtual i32 IsReady() OVERRIDE;       // [6] 0x001c08 (CWapObj default, inherited-shape)
    virtual i32 Unload() OVERRIDE;        // [7] 0x151eb0 (CDDrawWorker::DeleteAll, S1 obj)
    virtual i32 GetClassId() OVERRIDE;    // [8] 0x155770 -> CLASSID_WORKER
    virtual i32 Vfunc24(const char* key); // [9]  0x155810
    virtual void Slot10_1521f0();         // [10] 0x1521f0
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a4, i32 a5); // [11] 0x152110
    virtual i32 Vfunc30(i32 a1, i32 a2, i32 a4, i32 a5); // [12] 0x152060
    virtual i32 Vfunc34(i32 a1, i32 a3, i32 a4);         // [13] 0x151fb0
    virtual i32 Vfunc38(i32 a1, i32 a3, i32 a4);         // [14] 0x151f00
    virtual void Slot15_1522b0();                        // [15] 0x1522b0
    virtual void Slot16_1523b0();                        // [16] 0x1523b0
    // The slot-7 override's real (S1-obj) function is CDDrawWorker::DeleteAll @0x151eb0,
    // bound canonically (non-virtual QAEXXZ) in wwdgameobject; the dtor's devirtualized
    // slot-7 call targets it directly - reference the bound name so the CALL reloc is faithful.
    void DeleteAll(); // 0x151eb0  ?DeleteAll@CDDrawWorker@@QAEXXZ
    CDDrawWorker() {}

    CByteArray m_10; // +0x10  (m_04/m_08/m_0c inherited from CLoadable)
    char m_pad24[0x64 - 0x24];
    i32 m_64; // +0x64  0x1869f
    i32 m_68; // +0x68  0
}; // 0x6c
SIZE(CDDrawWorker, 0x6c);

DATA(0x002bf37c)
extern i32 g_resourceInstallActive;

// operator delete + the engine free.
void operator delete(void*);
extern "C" void RezFree(void* p); // _RezFree @0x1b9b82 (rezutil)

// The far sibling class (a FamilyMapBase-shaped, CObject-derived 5-slot class; real
// member-teardown ~ at 0xd5d70, CImage.cpp) whose scalar-deleting destructor
// (0x155720) landed in THIS obj's RVA span. Modeled with its ScalarDtor here so the
// ??_G call reloc binds to the real ??1CDDrawSubMgrFar @0xd5d70 (was misattributed to
// DDrawSubMgr.cpp's local CDDrawSubMgr, whose empty inline ~ left the call UNBOUND).
class CDDrawSubMgrFar {
public:
    void* ScalarDtor(u32 flags); // 0x155720 (`??_G` scalar-deleting destructor)
    virtual ~CDDrawSubMgrFar();  // 0xd5d70 (member teardown, CImage.cpp)
};
SIZE_UNKNOWN(CDDrawSubMgrFar);

// Helpers for the tree walkers: a directory-tree cursor (0x13a260 first child,
// 0x13a280 next child); each entry's +0x00 is a name string.
class RegDirEntry {
public:
    char* m_name; // +0x00
};
SIZE_UNKNOWN(RegDirEntry);

// A CDDrawWorker (vtable 0x5efbe8) viewed for the +0x28/+0x3c dispatches + the
// +0x18 status field; slots named from their retail slot-function RVAs.
class RegWorkerValue {
public:
    virtual void Slot00_1bef01();        // slot 0  0x1bef01 (CObject thunk)
    virtual void Slot04_155780();        // slot 1  0x155780 (scalar-dtor)
    virtual void Slot08_28ec();          // slot 2  0x0028ec (CObject thunk)
    virtual void Slot0C_106e();          // slot 3  0x00106e (CObject thunk)
    virtual void Slot10_4034();          // slot 4  0x004034 (CObject thunk)
    virtual void Slot14_155750();        // slot 5  0x155750
    virtual void Slot18_1c08();          // slot 6  0x001c08 (IsReady)
    virtual void Slot1C_151eb0();        // slot 7  0x151eb0 (DeleteAll)
    virtual void Slot20_155770();        // slot 8  0x155770
    virtual void Slot24_155810();        // slot 9  0x155810 (Vfunc24)
    virtual void Slot28_1521f0(i32 dir); // slot 10 (+0x28) 0x1521f0 (dispatched)
    virtual void Slot2C_152110();        // slot 11 0x152110 (Vfunc2C)
    virtual void Slot30_152060();        // slot 12 0x152060 (Vfunc30)
    virtual void Slot34_151fb0();        // slot 13 0x151fb0 (Vfunc34)
    virtual void Slot38_151f00();        // slot 14 0x151f00 (Vfunc38)
    virtual i32 Slot3C_1522b0(i32 dir);  // slot 15 (+0x3c) 0x1522b0 (dispatched)
    char m_pad04[0x18 - 0x04];
    i32 m_18; // +0x18  status field
};
SIZE_UNKNOWN(RegWorkerValue);
RELOC_VTBL(
    RegWorkerValue,
    0x001efbe8
); // reduced/derived view aliases CDDrawWorker (slot-RVA verified)

static inline i32 ReadRegistryField1c(const CDDrawWorkerRegistry* p) {
    return *(const i32*)((const char*)p + 0x1c);
}

static inline CDDrawWorker* MakeWorker(const CDDrawWorkerRegistry* parent) {
    // `new CDDrawWorker`: base ctor stamps 0x5efc30, the CByteArray member is
    // default-constructed, the Obj ctor stamps 0x5efbe8 (cl-implicit vptr stores).
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
    parent->m_map.Lookup(key, found);
    if (found == 0) {
        CDDrawWorker* worker = MakeWorker(parent);
        if (worker->Vfunc24(key) == 0) {
            if (worker != 0) {
                delete worker;
            }
            return 0;
        }
        parent->m_map[key] = (CObject*)worker;
        found = (CObject*)worker;
    }
    return (CDDrawWorker*)found;
}

// ---------------------------------------------------------------------------
// Clears the 25-dword scratch table and seeds the first entry to 100.
RVA(0x00154aa0, 0x20)
i32 CDDrawWorkerRegistry::ResetScratch() {
    for (i32 i = 0; i < 25; ++i) {
        g_bltFxScratch[i] = 0;
    }
    g_bltFxScratch[0] = 100;
    return 1;
}

// ---------------------------------------------------------------------------
// Runs the +0x58 hook, then clears the two counters.
RVA(0x00154ac0, 0x12)
void CDDrawWorkerRegistry::Shutdown() {
    ((CWorkerVtableView*)this)->MapTeardown_1552b0();
    g_resourceInstallActive = 0;
    g_surfaceColorKey = 0;
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x38.
RVA(0x00154ae0, 0xfc)
i32 CDDrawWorkerRegistry::DispatchKeyed38(i32 a1, const char* key, i32 a3, i32 a4) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc38(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x34.
RVA(0x00154be0, 0xfc)
i32 CDDrawWorkerRegistry::DispatchKeyed34(i32 a1, const char* key, i32 a3, i32 a4) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc34(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x30.
RVA(0x00154ce0, 0x101)
i32 CDDrawWorkerRegistry::DispatchKeyed30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc30(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x2c.
RVA(0x00154df0, 0x101)
i32 CDDrawWorkerRegistry::DispatchKeyed2C(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc2C(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Thin forwarders to worker slots +0x34/+0x38/+0x30/+0x2c.
RVA(0x00154f00, 0x1b)
i32 CDDrawWorkerRegistry::Forward34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4) {
    return worker->Vfunc34(a1, a3, a4);
}

RVA(0x00154f20, 0x1b)
i32 CDDrawWorkerRegistry::Forward38(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4) {
    return worker->Vfunc38(a1, a3, a4);
}

RVA(0x00154f40, 0x20)
i32 CDDrawWorkerRegistry::Forward30(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5) {
    return worker->Vfunc30(a1, a2, a4, a5);
}

RVA(0x00154f60, 0x20)
i32 CDDrawWorkerRegistry::Forward2C(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5) {
    return worker->Vfunc2C(a1, a2, a4, a5);
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
i32 CDDrawWorkerRegistry::InsertWorkerKey(CSymTab* dir, const char* sub, const char* prefix) {
    char* buf = (char*)operator new(0x100);
    i32 count = 0;
    if (buf == 0) {
        return count;
    }
    buf[0] = 0;
    RegDirEntry* e = (RegDirEntry*)dir->FirstSub();
    while (e != 0) {
        if (sub != 0 && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        count += ((CWorkerVtableView*)this)->Vfunc48(e, buf, (void*)prefix);
        e = (RegDirEntry*)dir->NextSub(e);
    }
    if (sub != 0 && *sub != 0) {
        CDDrawWorker* w = FindOrCreateWorker(this, sub);
        if (w == 0) {
            return 0;
        }
        ((RegWorkerValue*)w)->Slot28_1521f0((i32)dir);
        if (((RegWorkerValue*)w)->m_18 == 0) {
            ((CWorkerVtableView*)this)->Vfunc54(sub);
        } else {
            ++count;
        }
    }
    operator delete(buf);
    return count;
}

// ---------------------------------------------------------------------------
// 0x155160: the read-side twin of InsertWorkerKey - walk the directory tree, build
// the same path string, and accumulate this->+0x4c(entry, buf, prefix) (a negative
// result aborts to -1). Then, when sub is set, Lookup it in the map; if present,
// dispatch its +0x3c(dir) (a -1 aborts to -1) and bump the count when the value's
// +0x18 is positive. No EH frame (plain /O2 leaf).
// @early-stop
// regalloc/loop-schedule wall: the walk / sprintf-vs-strcpy / +0x4c dispatch + <0
// abort / Lookup / +0x3c dispatch + -1 abort / +0x18 count are reproduced; the
// buffer/entry/count register schedule + reloc-masked thunk names are the residual.
RVA(0x00155160, 0x11e)
i32 CDDrawWorkerRegistry::LookupWorkerKey(CSymTab* dir, const char* sub, const char* prefix) {
    char* buf = (char*)operator new(0x100);
    i32 count = 0;
    RegDirEntry* e = (RegDirEntry*)dir->FirstSub();
    while (e != 0) {
        if (sub != 0 && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        i32 r = ((CWorkerVtableView*)this)->Vfunc4C(e, buf, (void*)prefix);
        if (r < 0) {
            operator delete(buf);
            return -1;
        }
        count += r;
        e = (RegDirEntry*)dir->NextSub(e);
    }
    if (sub != 0 && *sub != 0) {
        CObject* out = 0;
        m_map.Lookup(sub, out);
        if (out != 0) {
            if (((RegWorkerValue*)out)->Slot3C_1522b0((i32)dir) == -1) {
                operator delete(buf);
                return -1;
            }
            if (((RegWorkerValue*)out)->m_18 > 0) {
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
        m_map.RemoveKey((const char*)((char*)worker + 0x24));
        delete worker;
    }
}

// ---------------------------------------------------------------------------
// Map teardown leaf (SEH): iterates m_map via GetNextAssoc, destroys each
// CObject* value via its scalar-deleting dtor (vtbl+0x4 arg 1), then RemoveAll.
RVA(0x001552b0, 0xa2)
void CDDrawWorkerRegistry::MapTeardown_1552b0() {
    CObject* val = 0;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete ((CWorkerValue*)val);
            }
        } while (pos != 0);
    }
    m_map.RemoveAll();
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
    POSITION pos = m_map.GetStartPosition();
    i32 n = 0;
    while (pos != 0) {
        m_map.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_map.RemoveKey(key);
            if (val != 0) {
                delete ((CWorkerValue*)val);
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
    POSITION pos = m_map.GetStartPosition();
    i32 total = 0;
    if (pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
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
    POSITION pos = m_map.GetStartPosition();
    while (pos != 0) {
        m_map.GetNextAssoc(pos, key, val);
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
    POSITION pos = m_map.GetStartPosition();
    while (pos != 0) {
        m_map.GetNextAssoc(pos, key, val);
        if (val != 0 && ((CImageSet*)val)->FindFrame((CImageFrame*)a1, (char*)a2, (int*)a3)) {
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

// ===========================================================================
// 0x1557a0 - ~CDDrawWorker: stamp own vtable, run DeleteAll (the slot-7 Unload,
// devirtualized in the dtor to a direct call - the body lives in the S1 obj as
// CDDrawWorker::DeleteAll, reloc-masked), then the array member destructs and
// ~CLoadable folds in. /GX frame from the destructible base+member.
// ===========================================================================
// 100%: re-basing onto the canonical CLoadable : CWapObj : CObject resolved the
// grand-base vptr-stamp-position wall - the real CObject grand-base sinks the
// 0x5e8cb4 re-stamp after the m_04/m_08/m_0c resets exactly as retail.
RVA(0x001557a0, 0x68)
CDDrawWorker::~CDDrawWorker() {
    DeleteAll(); // retail's devirtualized slot-7 call == CDDrawWorker::DeleteAll (0x151eb0)
    // m_10.~CByteArray() (trylevel 0) + ~CLoadable() (field resets +
    // grand-base vtable stamp) fold here.
}

// ---------------------------------------------------------------------------
// String copy leaf: copies at most 0x3F bytes from src into this+0x24,
// null-terminates at this+0x63, returns 1.
RVA(0x00155810, 0x23)
i32 CDDrawWorkerRegistry::StringCopy_155810(const char* src) {
    strncpy((char*)this + 0x24, src, 0x3f);
    *((char*)this + 0x63) = 0;
    return 1;
}
