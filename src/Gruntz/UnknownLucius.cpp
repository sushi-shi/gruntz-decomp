// UnknownLucius.cpp - tomalla-named DDraw surface/page-manager shared base
// (UnknownCGruntzMgrLucius).  This is the polymorphic base for the 10 sub-
// managers (Draco, Hermiona, Hagrid, etc.).  Two functions:
//   ctor  @0x156cb0 (32 B)  - seeds the three fields + stamps vtable.
//   dtor  @0x1574d0 (91 B)  - SEH-framed: calls VirtualMethodUnknown1C
//                              cleanup, resets fields, chains base dtor.
//
// The destructor is annotated as VirtualMethodUnknown14 in the match queue
// (the Ghidra/RTTI name), but its body is a destructor (vtable change +
// cleanup + field reset + base dtor chain).
//
// Field names are tomalla placeholders; only the OFFSETS + the emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// Forward-declare the family manager (root) stored at CGruntzMgr+0x30.
// Full definition lives in HarryPotter.cpp (HarryPotter unit) and in
// structure/managers/ddrawmgr_surface_family.h.
class UnknownClassCGruntzMgrHarryPotter;

// The vtable address for Lucius (0x5efc30) and for CObject (0x5e8cb4) are used
// in the dtor vtable chain, emitted automatically by the compiler.
class UnknownCGruntzMgrHogwarts {
public:
    UnknownCGruntzMgrHogwarts() {}
    UnknownCGruntzMgrHogwarts(int x) { m_fieldBaseUnknown = x; }
    virtual ~UnknownCGruntzMgrHogwarts() {}
    int m_fieldBaseUnknown;   // +0x04
};

class UnknownCGruntzMgrLucius : public UnknownCGruntzMgrHogwarts {
public:
    UnknownCGruntzMgrLucius(UnknownClassCGruntzMgrHarryPotter *pHarryPotter,
                            int unknown2, int unknown3);
    virtual ~UnknownCGruntzMgrLucius();
    virtual void VirtualMethodUnknown14();
    virtual int  VirtualMethodUnknown18();
    virtual void VirtualMethodUnknown1C();  // cleanup — defined in UnknownDraco.cpp
    virtual void VirtualMethodUnknown20();

    int  fieldUnknown8;                          // +0x08
    UnknownClassCGruntzMgrHarryPotter *m_pHarryPotter;  // +0x0c
};

// operator delete (used indirectly via VirtualMethodUnknown1C; may throw -> /GX).
void operator delete(void *);

// ---------------------------------------------------------------------------
// UnknownCGruntzMgrLucius::UnknownCGruntzMgrLucius  @0x156cb0
// Chains the Hogwarts(int) base ctor (inlined: this+0x04 = unknown2), stamps
// the Lucius vtable (compiler-generated), then seeds the remaining fields.
// __thiscall with 3 explicit args (ret 0xc).
//
// @address: 0x156cb0
// @size:    0x20
// ---------------------------------------------------------------------------
UnknownCGruntzMgrLucius::UnknownCGruntzMgrLucius(
    UnknownClassCGruntzMgrHarryPotter *pHarryPotter,
    int unknown2, int unknown3)
    : UnknownCGruntzMgrHogwarts(unknown2)
{
    fieldUnknown8 = unknown3;
    m_pHarryPotter = pHarryPotter;
}

// ---------------------------------------------------------------------------
// UnknownCGruntzMgrLucius::~UnknownCGruntzMgrLucius  @0x1574d0
// Scalar-deleting destructor.  Under /GX the compiler emits a C++ EH frame
// (push -1 / handler info / fs:0) around the body because VirtualMethod-
// Unknown1C may throw (it calls operator delete).  After the body runs, the
// compiler changes the vtable to the base (CObject @0x5e8cb4) and chains
// through the base destructors.
//
// @address: 0x1574d0
// @size:    0x5b
// ---------------------------------------------------------------------------
UnknownCGruntzMgrLucius::~UnknownCGruntzMgrLucius()
{
    VirtualMethodUnknown1C();
    m_fieldBaseUnknown = -1;
    fieldUnknown8 = 0;
    m_pHarryPotter = 0;
}

// Out-of-line stubs for unmatched virtuals (anchors the vtable in this TU).
void UnknownCGruntzMgrLucius::VirtualMethodUnknown14() {}
int  UnknownCGruntzMgrLucius::VirtualMethodUnknown18() { return 0; }
void UnknownCGruntzMgrLucius::VirtualMethodUnknown20() {}
