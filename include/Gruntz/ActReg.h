#ifndef GRUNTZ_GRUNTZ_ACTREG_H
#define GRUNTZ_GRUNTZ_ACTREG_H

#include <Bute/ButeTree.h> // CVariantSlot complete (ResolveEntry calls its Set)

#include <rva.h>

#include <Gruntz/TypeKeyColl.h> // the real registry chain (zDArray : _zdvec : _zvec)

struct CActReg : public zDArray {
    // 0x464e0 (defined out-of-line in FortressFlag.cpp, inside that band): the
    // STANDALONE copy of ResolveEntry below - same body. The two big act-register
    // fns (RegisterWarlordActions, RegisterActs_644af0) CALL it via ILT 0x3544
    // (cl's caller-size inline budget declines the expansion there) where the
    // small dispatchers inline ResolveEntry.
    char* Resolve(i32 id);
    // Standalone SLOT-returning ResolveEntry copy in the TriggerMgr band (0x46e0c0):
    // returns &entry-slot; callers deref for the config-name string. Reloc-masked.
    char** ResolveSlot_46e0c0(i32 id);

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

struct CLogicActTable : public CActReg {}; // per-logic-class dispatch tables
struct CLookupColl : public CActReg {};    // the outlined-lookup registries
struct CSiblingActReg : public CActReg {}; // CUserLogic-leaf sibling registries
SIZE_UNKNOWN(CSiblingActReg);
struct CTeleporterActReg : public CActReg {}; // CTeleporter's registry (0x6446b0)
struct CBehindCandyActReg : public CActReg {}; // CBehindCandyAni's registry (0x645f98)
struct CSingleAnimActReg : public CActReg {};  // CSingleAnimation's registry (0x645f70)

#endif // GRUNTZ_GRUNTZ_ACTREG_H
