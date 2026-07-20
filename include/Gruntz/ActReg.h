// ActReg.h - the activation-registrar archetype: CActReg IS the real registry class
// (the zDArray : _zdvec : _zvec : zErrHandling chain == retail's RTTI chain
// zDArray<T> : _zdvec : _zvec : zErrHandling, docs/rtti-vtable-catalog.tsv). The
// registries are lazily built .bss singletons (extern + DATA, no CRT init entry -
// only g_typeColl has a real dynamic-init ctor call in retail), constructed through
// the shared non-ctor two-phase Construct (0x8710) and looked up via the inline
// fast-range accessor below. Each concrete registry keeps its own subclass name so
// the DATA-pinned global symbols stay distinct.
#ifndef GRUNTZ_GRUNTZ_ACTREG_H
#define GRUNTZ_GRUNTZ_ACTREG_H

#include <Bute/ButeTree.h> // CVariantSlot complete (ResolveEntry calls its Set)

#include <rva.h>

#include <Gruntz/TypeKeyColl.h> // the real registry chain (zDArray : _zdvec : _zvec)

// The registry IS-A zDArray: fields (m_lo/m_hi/m_base/m_spare/m_stride/m_alloc/
// m_grown + the zErrHandling {vptr, m_errSink} head) and Construct/BaseConstruct/
// GrowTo/Destroy are all inherited from the real chain. This subclass only spells
// the inline fast-range lookup the leaf dispatchers expand.
struct CActReg : public zDArray {
    // 0x464e0 (defined out-of-line in FortressFlag.cpp, inside that band): the
    // STANDALONE copy of ResolveEntry below - same body. The two big act-register
    // fns (RegisterWarlordActions, RegisterActs_644af0) CALL it via ILT 0x3544
    // (cl's caller-size inline budget declines the expansion there) where the
    // small dispatchers inline ResolveEntry.
    char* Resolve(i32 id);

    char* ResolveEntry(i32 id) {
        m_grown = 0;
        if (id >= m_lo && id <= m_hi) {
            return m_base + (id - m_lo) * m_stride;
        }
        if (GrowTo(id, 0)) {
            return m_base + (id - m_lo) * m_stride;
        }
        void* item = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, reinterpret_cast<i32>(item), 0xc);
        return m_spare;
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
struct CBehindCandyActReg : public CActReg {}; // CBehindCandyAni's registry (0x645f98)
struct CSingleAnimActReg : public CActReg {};  // CSingleAnimation's registry (0x645f70)
// (the empty CCheckpointActReg subclass shell is gone - the 0x64e7c0 registry is
// CBrickz's plain CActReg, g_brickzActReg)

#endif // GRUNTZ_GRUNTZ_ACTREG_H
