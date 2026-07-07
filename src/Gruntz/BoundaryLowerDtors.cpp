// BoundaryLowerDtors.cpp - /GX destructors recovered from the engine_boundary
// backlog (lower half, RVA < 0x133370). Each is a real C++ destructor whose
// destructible base/member subobjects force the synchronous-EH (/GX) frame the
// retail compiler emits (push -1 / push handler / fs:0 chain + state writes). The
// owning class names are placeholders (declared in <Gruntz/BoundaryLowerDtorsViews.h>);
// only OFFSETS + code bytes are load-bearing. Vtable stamps + member-dtor callees
// reloc-mask. (See RezBufferObjectDtor.cpp.)
#include <Ints.h>
#include <Gruntz/GameModeBase.h>
#include <Gruntz/BoundaryUpper2Views.h>
#include <rva.h>
#include <Gruntz/BoundaryLowerDtorsViews.h> // placeholder /GX dtor classes

// The 0x16e070 leaf is CButeStore::ClearRecursive (header-less butestoreclear class); local decl
// (the CButeStoreNode* arg is load-bearing for the mangled name).
struct CButeStoreNode;
class CButeStore {
public:
    void ClearRecursive(CButeStoreNode* node);
};

// The Rez heap free (0x1b9b82, __cdecl). C++ linkage (NOT extern "C") so MSVC5
// treats it as potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree(void* p); // 0x1b9b82

// ===========================================================================
// 0x039f20 - ~CWorker39f20 (/GX): stamp the derived vtable (0x5e971c), RezFree the
// +0x04 heap buffer, then fold the CObject base subobject (restamp the base dtor
// vtable 0x5e8cb4). Byte-shape identical to ~CRezBufferObject. __thiscall.
// ===========================================================================
RVA(0x00039f20, 0x51)
CWorker39f20::~CWorker39f20() {
    if (m_4) {
        RezFree(m_4);
    }
}

// ===========================================================================
// 0x08c400 - ~CHolder8c400 (/GX): stamp the derived vtable (0x5e8cd4), run the
// +0x00 teardown (0x1c6a5c), then fold the base subobject (restamp 0x5e8cb4).
// __thiscall.
// ===========================================================================
RVA(0x0008c400, 0x46)
CHolder8c400::~CHolder8c400() {
    Teardown1c6a5c();
}

// ===========================================================================
// 0x0390a0 - ~CCredits390a0 (/GX): run the explicit cleanup (0x17b570), then let
// the two owned members at +0x138 (dtor 0x1b4b76) and +0x124 (dtor 0x1bf121) fold
// in reverse declaration order. __thiscall.
// ===========================================================================
RVA(0x000390a0, 0x5d)
CCredits390a0::~CCredits390a0() {
    ((CPageStore17b510*)this)->Close();
}

// ===========================================================================
// 0x08d000 - ~CMenuState8d000 (/GX): stamp the derived vtable (0x5e9d74), run the
// teardown body (0x2919), then fold the CState base subobject (restamp 0x5ea21c +
// base dtor 0x3f53). __thiscall.
// ===========================================================================
// @early-stop
// EH-dtor base-stamp idiom wall (95.2%): retail stamps the CState base vtable in
// the DERIVED dtor (mov [esi],0x5ea21c) just before the base cleanup call (0x3f53,
// a cleanup that assumes the vtable already stamped); the polymorphic-base model
// stamps it inside the base dtor instead, so our derived omits that one 6-byte
// store. Derived-vtable stamp, Teardown body, /GX frame + states all byte-faithful.
RVA(0x0008d000, 0x55)
CMenuState8d000::~CMenuState8d000() {
    ((CGameModeBase*)this)->Reset();
}

// ===========================================================================
// 0x021310 / 0x021570 - ~CButeTree (/GX, multiple inheritance): stamp both base
// vtables (0x5e94ac @+0, 0x5e949c @+8), run the body teardown (0x16e070), then fold
// the +0x08 second base (dtor 0x16dfc0, MI this-adjust null guard) and the +0x00
// first base (dtor 0x16da60). Two distinct derived classes share the base vtables.
// __thiscall.
// ===========================================================================
RVA(0x00021310, 0x70)
CButeTree21a::~CButeTree21a() {
    ((CButeStore*)this)->ClearRecursive(0);
}
RVA(0x00021570, 0x70)
CButeTree21b::~CButeTree21b() {
    ((CButeStore*)this)->ClearRecursive(0);
}
