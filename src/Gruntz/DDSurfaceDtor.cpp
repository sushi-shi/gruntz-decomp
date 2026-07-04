// CDDSurfaceDtor.cpp - ~CDDSurface (0x142a40), the DirectDraw surface wrapper's
// /GX destructor (re-homed from src/Stub/Discovered.cpp, matcher-1). CDDSurface is a
// pool-item surface deriving from the shared polymorphic base (surface vtable 0x5ef7f0);
// cl emits the implicit vptr stamp (stamp-first), runs the shared base teardown
// FreeSurfaces (0x13e4d0), then destroys the embedded CByteArray sub-object at +0x94
// (dtor 0x1b4f3e). Offsets + code bytes are load-bearing; the emitted ??_7 reloc-masks
// against the shared 0x5ef7f0 vtable.
#include <Ints.h>
#include <rva.h>

// The embedded CByteArray sub-object at base+0x94; its destructor is the engine helper
// 0x1b4f3e (__thiscall, reloc-masked). Modeled as a destructible value member so the /GX
// member-cleanup falls out at EH state -1.
struct CDDSurfaceSub94 {
    void DtorImpl(); // 0x1b4f3e
    ~CDDSurfaceSub94() {
        DtorImpl();
    }
};

// The shared pool-item surface base (vtable 0x5ef7f0) - the same 9-slot base CFileImage /
// CPoolItemBase (sibling TUs) model. Its INLINE virtual dtor is the shared teardown: cl
// emits the implicit vptr stamp (stamp-first), runs FreeSurfaces (slot 4, 0x13e4d0), then
// destroys the +0x94 member. Because the dtor is inline, ~CDDSurface below inlines this
// whole teardown (the derived vptr stamp folds as a dead store, leaving the base 0x5ef7f0
// stamp stamp-first) - retiring the former manual `mov [esi],&g_poolItemVtbl7f0` stamp
// that landed AFTER the trylevel-0 write (the old stamp-order wall).
class CDDSurfacePoolBase {
public:
    virtual ~CDDSurfacePoolBase() {
        FreeSurfaces();
    } // slot 0  (??_G / ~ 0x142a40 in the derived leaf)
    virtual i32 Refresh();       // slot 1  0x13e140
    virtual i32 Init1();         // slot 2  0x13e0a0
    virtual i32 BlitSurf();      // slot 3  0x13e0d0
    virtual void FreeSurfaces(); // slot 4  0x13e4d0 (the shared teardown, reloc-masked)
    virtual i32 v14();           // slot 5  0x1412d0
    virtual i32 v18();           // slot 6  0x141300
    virtual i32 v1c();           // slot 7  0x13f960
    virtual i32 v20();           // slot 8  0x13e2e0

    // implicit vptr        // +0x00
    char m_pad04[0x94 - 0x04];
    CDDSurfaceSub94 m_94; // +0x94 embedded destructible sub-object
    char m_pad95[0x98 - 0x95];
};
SIZE_UNKNOWN(CDDSurfacePoolBase);

// CDDSurface (DIRSURF.CPP): the DirectDraw held-surface wrapper - a pool-item surface.
// Its leaf (non-deleting) destructor inlines the base teardown above.
class CDDSurface : public CDDSurfacePoolBase {
public:
    ~CDDSurface();
};

RVA(0x00142a40, 0x53)
CDDSurface::~CDDSurface() {}

SIZE_UNKNOWN(CDDSurfaceSub94);
