#include <rva.h>
// CDDrawWorkerRegistry.cpp - leaf method(s) of the tomalla-named ddrawmgr surface-family
// sub-manager CDDrawWorkerRegistry (a CDirectDrawMgr surface/page sub-manager in the
// "Harry Potter" family; see src/Stub/types/ddrawmgr_surface_family.h).
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
class CDDrawWorkerRegistry;

// CString (4-byte char* wrapper). Only the default ctor + dtor are needed;
// GetNextAssoc writes the key output into it.
#include <Gruntz/CString.h>

inline void* operator new(u32, void* p) {
    return p;
}

// The looked-up value: only the scalar-deleting destructor slot (+0x04) is load-
// bearing. Declarations only - never defined, so no ??_7 is emitted here.
class SeverusValue {
public:
    virtual void Slot00();            // +0x00
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

// A map value as seen by the scan helpers: it exposes a dword at +0x10 (compared
// in FindKeyOfValue_165360) and a non-virtual probe at RVA 0x152xc0 (called from
// AnyValueMatches_155630). Modeled with a typed +0x10 field + the probe method so
// `val->m_10field` and `val->Probe(...)` lower to the exact loads/thiscall; the
// probe is declared only (its body is another TU), so it is a reloc-masked call.
class SeverusMapValue {
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

class UnknownSeverusVtableView {
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

class SeverusWorker {
public:
    virtual void Slot00();                               // +0x00
    virtual i32 ScalarDtor(i32 flag);                    // +0x04
    virtual void Slot08();                               // +0x08
    virtual void Slot0C();                               // +0x0c
    virtual void Slot10();                               // +0x10
    virtual void Slot14();                               // +0x14
    virtual void Slot18();                               // +0x18
    virtual void Slot1C();                               // +0x1c
    virtual void Slot20();                               // +0x20
    virtual i32 Vfunc24(const char* key);                // +0x24
    virtual void Slot28();                               // +0x28
    virtual i32 Vfunc2C(i32 a1, i32 a2, i32 a4, i32 a5); // +0x2c
    virtual i32 Vfunc30(i32 a1, i32 a2, i32 a4, i32 a5); // +0x30
    virtual i32 Vfunc34(i32 a1, i32 a3, i32 a4);         // +0x34
    virtual i32 Vfunc38(i32 a1, i32 a3, i32 a4);         // +0x38
};

struct SeverusWorkerObj : public SeverusWorker {
    i32 m_04;        // +0x04  parent+0x1c
    i32 m_08;        // +0x08  0
    i32 m_0c;        // +0x0c  parent+0x0c
    CByteArray m_10; // +0x10
    char m_pad24[0x64 - 0x24];
    i32 m_64; // +0x64  0x1869f
    i32 m_68; // +0x68  0
}; // 0x6c

DATA(0x001efc30)
extern void* g_severusWorkerBaseVtbl;
DATA(0x001efbe8)
extern void* g_severusWorkerVtbl;

extern i32 g_severusScratch[25];
DATA(0x002bf37c)
extern i32 g_severusCounterA;
extern i32 g_severusCounterB;

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
    i32 VirtualMethodUnknown28(i32 a1, i32 a2, SeverusWorker* worker, i32 a4, i32 a5);
    i32 VirtualMethodUnknown2C(i32 a1, i32 a2, SeverusWorker* worker, i32 a4, i32 a5);
    i32 VirtualMethodUnknown30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5);
    i32 VirtualMethodUnknown34(i32 a1, SeverusWorker* worker, i32 a3, i32 a4);
    i32 VirtualMethodUnknown38(i32 a1, const char* key, i32 a3, i32 a4);
    i32 VirtualMethodUnknown3C(i32 a1, SeverusWorker* worker, i32 a3, i32 a4);
    i32 VirtualMethodUnknown40(i32 a1, const char* key, i32 a3, i32 a4);
    void VirtualMethodUnknown50(SeverusWorkerObj* worker);
    void VirtualMethodUnknown54(const char* key);
    void VirtualMethodUnknown58();
    void MapTeardown_1552b0();
    i32 StringCopy_155810(const char* src);

    // Map-scan helpers (non-virtual; direct-called from the worker code region).
    i32 RemoveKeysEqual_155360(const char* base, const char* str);
    i32 SumSizesEqual_155460(const char* str, i32 a2);
    i32 HasKeyEqual_155550(const char* str);
    i32 AnyValueMatches_155630(i32 a1, i32 a2, i32 a3);
    CString FindKeyOfValue_165360(SeverusMapValue* target);

    void* m_vptr;              // +0x00
    i32 m_status;              // +0x04  initialized to -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_0c;                  // +0x0c  parent/root handle
    CMapStringToOb m_map;      // +0x10  worker-by-key map

    // Engine-label backlog stubs.
    void Stub_154f80();
    void Stub_155160();
    void Stub_156df0();
    void Stub_156e80();
};

static inline i32 SeverusReadField1c(const CDDrawWorkerRegistry* p) {
    return *(const i32*)((const char*)p + 0x1c);
}

static inline void StampSeverusBaseVtbl(SeverusWorkerObj* w) {
    *(void**)w = &g_severusWorkerBaseVtbl;
}
static inline void StampSeverusVtbl(SeverusWorkerObj* w) {
    *(void**)w = &g_severusWorkerVtbl;
}

static inline SeverusWorkerObj* MakeSeverusWorker(const CDDrawWorkerRegistry* parent) {
    SeverusWorkerObj* raw = (SeverusWorkerObj*)operator new(sizeof(SeverusWorkerObj));
    SeverusWorkerObj* w;
    if (raw != 0) {
        i32 field1c = SeverusReadField1c(parent);
        i32 harryPotter = parent->m_0c;
        StampSeverusBaseVtbl(raw);
        raw->m_04 = field1c;
        raw->m_08 = 0;
        raw->m_0c = harryPotter;
        new (&raw->m_10) CByteArray;
        StampSeverusVtbl(raw);
        raw->m_64 = 99999;
        raw->m_68 = 0;
        w = raw;
    } else {
        w = 0;
    }
    return w;
}

static inline SeverusWorkerObj* FindOrCreateWorker(CDDrawWorkerRegistry* parent, const char* key) {
    CObject* found = 0;
    parent->m_map.Lookup(key, found);
    if (found == 0) {
        SeverusWorkerObj* worker = MakeSeverusWorker(parent);
        if (worker->Vfunc24(key) == 0) {
            if (worker != 0) {
                worker->ScalarDtor(1);
            }
            return 0;
        }
        parent->m_map[key] = (CObject*)worker;
        found = (CObject*)worker;
    }
    return (SeverusWorkerObj*)found;
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
    ((UnknownSeverusVtableView*)this)->Slot58();
    g_severusCounterA = 0;
    g_severusCounterB = 0;
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x38.
RVA(0x00154ae0, 0xfc)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown38(i32 a1, const char* key, i32 a3, i32 a4) {
    SeverusWorkerObj* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc38(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x34.
RVA(0x00154be0, 0xfc)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown40(i32 a1, const char* key, i32 a3, i32 a4) {
    SeverusWorkerObj* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc34(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x30.
RVA(0x00154ce0, 0x101)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    SeverusWorkerObj* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc30(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x2c.
RVA(0x00154df0, 0x101)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown24(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    SeverusWorkerObj* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return worker->Vfunc2C(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x34.
RVA(0x00154f00, 0x1b)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown3C(i32 a1, SeverusWorker* worker, i32 a3, i32 a4) {
    return worker->Vfunc34(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x38.
RVA(0x00154f20, 0x1b)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown34(i32 a1, SeverusWorker* worker, i32 a3, i32 a4) {
    return worker->Vfunc38(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x30.
RVA(0x00154f40, 0x20)
i32 CDDrawWorkerRegistry::VirtualMethodUnknown2C(
    i32 a1,
    i32 a2,
    SeverusWorker* worker,
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
    SeverusWorker* worker,
    i32 a4,
    i32 a5
) {
    return worker->Vfunc2C(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Removes a non-null worker from the map by its key at +0x24, then destroys it.
RVA(0x00155280, 0x22)
void CDDrawWorkerRegistry::VirtualMethodUnknown50(SeverusWorkerObj* worker) {
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
        ((SeverusValue*)val)->ScalarDtor(1);
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
                ((SeverusValue*)val)->ScalarDtor(1);
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
                ((SeverusValue*)val)->ScalarDtor(1);
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
                    ((SeverusValue*)val)->ScalarDtor(1);
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
                    total += ((SeverusMapValue*)val)->ComputeSize_1523f0(a2);
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
            if (val != 0 && ((SeverusMapValue*)val)->Probe_1525c0(a1, a2, a3)) {
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
CString CDDrawWorkerRegistry::FindKeyOfValue_165360(SeverusMapValue* target) {
    CObject* val = 0;
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_map.GetNextAssoc(pos, key, val);
            if (val != 0 && ((SeverusMapValue*)val)->m_10field == target->m_10field) {
                return key;
            }
        } while (pos != 0);
    }
    return CString();
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: tomalla
// @stub
RVA(0x00154f80, 0x1d5)
void CDDrawWorkerRegistry::Stub_154f80() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00155160, 0x11e)
void CDDrawWorkerRegistry::Stub_155160() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00156df0, 0x1e)
void CDDrawWorkerRegistry::Stub_156df0() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00156e80, 0x38)
void CDDrawWorkerRegistry::Stub_156e80() {}

SIZE_UNKNOWN(SeverusMapValue);
SIZE_UNKNOWN(SeverusValue);
SIZE_UNKNOWN(SeverusWorker);
SIZE_UNKNOWN(SeverusWorkerObj);
SIZE_UNKNOWN(UnknownSeverusVtableView);
