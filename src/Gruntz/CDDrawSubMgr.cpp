#include <rva.h>
// UnknownLucius.cpp - tomalla-named DDraw surface/page-manager shared base
// (CDDrawSubMgr).  This is the polymorphic base for the 10 sub-
// managers (Draco, Hermiona, Hagrid, etc.).  Two functions:
//   ctor  - seeds the three fields + stamps vtable.
//   dtor  - SEH-framed: calls VirtualMethodUnknown1C
//           cleanup, resets fields, chains base dtor.
//
// Field names are tomalla placeholders; only the OFFSETS + the emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// Forward-declare the family manager (root) stored at CGruntzMgr+0x30.
// Full definition lives in HarryPotter.cpp (HarryPotter unit) and in
// structure/managers/ddrawmgr_surface_family.h.
class CDDrawSurfaceMgr;

// The Lucius and CObject vtables are used in the dtor vtable chain, emitted
// automatically by the compiler.
class CDDrawSubMgrBase {
public:
    CDDrawSubMgrBase() {}
    CDDrawSubMgrBase(int x) { m_fieldBaseUnknown = x; }
    virtual ~CDDrawSubMgrBase() {}
    int m_fieldBaseUnknown;   // +0x04
};

class CDDrawSubMgr : public CDDrawSubMgrBase {
public:
    CDDrawSubMgr(CDDrawSurfaceMgr *pHarryPotter,
                            int unknown2, int unknown3);
    virtual ~CDDrawSubMgr();
    virtual void VirtualMethodUnknown14();
    virtual int  VirtualMethodUnknown18();
    virtual void VirtualMethodUnknown1C();  // cleanup — defined in CDDrawSubMgrDraco.cpp
    virtual void VirtualMethodUnknown20();

    int  fieldUnknown8;                          // +0x08
    CDDrawSurfaceMgr *m_pHarryPotter;  // +0x0c
};

// operator delete (used indirectly via VirtualMethodUnknown1C; may throw -> /GX).
void operator delete(void *);

// ---------------------------------------------------------------------------
// CDDrawSubMgr::CDDrawSubMgr
// Chains the Hogwarts(int) base ctor (inlined: this+0x04 = unknown2), stamps
// the Lucius vtable (compiler-generated), then seeds the remaining fields.
// ---------------------------------------------------------------------------
RVA(0x156cb0, 0x20)
CDDrawSubMgr::CDDrawSubMgr(
    CDDrawSurfaceMgr *pHarryPotter,
    int unknown2, int unknown3)
    : CDDrawSubMgrBase(unknown2)
{
    fieldUnknown8 = unknown3;
    m_pHarryPotter = pHarryPotter;
}

// ---------------------------------------------------------------------------
// CDDrawSubMgr::~CDDrawSubMgr
// Scalar-deleting destructor.  Under /GX the compiler emits a C++ EH frame
// (push -1 / handler info / fs:0) around the body because VirtualMethod-
// Unknown1C may throw (it calls operator delete).  After the body runs, the
// compiler changes the vtable to the base (CObject) and chains
// through the base destructors.
// ---------------------------------------------------------------------------
RVA(0x1574d0, 0x5b)
CDDrawSubMgr::~CDDrawSubMgr()
{
    VirtualMethodUnknown1C();
    m_fieldBaseUnknown = -1;
    fieldUnknown8 = 0;
    m_pHarryPotter = 0;
}

// Out-of-line stubs for unmatched virtuals (anchors the vtable in this TU).
void CDDrawSubMgr::VirtualMethodUnknown14() {}
int  CDDrawSubMgr::VirtualMethodUnknown18() { return 0; }
void CDDrawSubMgr::VirtualMethodUnknown20() {}
