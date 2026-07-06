#include <rva.h>
#include <Bute/SymTab.h>
#include <Image/ImageSet.h>

#include <Gruntz/StateId.h> // StateId (GetStateId return type)
// CDDrawWorkerRegistry.cpp - leaf method(s) of the tomalla-named ddrawmgr surface-family
// sub-manager CDDrawWorkerRegistry (a CDirectDrawMgr surface/page sub-manager in the
// "DDraw surface manager" family; see docs/ddraw-family-names.md).
//
// CDDrawWorkerRegistry owns a CMapStringToOb at +0x10 (m_map) keyed by const char*
// strings. RemoveByKey is a keyed remove-and-destroy: Lookup the key in
// the map; if present, RemoveKey it and run the found value's scalar-deleting
// destructor (vtable +0x4, arg 1). Plain /O2 /MT leaf: NO SEH frame, NO data
// relocations - the only relocations are the reloc-masked rel32 thunk calls
// (CMapStringToOb::Lookup / RemoveKey, both out-of-line NAFXCW), and the found
// value's destructor is dispatched through its own vtable.
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine). The looked-up value is modeled as a polymorphic
// stub (virtuals at the right slots) ONLY so `val->ScalarDtor(1)` lowers to the
// exact `mov eax,[val]; push 1; call [eax+4]` __thiscall dispatch; its virtuals are
// never defined, so no vtable is emitted in this TU.
//
// The four factory methods construct a 0x6c-byte worker with a CByteArray member
// at +0x10, hence the TU is /GX so the inlined heap construction emits the same
// EH frame as retail.
// ---------------------------------------------------------------------------

// <Mfc.h> brings real MFC afxcoll: CObject / CByteArray / CMapStringToOb / POSITION
// (CString / CMapStringToOb signatures also via the shim includes below).
#include <Mfc.h>
#include <string.h> // strncpy (the StringCopy leaf, reloc-masked)
#include <stdio.h>  // sprintf ("%s%s%s" path builder in Stub_154f80 / Stub_155160)
#include <Globals.h>
// The canonical CDDrawWorkerRegistry + its unified own-vtable view CWorkerVtableView.
#include <DDrawMgr/DDrawWorkerRegistry.h>

// CString (4-byte char* wrapper). Only the default ctor + dtor are needed;
// GetNextAssoc writes the key output into it.
#include <Gruntz/String.h>

inline void* operator new(u32, void* p) {
    return p;
}

// The looked-up value: only the scalar-deleting destructor slot (+0x04) is load-
// bearing. Declarations only - never defined, so no ??_7 is emitted here.
class CWorkerValue {
public:
    virtual void GetRuntimeClass();   // [0] 0x1bef01 (shared thunk, declared-only)
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

// A map value as seen by the scan helpers: it exposes a dword at +0x10 (compared
// in FindKeyOfValue_165360) and a non-virtual probe at RVA 0x152xc0 (called from
// AnyValueMatches_155630). Modeled with a typed +0x10 field + the probe method so
// `val->m_frameArrayBase` and `val->Probe(...)` lower to the exact loads/thiscall; the
// probe is declared only (its body is another TU), so it is a reloc-masked call.

// CMapStringToOb lives at CDDrawWorkerRegistry+0x10. Lookup/RemoveKey are out-of-line
// NAFXCW thunks (reloc-masked rel32 calls); declared with the exact MFC signatures
// so clang mangles them to the MFC-canonical names.
#include <Gruntz/MapStringToOb.h>

// Real polymorphic two-level model (ALL-VTABLES mandate): CLoadable carries
// the 9-slot base vtable (masks 0x5efc30), CDDrawWorker adds slots 9..14 and
// carries the 15-slot derived vtable (masks 0x5efbe8). `new CDDrawWorker` makes
// cl auto-emit ??_7CLoadable + ??_7CDDrawWorker and stamp the base vptr
// (base ctor) then the derived vptr (Obj ctor) - no manual `*(void**)w=&g_*Vtbl`.
class CLoadable {
public:
    virtual void GetRuntimeClass();   // [0] 0x1bef01
    virtual i32 ScalarDtor(i32 flag); // [1] 0x155780 scalar-deleting dtor
    virtual void Serialize();         // [2] 0x0028ec
    virtual void AssertValid();       // [3] 0x00106e
    virtual void Dump();              // [4] 0x004034
    virtual void FUN_00555750();      // [5] 0x155750
    virtual void IsValidImage();      // [6] 0x001c08
    virtual void DeleteAll();         // [7] 0x151eb0 (= CDDrawWorker::DeleteAll, other TU)
    virtual void FUN_00555770();      // [8] 0x155770
    CLoadable() {}
};

struct CDDrawWorker : public CLoadable {
    virtual i32 Vfunc24(const char* key);                // [9]  0x155810
    virtual void FUN_005521f0();                         // [10] 0x1521f0
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a4, i32 a5); // [11] 0x152110
    virtual i32 Vfunc30(i32 a1, i32 a2, i32 a4, i32 a5); // [12] 0x152060
    virtual i32 Vfunc34(i32 a1, i32 a3, i32 a4);         // [13] 0x151fb0
    virtual i32 Vfunc38(i32 a1, i32 a3, i32 a4);         // [14] 0x151f00
    virtual void FUN_005522b0();                         // [15] 0x1522b0
    virtual void FUN_005523b0();                         // [16] 0x1523b0
    CDDrawWorker() {}

    i32 m_04;        // +0x04  parent+0x1c
    i32 m_08;        // +0x08  0
    i32 m_0c;        // +0x0c  parent+0x0c
    CByteArray m_10; // +0x10
    char m_pad24[0x64 - 0x24];
    i32 m_64; // +0x64  0x1869f
    i32 m_68; // +0x68  0
}; // 0x6c

DATA(0x002bf37c)
extern i32 g_resourceInstallActive;

// operator delete + the member-teardown host (real ~ at 0x156e10, CDDrawSubMgr.cpp,
// as CDDrawRegistryDtorHost::~) that this class's ??_G scalar-dtor (0x156df0) calls.
void operator delete(void*);
class CDDrawRegistryDtorHost {
public:
    ~CDDrawRegistryDtorHost();
};

// Helpers for Stub_156e80 (0x156e80): a probe chain (0x13b900 -> object, whose
// 0x13a230 yields the result) and the parent's +0x48 vtable dispatch.
class RegProbeChain {
public:
    class RegProbeChain* Get_13b900(i32 a); // 0x13b900
    void* Deref_13a230();                   // 0x13a230
};
DATA(0x006293f4)
extern char g_emptyString[]; // 0x6293f4

// A directory-tree cursor: 0x13a260 (first child), 0x13a280 (next child); each entry's
// +0x00 is a name string.
class RegDirEntry {
public:
    char* m_name; // +0x00
};

// A CDDrawWorker (vtable 0x5efbe8) viewed for the +0x28/+0x3c dispatches + the +0x18
// status field; slots named from their retail slot-function RVAs. Slot28 (0x1521f0)
// and Slot3C (0x1522b0) are the dispatched ops, modeled with the arg arity the
// Stub_154f80/155160 call sites use (the CDDrawWorker slot declarations elsewhere
// model them 0-arg - an arity reconcile for the deferred family-unification pass).
class RegWorkerValue {
public:
    virtual void Slot00_1bef01();        // slot 0  0x1bef01 (CObject thunk)
    virtual void Slot04_155780();        // slot 1  0x155780 (ScalarDtor)
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
                worker->ScalarDtor(1);
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
// Thin forwarder to worker slot +0x34.
RVA(0x00154f00, 0x1b)
i32 CDDrawWorkerRegistry::Forward34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4) {
    return worker->Vfunc34(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x38.
RVA(0x00154f20, 0x1b)
i32 CDDrawWorkerRegistry::Forward38(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4) {
    return worker->Vfunc38(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x30.
RVA(0x00154f40, 0x20)
i32 CDDrawWorkerRegistry::Forward30(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5) {
    return worker->Vfunc30(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x2c.
RVA(0x00154f60, 0x20)
i32 CDDrawWorkerRegistry::Forward2C(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5) {
    return worker->Vfunc2C(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Removes a non-null worker from the map by its key at +0x24, then destroys it.
RVA(0x00155280, 0x22)
void CDDrawWorkerRegistry::RemoveWorker(CDDrawWorker* worker) {
    if (worker != 0) {
        m_map.RemoveKey((const char*)((char*)worker + 0x24));
        worker->ScalarDtor(1);
    }
}

// ---------------------------------------------------------------------------
// Constant state id.
RVA(0x00156de0, 0x6)
StateId CDDrawWorkerRegistry::GetStateId() {
    return STATE_WORKERREGISTRY; // 0x12
}

// ---------------------------------------------------------------------------
// Same base readiness predicate used by several CDDrawSubMgr-derived managers.
RVA(0x001576d0, 0x16)
i32 CDDrawWorkerRegistry::IsReady() {
    if (m_0c == 0) {
        goto fail;
    }
    if (m_status != -1) {
        return 1;
    }

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// Lookup `key` in the map; if found, RemoveKey it and run the value's scalar-
// deleting destructor (vtbl +0x4, arg 1).
//
// RESIDUE (~77.5%, NOT a logic/offset/type/CFG error - register-allocation +
// store/load-scheduling entropy, matching-patterns.md / match-learnings.md
// "register-alloc coin-flip"): the logic, CFG, the val=0 init, both library calls,
// their args, and the dtor dispatch are all reproduced; only the register schedule
// the allocator picked differs. The target holds `key` in EDI and keeps `val`
// purely on the stack, reloading it (`mov ecx,[esp+8]`) AFTER RemoveKey and
// branching on Lookup's return in EAX. MSVC5 on this (isolated) TU instead caches
// `key` in EBX and `val` in EDI across the calls and branches on the cached val -
// equivalent codegen, an extra callee-saved register, a different ecx/edx pairing.
// Every source form tried (if(Lookup(...)) / if(...!=0) / captured `int found` /
// volatile val) produced the identical schedule; the surrounding symbol-set is what
// re-rolls the allocation, so no in-function lever flips it. Left as the plateau.
RVA(0x00156ec0, 0x40)
void CDDrawWorkerRegistry::RemoveByKey(const char* key) {
    CObject* val = 0;
    if (m_map.Lookup(key, val)) {
        m_map.RemoveKey(key);
        ((CWorkerValue*)val)->ScalarDtor(1);
    }
}

// ---------------------------------------------------------------------------
// Map teardown: iterate all entries in m_map via GetNextAssoc, destroying each
// CObject* value via its scalar-deleting destructor (vtbl +0x4 arg 1), then
// RemoveAll the map. Same pattern as CDDrawWorkerMapSmall::DestroyAll but
// without the final m_64 clear (that field does not exist in this class).
//
// Carries a /GX EH frame for the local CString key (destructor must fire on
// unwind through the iteration loop).
RVA(0x00165210, 0xa2)
void CDDrawWorkerRegistry::DestroyAll() {
    CObject* val = 0;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                ((CWorkerValue*)val)->ScalarDtor(1);
            }
        } while (pos != 0);
    }
    m_map.RemoveAll();
}

// ---------------------------------------------------------------------------
// Map teardown leaf (SEH)
// Iterates m_map via GetNextAssoc, destroys each CObject* value via
// scalar-deleting dtor (vtbl+0x4 arg 1), then RemoveAll.
RVA(0x001552b0, 0xa2)
void CDDrawWorkerRegistry::MapTeardown_1552b0() {
    CObject* val = 0;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                ((CWorkerValue*)val)->ScalarDtor(1);
            }
        } while (pos != 0);
    }
    m_map.RemoveAll();
}

// ---------------------------------------------------------------------------
// String copy leaf
// Copies at most 0x3F bytes from src into this+0x24, null-terminates at
// this+0x63, returns 1.
RVA(0x00155810, 0x23)
i32 CDDrawWorkerRegistry::StringCopy_155810(const char* src) {
    strncpy((char*)this + 0x24, src, 0x3f);
    *((char*)this + 0x63) = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// Map scan: remove every entry whose key strncmp-equals `str` (over its full
// length), destroying each removed value via its scalar dtor; returns the count.
// The compare string is a CString built from `base` then assigned `str`.
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
                ((CWorkerValue*)val)->ScalarDtor(1);
            }
            ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// Map scan: sum each non-null value's ComputeSize_1523f0(a2) over the entries
// whose key strncmp-matches `str` (a null/empty `str` matches every entry).
// Carries a /GX EH frame for the local CString key.
// @early-stop
// zero-register-pin wall (~69%): map-scan idiom applied (GetStartPosition + the
// str==0||*str==0||strncmp==0 shared-add || chain replaces the diverging matched-
// bool neg/sbb/inc; docs/patterns/mfc-map-walk-while-not-guard-dowhile.md). The
// GetNextAssoc scan, match-all guard, strlen+strncmp compare, and per-value
// ComputeSize_1523f0 thiscall accumulation all reproduced. Residue: retail pins 0
// in edi (xor edi,edi + cmp edi,X) and holds a2 in ebx across the body where our
// cl uses test/immediate + a per-call a2 reload - regalloc coin-flip, no source
// lever. docs/patterns/zero-register-pinning.md.
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
// Map scan: probe every non-null value via its Probe_1525c0(a1,a2,a3); return 1
// on the first that returns nonzero, else 0. Returns 0 immediately if a1 is null.
// @early-stop
// regalloc wall (~75.9%) - complete & correct: a1==0 guard, the per-value
// Probe_1525c0 thiscall, the loop CFG all reproduced. Residue is the same
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
// Map scan: return (by value) the key of the first entry whose value's +0x10
// dword equals target's +0x10; empty CString if none. Closed by the map-scan idiom
// (top-tested while + real GetStartPosition) plus spelling the no-match return as a
// named `CString empty; return empty;` so cl materializes the empty temp + copy-
// ctor exactly as retail (a bare `return CString()` RVOs it into the return slot).
RVA(0x00165360, 0xf1)
CString CDDrawWorkerRegistry::FindKeyOfValue_165360(CImageSet* target) {
    CObject* val = 0;
    POSITION pos = m_map.GetStartPosition();
    CString key;
    while (pos != 0) {
        m_map.GetNextAssoc(pos, key, val);
        if (val != 0 && *(i32*)((CImageSet*)val)->m_array == *(i32*)target->m_array) {
            return key;
        }
    }
    CString empty;
    return empty;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// 0x154f80: walk the directory tree under `dir`; for each entry build a path string
// ("<sub><prefix><name>" when sub is set, else just the name) into a 0x100 heap buffer
// and accumulate this->+0x48(entry, buf, prefix). Then, when sub is set, find-or-create
// the keyed worker, dispatch its +0x28(dir), and either run this->+0x54(sub) (worker
// inactive) or bump the count. /GX EH frame for the partially-built worker.
// @early-stop
// worker-ctor + regalloc wall: the directory walk / sprintf-vs-strcpy / +0x48 dispatch
// / find-or-create (CDDrawWorker) / +0x28 / status branch are reproduced; retail's
// inline worker construction seeds fields before the CByteArray ctor + stamps the
// derived vtable last, and the buffer/entry register schedule differs. Reloc-masked
// EH-state + map/thunk names. Logic/CFG/offsets complete.
RVA(0x00154f80, 0x1d5)
i32 CDDrawWorkerRegistry::Stub_154f80(CSymTab* dir, const char* sub, const char* prefix) {
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
// 0x155160: the read-side twin of Stub_154f80 - walk the directory tree, build the
// same path string, and accumulate this->+0x4c(entry, buf, prefix) (a negative result
// aborts to -1). Then, when sub is set, Lookup it in the map; if present, dispatch its
// +0x3c(dir) (a -1 aborts to -1) and bump the count when the value's +0x18 is positive.
// No EH frame (plain /O2 leaf). Returns the accumulated count.
// @early-stop
// regalloc/loop-schedule wall: the walk / sprintf-vs-strcpy / +0x4c dispatch + <0
// abort / Lookup / +0x3c dispatch + -1 abort / +0x18 count are reproduced; the
// buffer/entry/count register schedule + reloc-masked thunk names are the residual.
RVA(0x00155160, 0x11e)
i32 CDDrawWorkerRegistry::Stub_155160(CSymTab* dir, const char* sub, const char* prefix) {
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
// 0x156df0: ??_G scalar-deleting destructor - run the real member-teardown ~
// (0x156e10, CDDrawSubMgr.cpp as CDDrawRegistryDtorHost::~) then operator delete.
SYMBOL(??_GCDDrawRegistryDtorHost @@UAEPAXI@Z)
RVA(0x00156df0, 0x1e)
void* CDDrawWorkerRegistry::Stub_156df0(i32 flag) {
    ((CDDrawRegistryDtorHost*)this)->CDDrawRegistryDtorHost::~CDDrawRegistryDtorHost();
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x156e80: probe `arg1` through 0x13b900(arg2) -> object, deref via 0x13a230; if
// the result is non-null, dispatch this->+0x48 with (result, g_emptyString,
// &g_dat60b588) and return it, else 0. __thiscall, 2 args (ret 8).
RVA(0x00156e80, 0x38)
i32 CDDrawWorkerRegistry::Stub_156e80(RegProbeChain* arg1, i32 arg2) {
    RegProbeChain* obj = arg1->Get_13b900(arg2);
    void* result = obj->Deref_13a230();
    if (result == 0) {
        return 0;
    }
    return ((CWorkerVtableView*)this)->Vfunc48(result, g_emptyString, &g_dat60b588);
}

SIZE_UNKNOWN(CDDrawRegistryDtorHost);
SIZE_UNKNOWN(RegProbeChain);
SIZE_UNKNOWN(RegDirEntry);
SIZE_UNKNOWN(RegDirHandle);
SIZE_UNKNOWN(RegWorkerValue);
SIZE_UNKNOWN(CWorkerMapValue);
SIZE_UNKNOWN(CWorkerValue);
SIZE(CDDrawWorker, 0x6c);
VTBL(CLoadable, 0x001efc30);    // ??_7CLoadable (was g_loadableVtbl, 9 slots)
VTBL(CDDrawWorker, 0x001efbe8); // ??_7CDDrawWorker (was g_ddrawWorkerVtbl, 17 slots)
