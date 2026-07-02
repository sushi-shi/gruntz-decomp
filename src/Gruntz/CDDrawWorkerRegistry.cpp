#include <rva.h>
// CDDrawWorkerRegistry.cpp - leaf method(s) of the tomalla-named ddrawmgr surface-family
// sub-manager CDDrawWorkerRegistry (a CDirectDrawMgr surface/page sub-manager in the
// "DDraw surface manager" family; see src/Stub/types/ddrawmgr_surface_family.h).
//
// CDDrawWorkerRegistry owns a CMapStringToOb at +0x10 (m_unknownMap) keyed by const char*
// strings. VirtualMethodUnknown54 is a keyed remove-and-destroy: Lookup the key in
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
class CDDrawWorkerRegistry;

// CString (4-byte char* wrapper). Only the default ctor + dtor are needed;
// GetNextAssoc writes the key output into it.
#include <Gruntz/CString.h>

inline void* operator new(u32, void* p) {
    return p;
}

// The looked-up value: only the scalar-deleting destructor slot (+0x04) is load-
// bearing. Declarations only - never defined, so no ??_7 is emitted here.
class CWorkerValue {
public:
    virtual void FUN_005bef01();      // [0] 0x1bef01 (shared thunk, declared-only)
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

// A map value as seen by the scan helpers: it exposes a dword at +0x10 (compared
// in FindKeyOfValue_165360) and a non-virtual probe at RVA 0x152xc0 (called from
// AnyValueMatches_155630). Modeled with a typed +0x10 field + the probe method so
// `val->m_10field` and `val->Probe(...)` lower to the exact loads/thiscall; the
// probe is declared only (its body is another TU), so it is a reloc-masked call.
class CWorkerMapValue {
public:
    char m_pad00[0x10];
    i32 m_10field; // +0x10
    i32 Probe_1525c0(i32 a1, i32 a2, i32 a3);
    i32 ComputeSize_1523f0(i32 a1); // 0x1523f0 reloc-masked __thiscall
};

// CMapStringToOb lives at CDDrawWorkerRegistry+0x10. Lookup/RemoveKey are out-of-line
// NAFXCW thunks (reloc-masked rel32 calls); declared with the exact MFC signatures
// so clang mangles them to the MFC-canonical names.
#include <Gruntz/CMapStringToOb.h>

class CWorkerVtableView {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30();
    virtual void Slot34();
    virtual void Slot38();
    virtual void Slot3C();
    virtual void Slot40();
    virtual void Slot44();
    virtual void Slot48();
    virtual void Slot4C();
    virtual void Slot50();
    virtual void Slot54();
    virtual void Slot58();
};

// Real polymorphic two-level model (ALL-VTABLES mandate): CLoadable carries
// the 9-slot base vtable (masks 0x5efc30), CDDrawWorker adds slots 9..14 and
// carries the 15-slot derived vtable (masks 0x5efbe8). `new CDDrawWorker` makes
// cl auto-emit ??_7CLoadable + ??_7CDDrawWorker and stamp the base vptr
// (base ctor) then the derived vptr (Obj ctor) - no manual `*(void**)w=&g_*Vtbl`.
class CLoadable {
public:
    virtual void FUN_005bef01();      // [0] 0x1bef01
    virtual i32 ScalarDtor(i32 flag); // [1] 0x155780 scalar-deleting dtor
    virtual void FUN_004028ec();      // [2] 0x0028ec
    virtual void FUN_0040106e();      // [3] 0x00106e
    virtual void FUN_00404034();      // [4] 0x004034
    virtual void FUN_00555750();      // [5] 0x155750
    virtual void FUN_00401c08();      // [6] 0x001c08
    virtual void FUN_00551eb0();      // [7] 0x151eb0 (DeleteAll, CDDrawWorker other TU)
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
extern i32 g_severusCounterA;

// ---------------------------------------------------------------------------
// CDDrawWorkerRegistry - only the load-bearing offset is modeled: the CMapStringToOb at
// +0x10. The matched method occupies a lower vtable slot (slot number not load-
// bearing, only body), placed last.
// ---------------------------------------------------------------------------
class CDDrawWorkerRegistry {
public:
    i32 VirtualMethodUnknown14();
    i32 VirtualMethodUnknown18();
    void VirtualMethodUnknown1C();
    i32 VirtualMethodUnknown20();
    i32 VirtualMethodUnknown24(i32 a1, i32 a2, const char* key, i32 a4, i32 a5);
    i32 VirtualMethodUnknown28(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5);
    i32 VirtualMethodUnknown2C(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5);
    i32 VirtualMethodUnknown30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5);
    i32 VirtualMethodUnknown34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4);
    i32 VirtualMethodUnknown38(i32 a1, const char* key, i32 a3, i32 a4);
    i32 VirtualMethodUnknown3C(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4);
    i32 VirtualMethodUnknown40(i32 a1, const char* key, i32 a3, i32 a4);
    void VirtualMethodUnknown50(CDDrawWorker* worker);
    void VirtualMethodUnknown54(const char* key);
    void VirtualMethodUnknown58();
    void MapTeardown_1552b0();
    i32 StringCopy_155810(const char* src);

    // Map-scan helpers (non-virtual; direct-called from the worker code region).
    i32 RemoveKeysEqual_155360(const char* base, const char* str);
    i32 SumSizesEqual_155460(const char* str, i32 a2);
    i32 HasKeyEqual_155550(const char* str);
    i32 AnyValueMatches_155630(i32 a1, i32 a2, i32 a3);
    CString FindKeyOfValue_165360(CWorkerMapValue* target);

    void* m_vptr;              // +0x00
    i32 m_status;              // +0x04  initialized to -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_0c;                  // +0x0c  parent/root handle
    CMapStringToOb m_map;      // +0x10  worker-by-key map

    // Engine-label backlog stubs.
    i32 Stub_154f80(class RegDirHandle* dir, const char* sub, const char* prefix);
    i32 Stub_155160(class RegDirHandle* dir, const char* sub, const char* prefix);
    void* Stub_156df0(i32 flag);
    i32 Stub_156e80(class RegProbeChain* a1, i32 a2);
};

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
DATA(0x0060b588)
class RegView48 {
public:
    virtual void s00();
    virtual void s04();
    virtual void s08();
    virtual void s0c();
    virtual void s10();
    virtual void s14();
    virtual void s18();
    virtual void s1c();
    virtual void s20();
    virtual void s24();
    virtual void s28();
    virtual void s2c();
    virtual void s30();
    virtual void s34();
    virtual void s38();
    virtual void s3c();
    virtual void s40();
    virtual void s44();
    virtual i32 Vfunc48(void* a, const char* b, void* c); // +0x48
    virtual i32 Vfunc4C(void* a, const char* b, void* c); // +0x4c
    virtual void s50();
    virtual void Vfunc54(const char* key); // +0x54
};

// A directory-tree cursor: 0x13a260 (first child), 0x13a280 (next child); each entry's
// +0x00 is a name string.
class RegDirEntry {
public:
    char* m_name; // +0x00
};
class RegDirHandle {
public:
    RegDirEntry* First_13a260();              // 0x13a260 (__thiscall)
    RegDirEntry* Next_13a280(RegDirEntry* e); // 0x13a280 (__thiscall, 1 arg)
};

// A worker value viewed for the +0x28/+0x3c dispatches + the +0x18 status field.
class RegWorkerValue {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void Slot28(i32 dir); // +0x28
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual void v38();
    virtual i32 Slot3C(i32 dir); // +0x3c
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
i32 CDDrawWorkerRegistry::VirtualMethodUnknown18() {
    for (i32 i = 0; i < 25; ++i) {
        g_severusScratch[i] = 0;
    }
    g_severusScratch[0] = 100;
    return 1;
}

// ---------------------------------------------------------------------------
// Runs the +0x58 hook, then clears the two counters.
RVA(0x00154ac0, 0x12)
void CDDrawWorkerRegistry::VirtualMethodUnknown1C() {
    ((CWorkerVtableView*)this)->Slot58();
    g_severusCounterA = 0;
    g_severusCounterB = 0;
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x38.
RVA(0x00154ae0, 0xfc)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown38(i32 a1, const char* key, i32 a3, i32 a4) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc38(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x34.
RVA(0x00154be0, 0xfc)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown40(i32 a1, const char* key, i32 a3, i32 a4) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc34(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x30.
RVA(0x00154ce0, 0x101)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc30(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x2c.
RVA(0x00154df0, 0x101)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown24(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc2C(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x34.
RVA(0x00154f00, 0x1b)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown3C(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4) {
    return worker->Vfunc34(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x38.
RVA(0x00154f20, 0x1b)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4) {
    return worker->Vfunc38(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x30.
RVA(0x00154f40, 0x20)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown2C(
    i32 a1,
    i32 a2,
    CDDrawWorker* worker,
    i32 a4,
    i32 a5
) {
    return worker->Vfunc30(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x2c.
RVA(0x00154f60, 0x20)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown28(
    i32 a1,
    i32 a2,
    CDDrawWorker* worker,
    i32 a4,
    i32 a5
) {
    return worker->Vfunc2C(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Removes a non-null worker from the map by its key at +0x24, then destroys it.
RVA(0x00155280, 0x22)
void CDDrawWorkerRegistry::VirtualMethodUnknown50(CDDrawWorker* worker) {
    if (worker != 0) {
        m_map.RemoveKey((const char*)((char*)worker + 0x24));
        worker->ScalarDtor(1);
    }
}

// ---------------------------------------------------------------------------
// Constant state id.
RVA(0x00156de0, 0x6)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown20() {
    return 0x12;
}

// ---------------------------------------------------------------------------
// Same base readiness predicate used by several Lucius-derived managers.
RVA(0x001576d0, 0x16)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown14() {
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
void CDDrawWorkerRegistry::VirtualMethodUnknown54(const char* key) {
    CObject* val = 0;
    if (m_map.Lookup(key, val)) {
        m_map.RemoveKey(key);
        ((CWorkerValue*)val)->ScalarDtor(1);
    }
}

// ---------------------------------------------------------------------------
// Map teardown: iterate all entries in m_map via GetNextAssoc, destroying each
// CObject* value via its scalar-deleting destructor (vtbl +0x4 arg 1), then
// RemoveAll the map. Same pattern as CDDrawWorkerMapSmall::VirtualMethodUnknown1C but
// without the final m_64 clear (that field does not exist in this class).
//
// Carries a /GX EH frame for the local CString key (destructor must fire on
// unwind through the iteration loop).
RVA(0x00165210, 0xa2)
void CDDrawWorkerRegistry::VirtualMethodUnknown58() {
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
// @early-stop
// regalloc wall (~91.7%) - complete & correct: logic/CFG/all calls/args/offsets
// reproduced. Residue is the val/loop-flag stack-slot swap (0x10<->0x14 coin-flip,
// docs/patterns/zero-register-pinning.md) + the reloc-masked EH-state push (0x0 vs
// 0x8, same as the 100%-matched MapTeardown_1552b0). No source lever flips it.
RVA(0x00155360, 0xf8)
i32 CDDrawWorkerRegistry::RemoveKeysEqual_155360(const char* base, const char* str) {
    CString match(base);
    match = str;
    i32 len = match.GetLength();
    i32 n = 0;
    CObject* val = 0;
    CString key;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (strncmp(key, match, len) == 0) {
                m_map.RemoveKey(key);
                if (val != 0) {
                    ((CWorkerValue*)val)->ScalarDtor(1);
                }
                ++n;
            }
        } while (pos != 0);
    }
    return n;
}

// ---------------------------------------------------------------------------
// Map scan: sum each non-null value's ComputeSize_1523f0(a2) over the entries
// whose key strncmp-matches `str` (a null/empty `str` matches every entry).
// Carries a /GX EH frame for the local CString key.
// @early-stop
// regalloc/EH-state wall - complete & correct: the GetNextAssoc scan, the
// str==0/*str==0 match-all guard, the strlen+strncmp compare, and the per-value
// ComputeSize_1523f0 thiscall accumulation are reproduced. Residue is the same
// val/key/loop-flag stack-slot schedule + reloc-masked EH-state push as the
// sibling RemoveKeysEqual_155360. docs/patterns/zero-register-pinning.md.
RVA(0x00155460, 0xe2)
i32 CDDrawWorkerRegistry::SumSizesEqual_155460(const char* str, i32 a2) {
    i32 total = 0;
    CObject* val = 0;
    CString key;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                i32 matched;
                if (str != 0 && *str != 0) {
                    matched = (strncmp(key, str, strlen(str)) == 0);
                } else {
                    matched = 1;
                }
                if (matched) {
                    total += ((CWorkerMapValue*)val)->ComputeSize_1523f0(a2);
                }
            }
        } while (pos != 0);
    }
    return total;
}

// ---------------------------------------------------------------------------
// Map scan: return 1 if any key strncmp-equals `str` over strlen(str), else 0.
// @early-stop
// optimizer loop-peel wall (~61.4%) - complete & correct. MSVC5 peels the first
// iteration of this `do/while + early return` here (the retail body is a single
// loop); body/calls/args match. Every restructure tried (break+flag, slot reorder)
// scored lower. Deferred to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x00155550, 0xdc)
i32 CDDrawWorkerRegistry::HasKeyEqual_155550(const char* str) {
    i32 len = strlen(str);
    CObject* val = 0;
    CString key;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (strncmp(key, str, len) == 0) {
                return 1;
            }
        } while (pos != 0);
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
    CObject* val = 0;
    CString key;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (val != 0 && ((CWorkerMapValue*)val)->Probe_1525c0(a1, a2, a3)) {
                return 1;
            }
        } while (pos != 0);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Map scan: return (by value) the key of the first entry whose value's +0x10
// dword equals target's +0x10; empty CString if none.
// @early-stop
// NRVO-elision wall (~79.1%) - complete & correct: the whole scan + the +0x10
// compare + the `return key` copy-ctor path match byte-for-byte. Residue is only
// the no-match `return CString()` path: retail materializes an empty CString temp
// then copy-constructs the return (no RVO); MSVC5 here elides it into the return
// slot directly. An optimizer choice, not a source-steerable one.
RVA(0x00165360, 0xf1)
CString CDDrawWorkerRegistry::FindKeyOfValue_165360(CWorkerMapValue* target) {
    CObject* val = 0;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (val != 0 && ((CWorkerMapValue*)val)->m_10field == target->m_10field) {
                return key;
            }
        } while (pos != 0);
    }
    return CString();
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
i32 CDDrawWorkerRegistry::Stub_154f80(RegDirHandle* dir, const char* sub, const char* prefix) {
    char* buf = (char*)operator new(0x100);
    i32 count = 0;
    if (buf == 0) {
        return count;
    }
    buf[0] = 0;
    RegDirEntry* e = dir->First_13a260();
    while (e != 0) {
        if (sub != 0 && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        count += ((RegView48*)this)->Vfunc48(e, buf, (void*)prefix);
        e = dir->Next_13a280(e);
    }
    if (sub != 0 && *sub != 0) {
        CDDrawWorker* w = FindOrCreateWorker(this, sub);
        if (w == 0) {
            return 0;
        }
        ((RegWorkerValue*)w)->Slot28((i32)dir);
        if (((RegWorkerValue*)w)->m_18 == 0) {
            ((RegView48*)this)->Vfunc54(sub);
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
i32 CDDrawWorkerRegistry::Stub_155160(RegDirHandle* dir, const char* sub, const char* prefix) {
    char* buf = (char*)operator new(0x100);
    i32 count = 0;
    RegDirEntry* e = dir->First_13a260();
    while (e != 0) {
        if (sub != 0 && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        i32 r = ((RegView48*)this)->Vfunc4C(e, buf, (void*)prefix);
        if (r < 0) {
            operator delete(buf);
            return -1;
        }
        count += r;
        e = dir->Next_13a280(e);
    }
    if (sub != 0 && *sub != 0) {
        CObject* out = 0;
        m_map.Lookup(sub, out);
        if (out != 0) {
            if (((RegWorkerValue*)out)->Slot3C((i32)dir) == -1) {
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
    return ((RegView48*)this)->Vfunc48(result, g_emptyString, &g_dat60b588);
}

SIZE_UNKNOWN(CDDrawRegistryDtorHost);
SIZE_UNKNOWN(RegProbeChain);
SIZE_UNKNOWN(RegView48);
SIZE_UNKNOWN(RegDirEntry);
SIZE_UNKNOWN(RegDirHandle);
SIZE_UNKNOWN(RegWorkerValue);
SIZE_UNKNOWN(CWorkerMapValue);
SIZE_UNKNOWN(CWorkerValue);
SIZE(CDDrawWorker, 0x6c);
SIZE_UNKNOWN(CWorkerVtableView);
VTBL(CLoadable, 0x001efc30);    // ??_7CLoadable (was g_loadableVtbl, 9 slots)
VTBL(CDDrawWorker, 0x001efbe8); // ??_7CDDrawWorker (was g_ddrawWorkerVtbl, 17 slots)
