// TypeKeyColl.cpp - the shared type-key collection / zDArray engine subsystem.
//
// This is the growable key collection (RTTI CTypeKeyColl, @0x6bf650) and its
// zDArray-style allocating base that back the per-class type registries used by
// CKitchenSlime / CProjectile / CWarlord (each models the *registry* side in its
// own TU; the collection's own ctors / binary-search / global dynamic-init live
// here). All callees into the deeper engine (the base ctor, the error/insert
// helper, the CRT alloc/free) are external no-body so their call rel32 / the
// vtable + global DIR32 stores reloc-mask in objdiff.
//
// Functions (retail-RVA order):
//   CTypeKeyColl::CTypeKeyColl   0x16dda0  (derived ctor; calls the 2D-array ctor)
//   CZArray2D::CZArray2D         0x16de30  (allocating ctor, /GX EH frame)
//   CKeyFinder::CKeyFinder       0x16e1a0  (the binary-search cursor ctor)
//   CKeyFinder::Find             0x16e1d0  (binary search over g_keyArray)
//   ProjTypeXfer                 0x16e4f0  (archive serialize of the resolved entry)
//   `dynamic initializer for g_buteTree' 0x16e6a0
//   `dynamic initializer for g_typeColl' 0x16e730
//   CButeTree::`scalar deleting destructor' 0x16e9c0
#include <Mfc.h>
#include <Bute/ButeTree.h> // canonical CButeTree / CVariantSlot (one shape)
#include <rva.h>
#include <stdlib.h> // malloc (0x120b60)
#include <string.h> // memset

#include <Gruntz/StringNode.h>    // the type-name teardown slot
#include <Gruntz/TypeKeyColl.h>   // CZErrSink/CZArrayRoot/CZArray2D/CTypeKeyColl (one shape)
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Gruntz/XferArchive.h>   // canonical CXferArchive/CXferField (ProjTypeXfer arg)
#include <Globals.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

// ===========================================================================
// Vtables (UNMATCHED engine tables - stamped by address, reloc-masked DIR32).
//
// The CZArrayRoot/CZArray2D/CTypeKeyColl vtables (0x5f04cc/0x5f04d4/0x5f04d0) are
// NO LONGER externs: that 1-slot-each construction hierarchy is now modeled as
// REAL polymorphic C++ (virtual dtor per level, below), so cl emits the implicit
// ??_7 vptr stamp in each ctor (reloc-masked) instead of a manual stamp.
// ===========================================================================
// g_keyFinderVtbl is NOT a vtable: 0x16e220 is a FUNCTION in .text (the default
// callback the CKeyFinder/CVariantSlot +0x00 slot is seeded with) - stored as a
// plain fn-ptr field init, not a polymorphic vptr, so it stays a manual store.
DATA(0x0016e220)
extern void* g_keyFinderVtbl; // CKeyFinder +0x00 default callback fn (in .text)
DATA(0x001f04e0)
extern void* g_buteTreeVtbl; // g_buteTree runtime vtable (dyn-init stamp)
DATA(0x001f04dc)
extern void* g_buteTreeSubVtbl; // g_buteTree +0x08 sub-object vtable
DATA(0x001f04e4)
extern void* g_typeCollRunVtbl; // g_typeColl runtime vtable (dyn-init stamp)
DATA(0x001e94ac)
extern void* g_buteTreeDtorVtbl; // CButeTree dtor-phase vtable
DATA(0x001e949c)
extern void* g_buteTreeDtorSubVtbl; // CButeTree dtor-phase +0x08 vtable

// ===========================================================================
// The registry globals (BSS / .data; DATA-pinned so the loads reloc-mask).
// ===========================================================================
extern void* g_retAddrBreadcrumb; // 0x6bf428
DATA(0x002bf464)
extern void* g_projActCache; // 0x6bf464
DATA(0x002bf658)
extern i32 g_typeLo;
DATA(0x002bf65c)
extern i32 g_typeHi;
DATA(0x002bf660)
extern char* g_typeBase;
DATA(0x002bf668)
extern i32 g_typeStride;
DATA(0x002bf670)
extern i32 g_typeCount;
DATA(0x002bf66c)
extern void* g_typeNodes;

// The deeper-base ctor argument (a data tag global at 0x6bf468).

// The "Inconsistent bounds" / "out of memory" message strings are emitted as
// literals (their DIR32 references reloc-mask against the retail $SG symbols).

// ===========================================================================
// External engine leaves (no body - call rel32 reloc-masks).
// ===========================================================================
// CZErrSink (the fatal-alloc/bounds error sink stored at +0x04) is the shared
// <Gruntz/TypeKeyColl.h> shape.
// The CKSlimeColl2 dispatcher the type-name lookup grows the collection through.
DATA(0x002bf654)
extern CVariantSlot* g_typeColl2;

// The zDArray construction hierarchy CZArrayRoot <- CZArray2D <- CTypeKeyColl and
// the leaf ??_7CTypeKeyColl @0x5f04d0 are the shared <Gruntz/TypeKeyColl.h> shape.
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl; // 0x6bf650

extern "C" void* GetCallerRetAddr(); // 0x16e0f0 (records the fatal context)
extern "C" void RezFree(void* p);    // 0x1b9b82 (engine operator delete / free)
extern void* GetRetAddr();           // 0x16d990 (the _ReturnAddress breadcrumb capture)

// The CString slot teardown the node-array free loop walks (one per 4-byte slot).
// ===========================================================================
// CTypeKeyColl::CTypeKeyColl (0x16dda0) - the derived ctor. Forwards the four
// arguments to the 2D-array base ctor (0x16de30), then derives the cursor (==
// the primary buffer) and the element count (hi - lo + 1). cl emits the implicit
// ??_7CTypeKeyColl vptr stamp (was `*(void**)this = &g_typeKeyCollVtbl`). No EH
// frame of its own (the base ctor owns the unwind state).
// ===========================================================================
RVA(0x0016dda0, 0x3c)
CTypeKeyColl::CTypeKeyColl(i32 stride, i32 lo, i32 hi, void* scratch)
    : CZArray2D(stride, lo, hi, scratch) {
    m_cursor = m_buf;
    m_count = m_hi - m_lo + 1;
}

// ===========================================================================
// CZArray2D::CZArray2D (0x16de30) - the allocating zDArray base ctor. Builds the
// deeper root (the CZArrayRoot base subobject), records the [lo, hi] bounds +
// element stride, allocates the (hi-lo+1)*stride element buffer (+ a scratch
// element when none was supplied), and reports a fatal "Inconsistent bounds" /
// "out of memory" through the owner sink on failure. cl emits the implicit
// ??_7CZArray2D vptr stamp (was `*(void**)this = &g_zArray2DVtbl`) and the /GX
// unwind frame (the partially-built CZArrayRoot subobject must be destroyed if a
// later allocation throws).
//
// @early-stop
// vptr-position wall (~96%, up from 67% as a plain method). Modeling this as a
// real ctor over a destructible CZArrayRoot base recovered the whole /GX state
// frame (push -1 / push handler / fs:0 chain / trylevel write) that the plain
// method could not emit - the bulk of the old gap. Residue: cl schedules the
// implicit ??_7CZArray2D stamp BEFORE the m_lo/m_hi/m_buf/m_stride stores, but
// retail sinks it AFTER them, plus a minor regalloc swap in the lo/hi/stride/
// scratch load sequence. Not source-steerable; deferred to the final sweep.
RVA(0x0016de30, 0xe7)
CZArray2D::CZArray2D(i32 stride, i32 lo, i32 hi, void* scratch)
    : CZArrayRoot(&g_zArrayTag) { // 0x16d9c0
    m_buf2 = scratch;
    m_lo = lo;
    m_hi = hi;
    m_buf = 0;
    m_stride = stride;
    if (lo > hi) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        ((CVariantSlot*)m_owner)->Set((void*)this, (i32) "Inconsistent bounds", 0x16);
        return;
    }
    i32 total = (hi - lo + 1) * stride;
    void* buf = malloc(total);
    m_buf = buf;
    if (buf != 0) {
        memset(buf, 0, total);
        if (m_buf2 != 0) {
            return;
        }
        m_buf2 = malloc(m_stride);
        if (m_buf2 != 0) {
            return;
        }
    }
    g_retAddrBreadcrumb = GetCallerRetAddr();
    ((CVariantSlot*)m_owner)->Set((void*)this, (i32) "out of memory", 0xc);
}

// ===========================================================================
// CKeyFinder - the binary-search cursor over the sorted key array. Its ctor
// (0x16e1a0) seeds the two small constant fields and stows the owner; Find
// (0x16e1d0) bisects g_keyArray (12-byte records, g_keyCount entries) for `key`,
// recording the found index (or the insertion point) into +0x04.
// ===========================================================================
SIZE_UNKNOWN(CKeyFinder);
struct CKeyFinder {
    void* m_vtbl;            // +0x00
    i32 m_index;             // +0x04  found index / insertion point
    u16 m_08;                // +0x08
    u16 m_0a;                // +0x0a  (padding)
    i32 m_0c;                // +0x0c  = 2
    i32 m_10;                // +0x10  = 2
    void* m_owner;           // +0x14
    CKeyFinder(void* owner); // 0x16e1a0
    i32 Find(i32 key);       // 0x16e1d0
};

// The sorted key array + its element count (12-byte records).
extern i32 g_keyArray[];
DATA(0x002bf618)
extern i32 g_keyCount;

RVA(0x0016e1a0, 0x23)
CKeyFinder::CKeyFinder(void* owner) {
    m_0c = 2;
    m_10 = 2;
    *(void**)this = &g_keyFinderVtbl;
    m_08 = 0;
    m_owner = owner;
}

RVA(0x0016e1d0, 0x4b)
i32 CKeyFinder::Find(i32 key) {
    i32 hi = g_keyCount - 1;
    i32 lo = 0;
    if (hi >= 0) {
        do {
            i32 mid = (lo + hi) / 2;
            m_index = mid;
            i32 d = g_keyArray[mid * 3] - key;
            if (d < 0) {
                lo = mid + 1;
            } else if (d <= 0) {
                return mid;
            } else {
                hi = mid - 1;
            }
        } while (lo <= hi);
    }
    m_index = hi + 1;
    return -1;
}

// ===========================================================================
// ProjTypeXfer (0x16e4f0) - serialize the type-name table entry resolved from an
// archive record through the archive's slot dispatches. Resolves the entry id
// (ar->m_14->m_1c) to its CTypeKeyColl entry (the inlined fast-range / Find /
// grow lookup), frees the stale node array, then xfers the entry name (slot
// +0x0c) and id (slot +0x10); a second identical resolve xfers the name through
// slot +0x14. Returns 1.
// ===========================================================================
// The archive record (`ar`) the serializer drives is the canonical CXferArchive
// (<Gruntz/XferArchive.h>, included above).
DATA(0x002bf664)
extern CTypeNameEntry* g_typeCur;

// The inlined type-id -> entry resolution (== KitchenSlime/Projectile TypeLookup).
static inline char* TypeResolve(i32 key) {
    g_typeCount = 0;
    if (key >= g_typeLo && key <= g_typeHi) {
        return g_typeBase + (key - g_typeLo) * g_typeStride;
    }
    if ((i32)((_zvec*)&g_typeColl)->GrowTo(key, 0)) {
        return g_typeBase + (key - g_typeLo) * g_typeStride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl2->Set(&g_typeColl, (i32)item, 0xc);
    return (char*)g_typeCur;
}

// Free the stale node array (g_typeCount slots, walking g_typeNodes).
static inline void FreeNodes() {
    CStringNode* nodes = (CStringNode*)g_typeNodes;
    i32 cnt = g_typeCount;
    if (cnt != 0) {
        do {
            if (nodes != 0) {
                nodes->Free();
            }
            ++nodes;
        } while (--cnt);
    }
}

// @early-stop
// register-scheduling wall: the two inlined TypeResolve copies + the node-free
// count-down loops + the interleaved archive slot dispatches pin the saved regs in
// a spill order MSVC reproduces only for one allocation; logic + offsets + the
// inlined lookup + the slot conventions are byte-faithful. Deferred to the final sweep.
RVA(0x0016e4f0, 0x19b)
i32 ProjTypeXfer(CXferArchive* ar) {
    CTypeNameEntry* entry = (CTypeNameEntry*)TypeResolve(ar->m_14->m_1c);
    FreeNodes();
    ar->Xfer0c(entry->m_name.GetBuffer(0)); // 0x1ba11c ?GetBuffer@CString@@QAEPADH@Z
    ar->Xfer10(ar->m_14->m_1c);

    entry = (CTypeNameEntry*)TypeResolve(ar->m_14->m_1c);
    FreeNodes();
    ar->Xfer14(entry->m_name.GetBuffer(0));
    return 1;
}

// ===========================================================================
// `dynamic initializer for g_buteTree' (0x16e6a0) - construct the global bute
// tree (deeper base ctor 0x16dff0) then stamp its runtime vtables.
// ===========================================================================
// g_buteTree is the canonical CButeTree (crit-bit trie, include/Bute/ButeTree.h);
// Construct (0x16dff0) runs the deeper base ctor. The +0x00 / +0x08 runtime vptrs are
// stamped via raw casts (CButeTree is now real-polymorphic MI, no manual vptr fields).
DATA(0x002bf620)
extern CButeTree g_buteTree;

RVA(0x0016e6a0, 0x26)
void DynInitButeTree() {
    g_buteTree.Construct(&g_buteTreeArg, 0);
    *(void**)&g_buteTree = &g_buteTreeVtbl;
    *(void**)((char*)&g_buteTree + 8) = &g_buteTreeSubVtbl;
}

// Placement new (construct g_typeColl in place; no allocation, so it just runs the
// CTypeKeyColl ctor on the existing global, exactly as the retail in-place build).
inline void* operator new(u32, void* p) {
    return p;
}

// ===========================================================================
// `dynamic initializer for g_typeColl' (0x16e730) - construct the shared key
// collection with the [0x7d0, 0x7da] id range, stamp its runtime vtable, then
// free the (initially stale) node array. The construct is a placement-new of the
// real CTypeKeyColl ctor (0x16dda0) over the global; the runtime re-stamp swaps in
// the live g_typeCollRunVtbl over the just-built construction vtable.
// ===========================================================================
// @early-stop
// placement-new null-guard + count-down-induction wall (~70%). Two residues:
// (1) constructing g_typeColl now goes through the REAL CTypeKeyColl ctor via a
// placement-new (the only way to invoke a ctor on the pre-pinned extern global);
// MSVC5 emits the placement null-guard `mov eax,&g_typeColl; test eax,eax; je`
// that retail's direct in-place build lacks - intrinsic to placement-new of a
// non-trivial ctor, not source-steerable. (2) the node-free loop: retail
// materializes the counter via the `ecx=cnt; eax=cnt-1; lea edi,[eax+1]`
// strength-reduced idiom and shrink-wraps `push edi`; cl loads the count plainly.
// Same not-source-steerable idiom as CKitchenSlime/CProjectile::RegisterType.
// Deferred to the final sweep.
RVA(0x0016e730, 0x51)
void DynInitTypeColl() {
    new (&g_typeColl) CTypeKeyColl(4, 0x7d0, 0x7da, (void*)1);
    CStringNode* nodes = (CStringNode*)g_typeNodes;
    *(void**)&g_typeColl = &g_typeCollRunVtbl;
    if (nodes != 0) {
        i32 cnt = g_typeCount;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    nodes->Free();
                }
                ++nodes;
            } while (--cnt);
        }
    }
}

// ===========================================================================
// CButeTree::`scalar deleting destructor' (0x16e9c0) - restamp the dtor-phase
// vtables, run the member + base teardown, and free the object when bit0 of the
// flag is set. Returns `this`.
// ===========================================================================
RVA(0x0016e9c0, 0x45)
void* CButeTree::ScalarDtor(u32 flags) {
    *(void**)this = &g_buteTreeDtorVtbl;
    *(void**)((char*)this + 8) = &g_buteTreeDtorSubVtbl;
    ClearRecursive(0);
    ((CButeTreeBase2*)(this != 0 ? (char*)this + 8 : 0))->Dtor();
    BaseDtor();
    if (flags & 1) {
        RezFree(this);
    }
    return this;
}

// ===========================================================================
// 0x16d000 - config field loader.  __cdecl(reader, data): pulls 29 doubles and
// one int out of the reader into the data block at fixed offsets, returning the
// reader.  Models the reader's per-field accessors (0x191fe0 double / 0x191f30
// int, both __thiscall) as reloc-masked no-body methods.
// ===========================================================================
SIZE_UNKNOWN(CConfigReader);
class CConfigReader {
public:
    void ReadDouble(void* field); // 0x191fe0
    void ReadInt(void* field);    // 0x191f30
};

RVA(0x0016d000, 0x189)
CConfigReader* LoadConfigFields(CConfigReader* r, char* d) {
    r->ReadDouble(d + 0x00);
    r->ReadDouble(d + 0x08);
    r->ReadDouble(d + 0x10);
    r->ReadDouble(d + 0x18);
    r->ReadDouble(d + 0x20);
    r->ReadDouble(d + 0x28);
    r->ReadDouble(d + 0x30);
    r->ReadDouble(d + 0x38);
    r->ReadDouble(d + 0x40);
    r->ReadDouble(d + 0x48);
    r->ReadDouble(d + 0x50);
    r->ReadDouble(d + 0x70);
    r->ReadDouble(d + 0x78);
    r->ReadDouble(d + 0x80);
    r->ReadDouble(d + 0x88);
    r->ReadDouble(d + 0x90);
    r->ReadDouble(d + 0x98);
    r->ReadDouble(d + 0xa0);
    r->ReadDouble(d + 0xa8);
    r->ReadDouble(d + 0xb0);
    r->ReadInt(d + 0xb8);
    r->ReadDouble(d + 0xc0);
    r->ReadDouble(d + 0xc8);
    r->ReadDouble(d + 0xd0);
    r->ReadDouble(d + 0xd8);
    r->ReadDouble(d + 0xe0);
    r->ReadDouble(d + 0xe8);
    r->ReadDouble(d + 0xf0);
    r->ReadDouble(d + 0xf8);
    r->ReadDouble(d + 0x100);
    return r;
}

// ===========================================================================
// 0x16d850 - variant/property setter (the HOT helper, ret 0xc).  Switches on the
// slot's type tag (m_0c): 4 -> store the low word directly; otherwise probe the
// global index table (when enabled) and, by tag 2/1, either dispatch through the
// resolved table entry's fn / word slot, or (unresolved) format the slot's label
// + invoke the slot's own +0x00 callback / store the word.  The index probe is
// CKeyFinder::Find (0x16e1d0, defined above); the table + gate are globals.
// @early-stop
// 99.85% - reloc-typing / entropy-tail artifact only: the full switch dispatch,
// indexed-table paths, inline strcpy + Format_18d0f0 and the +0x00 callback are
// all byte-exact; the residual is the DIR32-vs-REL32 scoring on the named
// g_varTable / g_varProbeEnabled / Find externs (docs/matching-patterns.md fuzzy%).
// ===========================================================================
// The slot's resolved-index dispatch table (12-byte stride): a __cdecl fn at +0,
// a word slot at +4.  Reloc-masked DATA extern (base 0x6bf49c).
SIZE_UNKNOWN(CVarTableEntry);
struct CVarTableEntry {
    void(__cdecl* fn)(i32 a, i32 b); // +0x00
    u16 w;                           // +0x04
    char m_pad06[12 - 6];            // 12-byte stride
};
DATA(0x002bf49c)
extern CVarTableEntry g_varTable[]; // 0x6bf49c

// The probe-enable gate (a function-ptr/flag global; nonzero -> probe the table).
DATA(0x002bf618)
extern void* g_varProbeEnabled; // 0x6bf618

// The slot label formatter (__cdecl(buf, value, cap)).
extern "C" void Format_18d0f0(char* buf, i32 value, i32 cap); // 0x18d0f0

// CVariantSlot (the +0x00 callback / probe-index / word / type-tag / label slot)
// is the canonical one in <Bute/ButeTree.h>; Set (0x16d850) is defined here.

RVA(0x0016d850, 0x11e)
void CVariantSlot::Set(void* key, i32 arg2, i32 arg3) {
    if (m_0c == 4) {
        m_08 = (u16)arg3;
        return;
    }
    i32 idx;
    if (g_varProbeEnabled != 0) {
        idx = ((CKeyFinder*)this)->Find((i32)key);
    } else {
        idx = -1;
    }
    if (idx == -1) {
        if (m_0c == 2) {
            char buf[0x94];
            strcpy(buf, m_14);
            Format_18d0f0(buf, arg2, 0x4f);
            m_callback(buf, arg3);
        } else if (m_0c == 1) {
            m_08 = (u16)arg3;
        }
    } else {
        if (m_0c == 2) {
            g_varTable[idx].fn(arg2, arg3);
        } else if (m_0c == 1) {
            g_varTable[idx].w = (u16)arg3;
        }
    }
}
