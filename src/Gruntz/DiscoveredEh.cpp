// DiscoveredEh.cpp - trace-discovered /GX leaf destructors re-homed from
// src/Stub/Discovered.cpp (engine_discovered is base-profile; these dtors carry a
// C++ exception-handling frame, so they need the eh profile). Each placeholder
// class's true name is unknown; only OFFSETS + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// ClassUnknown_55 @0x016460 - a byte-identical twin of ~CImgHolder (0x16500,
// Dialogs.cpp): a derived holder whose /GX dtor frees an embedded CImageList
// (DeleteImageList 0x1c6a5c) between the implicit own vptr stamp (0x5e8cd4) and the
// folded base re-stamp (0x5e8cb4). The empty inline base dtor folds as the LAST
// store; the non-trivial base earns the /GX frame. Real-virtual model
// (docs/patterns/eh-dtor-implicit-vptr-stamp-first.md sub-case 2); cl emits the
// ??_7 stamps, which reloc-mask.
struct CU55Base {
    virtual ~CU55Base() {}
};
struct CU55 : CU55Base {
    void DeleteImageList(); // 0x1c6a5c (NAFXCW CImageList::DeleteImageList)
    virtual ~CU55();        // 0x016460
};
RVA(0x00016460, 0x46)
CU55::~CU55() {
    DeleteImageList();
}

// ---------------------------------------------------------------------------
// CButeStore @0x174d70 - a multiple-inheritance /GX dtor. The most-derived class
// re-stamps its primary vptr (+0, 0x5e94ac) and its secondary base vptr (+8,
// 0x5e949c) at entry, runs ClearRecursive(0) (0x16e070), then destructs its bases
// in reverse declaration order: the secondary base @+8 (~CObj50 0x16dfc0, with the
// `this ? this+8 : 0` adjust) then the primary base @0 (~CContainerErr 0x16da60).
// Real-polymorphic multiple-inheritance model so cl emits both implicit vptr stamps
// + both base-dtor CALLs (all reloc-mask). The non-trivial bases earn the /GX frame.
struct CContainerErr {
    virtual ~CContainerErr(); // 0x16da60 (external)
    char m_pad[8 - 4];        // vptr@+0; CObj50 begins at +8
};
struct CObj50 {
    virtual ~CObj50(); // 0x16dfc0 (external; stamps 0x5f04d8 internally)
};
struct CButeStore : CContainerErr, CObj50 {
    virtual ~CButeStore();    // 0x174d70
    void ClearRecursive(i32); // 0x16e070
};
RVA(0x00174d70, 0x70)
CButeStore::~CButeStore() {
    ClearRecursive(0);
}

SIZE_UNKNOWN(CU55Base);
SIZE_UNKNOWN(CU55);
SIZE_UNKNOWN(CObj50);
