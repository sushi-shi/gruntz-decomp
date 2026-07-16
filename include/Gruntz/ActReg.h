// ActReg.h - the activation-registrar archetype (RTTI zDArray family), the single
// shared shape of the [lo,hi] fast-range coordinate registry the whole engine
// reuses. Instantiated as the per-logic-class dispatch tables, the teleporter /
// checkpoint / secret-level registries, and range-registered from the boundary
// thunk pool. Each concrete registry keeps its own placeholder name (so the
// DATA-pinned global symbols are unchanged) but shares this one definition, so the
// name has a single layout tree-wide.
//
//   ResolveEntry  the inline fast-range + slow Find/GetRetAddr/Insert rebuild lookup
//   RegisterRange (0x3742)   seed the fast [lo,hi] id range
//   Lookup        (0x3864)   the outlined ResolveEntry (used at large call sites)
//   Construct     (0x408710) build the registry over a fixed range
//
// Reuses <Gruntz/ActColl.h> (CActColl/CVariantSlot/GetRetAddr + alloc-scratch globals);
// deliberately does NOT pull the bute-tree / MFC chain, so the boundary-thunk and
// teleporter/checkpoint TUs can share it without a heavy include. Only offsets +
// code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_GRUNTZ_ACTREG_H
#define GRUNTZ_GRUNTZ_ACTREG_H

#include <Bute/ButeTree.h>

class CVariantSlot; // folded CActColl2

#include <rva.h>

#include <Gruntz/ActColl.h> // CActColl/CVariantSlot/GetRetAddr + g_projActCache/g_retAddrBreadcrumb

// The registry IS-A CActColl (its +0x00 collection object is the CActColl base);
// the slow lookup is a direct base Find call, no (CActColl*)this view cast.
struct CActReg : public CActColl {
    // m_coll (+0x00) comes from the CActColl base (Find's this == this).
    CVariantSlot* m_coll2; // +0x04  Insert's this
    i32 m_lo;              // +0x08
    i32 m_hi;              // +0x0c
    char* m_base;          // +0x10
    char* m_cur;           // +0x14  slow-path result slot
    i32 m_stride;          // +0x18
    char m_pad1c[0x20 - 0x1c];
    i32 m_scratch; // +0x20

    // Lookup @0x3864 IS _zvec::IndexToPtr; RegisterRange/Construct = CZDArrayDerived::Construct. Cast at calls.

    // 0x464e0 (defined out-of-line in FortressFlag.cpp, inside that band): the
    // STANDALONE copy of ResolveEntry below - same body. The two big act-register
    // fns (RegisterWarlordActions, RegisterActs_644af0) CALL it via ILT 0x3544
    // (cl's caller-size inline budget declines the expansion there) where the
    // small dispatchers inline ResolveEntry.
    char* Resolve(i32 id);

    char* ResolveEntry(i32 id) {
        m_scratch = 0;
        if (id >= m_lo && id <= m_hi) {
            return m_base + (id - m_lo) * m_stride;
        }
        if ((i32)((_zvec*)this)->GrowTo(id, 0)) {
            return m_base + (id - m_lo) * m_stride;
        }
        void* item = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        m_coll2->Set(this, (i32)item, 0xc);
        return m_cur;
    }
};

// The concrete per-registry instances: same archetype, distinct placeholder names
// so each DATA-pinned global keeps its own symbol. (CActReg itself is used for the
// untyped per-class registries + the boundary range-register thunks.)
struct CLogicActTable : public CActReg {}; // per-logic-class dispatch tables
struct CLookupColl : public CActReg {};    // the outlined-lookup registries
struct CSiblingActReg : public CActReg {}; // CUserLogic-leaf sibling registries
SIZE_UNKNOWN(CSiblingActReg);
struct CTeleporterActReg : public CActReg {}; // CTeleporter's registry (0x6446b0)
struct CCheckpointActReg : public CActReg {}; // CCheckpointTrigger's registry (0x64e7c0)

#endif // GRUNTZ_GRUNTZ_ACTREG_H
