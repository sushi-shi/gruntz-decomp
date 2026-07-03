// BoundaryLowerDtors.cpp - /GX destructors recovered from the engine_boundary
// backlog (lower half, RVA < 0x133370). Each is a real C++ destructor whose
// destructible base/member subobjects force the synchronous-EH (/GX) frame the
// retail compiler emits (push -1 / push handler / fs:0 chain + state writes). The
// owning class names are placeholders; only OFFSETS + code bytes are load-bearing.
// Vtable stamps + member-dtor callees reloc-mask. (See RezBufferObjectDtor.cpp.)
#include <Ints.h>
#include <rva.h>

// The Rez heap free (0x1b9b82, __cdecl). C++ linkage (NOT extern "C") so MSVC5
// treats it as potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree39f20(void* p); // 0x1b9b82

// ===========================================================================
// 0x039f20 - ~CWorker39f20 (/GX): stamp the derived vtable (0x5e971c), RezFree the
// +0x04 heap buffer, then fold the CObject base subobject (restamp the base dtor
// vtable 0x5e8cb4). Byte-shape identical to ~CRezBufferObject. __thiscall.
// ===========================================================================
struct WorkerBase39f20 {
    virtual ~WorkerBase39f20(); // base vptr @ +0x00 (folds 0x5e8cb4)
};
SIZE_UNKNOWN(WorkerBase39f20);
inline WorkerBase39f20::~WorkerBase39f20() {}
struct CWorker39f20 : WorkerBase39f20 {
    char* m_4; // +0x04  heap buffer
    ~CWorker39f20() OVERRIDE;
};
SIZE_UNKNOWN(CWorker39f20);
RVA(0x00039f20, 0x51)
CWorker39f20::~CWorker39f20() {
    if (m_4) {
        RezFree39f20(m_4);
    }
}

// ===========================================================================
// 0x08c400 - ~CHolder8c400 (/GX): stamp the derived vtable (0x5e8cd4), run the
// +0x00 teardown (0x1c6a5c), then fold the base subobject (restamp 0x5e8cb4).
// __thiscall.
// ===========================================================================
struct WorkerBase8c400 {
    virtual ~WorkerBase8c400(); // base vptr @ +0x00 (folds 0x5e8cb4)
};
SIZE_UNKNOWN(WorkerBase8c400);
inline WorkerBase8c400::~WorkerBase8c400() {}
struct CHolder8c400 : WorkerBase8c400 {
    void Teardown1c6a5c(); // 0x1c6a5c
    ~CHolder8c400() OVERRIDE;
};
SIZE_UNKNOWN(CHolder8c400);
RVA(0x0008c400, 0x46)
CHolder8c400::~CHolder8c400() {
    Teardown1c6a5c();
}

// ===========================================================================
// 0x0390a0 - ~CCredits390a0 (/GX): run the explicit cleanup (0x17b570), then let
// the two owned members at +0x138 (dtor 0x1b4b76) and +0x124 (dtor 0x1bf121) fold
// in reverse declaration order. __thiscall.
// ===========================================================================
struct Member124_390a0 {
    char pad0[0x14];    // 0x124..0x137 (size 0x14)
    ~Member124_390a0(); // 0x1bf121
};
SIZE_UNKNOWN(Member124_390a0);
struct Member138_390a0 {
    char pad0[0x14];
    ~Member138_390a0(); // 0x1b4b76
};
SIZE_UNKNOWN(Member138_390a0);
struct CCredits390a0 {
    void Cleanup17b570();  // 0x17b570
    char pad4[0x124];      // +0x00 .. +0x123
    Member124_390a0 m_124; // +0x124
    Member138_390a0 m_138; // +0x138
    ~CCredits390a0();
};
SIZE_UNKNOWN(CCredits390a0);
RVA(0x000390a0, 0x5d)
CCredits390a0::~CCredits390a0() {
    Cleanup17b570();
}

// ===========================================================================
// 0x08d000 - ~CMenuState8d000 (/GX): stamp the derived vtable (0x5e9d74), run the
// teardown body (0x2919), then fold the CState base subobject (restamp 0x5ea21c +
// base dtor 0x3f53). __thiscall.
// ===========================================================================
struct CStateBase8d000 {
    virtual ~CStateBase8d000(); // base vptr (folds 0x5ea21c), dtor 0x3f53
};
SIZE_UNKNOWN(CStateBase8d000);
struct CMenuState8d000 : CStateBase8d000 {
    void Teardown2919(); // 0x2919
    ~CMenuState8d000() OVERRIDE;
};
SIZE_UNKNOWN(CMenuState8d000);
// @early-stop
// EH-dtor base-stamp idiom wall (95.2%): retail stamps the CState base vtable in
// the DERIVED dtor (mov [esi],0x5ea21c) just before the base cleanup call (0x3f53,
// a cleanup that assumes the vtable already stamped); the polymorphic-base model
// stamps it inside the base dtor instead, so our derived omits that one 6-byte
// store. Derived-vtable stamp, Teardown body, /GX frame + states all byte-faithful.
RVA(0x0008d000, 0x55)
CMenuState8d000::~CMenuState8d000() {
    Teardown2919();
}

// ===========================================================================
// 0x021310 / 0x021570 - ~CButeTree (/GX, multiple inheritance): stamp both base
// vtables (0x5e94ac @+0, 0x5e949c @+8), run the body teardown (0x16e070), then fold
// the +0x08 second base (dtor 0x16dfc0, MI this-adjust null guard) and the +0x00
// first base (dtor 0x16da60). Two distinct derived classes share the base vtables.
// __thiscall.
// ===========================================================================
struct CButeBase1_21 {
    virtual ~CButeBase1_21();   // +0x00 vptr (0x5e94ac), dtor 0x16da60
    void Teardown16e070(i32 z); // 0x16e070
    i32 m_4;                    // +0x04 (pads first base to 8 so the second base lands at +0x08)
};
SIZE_UNKNOWN(CButeBase1_21);
struct CButeBase2_21 {
    virtual ~CButeBase2_21(); // +0x08 vptr (0x5e949c), dtor 0x16dfc0
};
SIZE_UNKNOWN(CButeBase2_21);
struct CButeTree21a : CButeBase1_21, CButeBase2_21 {
    ~CButeTree21a() OVERRIDE;
};
SIZE_UNKNOWN(CButeTree21a);
RVA(0x00021310, 0x70)
CButeTree21a::~CButeTree21a() {
    Teardown16e070(0);
}
struct CButeTree21b : CButeBase1_21, CButeBase2_21 {
    ~CButeTree21b() OVERRIDE;
};
SIZE_UNKNOWN(CButeTree21b);
RVA(0x00021570, 0x70)
CButeTree21b::~CButeTree21b() {
    Teardown16e070(0);
}
