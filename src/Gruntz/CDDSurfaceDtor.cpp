// CDDSurfaceDtor.cpp - ~CDDSurface (0x142a40), the DirectDraw surface wrapper's
// /GX destructor (re-homed from src/Stub/Discovered.cpp, matcher-1). Stamps the
// CPoolItemA base vptr (0x5ef7f0), runs the base teardown (0x13e4d0), then
// destroys the embedded sub-object at +0x94 (dtor 0x1b4f3e). Offsets + code
// bytes are load-bearing; the foreign vtable is stamped by address (reloc-masked).
#include <Ints.h>
#include <rva.h>

// The CPoolItemA base vftable (0x5ef7f0); stamped by address.
struct CPoolItemAVtbl;
extern CPoolItemAVtbl g_poolItemVtbl7f0;

// The embedded sub-object at +0x94; its destructor is the engine helper 0x1b4f3e
// (__thiscall, reloc-masked). Modeled as a destructible value member so the /GX
// member-cleanup falls out at EH state -1.
struct CDDSurfaceSub94 {
    void DtorImpl(); // 0x1b4f3e
    ~CDDSurfaceSub94() {
        DtorImpl();
    }
};

class CDDSurface {
public:
    void* m_vptr; // 0x00 (manual CPoolItemA vptr stamp)
    char m_pad04[0x94 - 0x04];
    CDDSurfaceSub94 m_94; // +0x94 embedded destructible sub-object
    char m_pad95[0x98 - 0x95];

    void BaseTeardown(); // 0x13e4d0 (__thiscall, reloc-masked)
    ~CDDSurface();
};

// @early-stop
// eh-dtor-vptr-stamp-vs-trylevel-order wall (94.0%): body byte-identical except
// the manual `mov [esi],&g_poolItemVtbl7f0` vptr stamp is scheduled AFTER the
// trylevel-0 write, where retail stamps first. The documented steering
// (eh-dtor-implicit-vptr-stamp-first: make the class real-polymorphic so cl
// emits the implicit stamp-first) is unavailable here - the vtable (0x5ef7f0)
// is the foreign CPoolItemA vtable owned by another TU, so a cl-emitted ??_7
// would diverge. Deferred.
RVA(0x00142a40, 0x53)
CDDSurface::~CDDSurface() {
    m_vptr = (void*)&g_poolItemVtbl7f0;
    BaseTeardown();
}
