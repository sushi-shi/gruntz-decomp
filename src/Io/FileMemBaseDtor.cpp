// FileMemBaseDtor.cpp - the isolated scored copy of CFileMemBase::~CFileMemBase
// (0x001578b0), re-homed from src/Stub/BoundaryTailEh.cpp. The `.cpp`-local
// `struct CFileMemBase` reduced view that used to model it is dissolved onto the
// real polymorphic CFileMemBase (<Io/FileMem.h>).
//
// Split into its OWN TU (the established isolated-dtor pattern, cf. SpotLightCtor.cpp
// / LevelTimeDtor.cpp): the delinker packs a unit's RVA methods into one target
// section and only offset-0 re-pairs cleanly, so homing 0x1578b0 into the multi-fn
// filemem unit re-packs it and collaterally craters CFileMem::~CFileMem
// (59.8%->2%). Isolated here, 0x1578b0 pairs at offset 0. filemem.cpp keeps the
// RVA-less ??_G vtable-anchor copy. The CString m_name member forces the /GX EH frame.
#include <Io/FileMem.h>
#include <rva.h>

// @early-stop
// EH-dtor virtual-dispatch wall (~89%): the base teardown logic is byte-faithful, but
// retail dispatches Reset as `call ds:[0x5efe74]` - an absolute indirect through the
// base vtable slot 3 (+0xc), i.e. a virtual dispatch inside a dtor that MSVC5
// devirtualizes to a direct call from clean C++, so the dispatch byte + the /GX
// trylevel store sequencing diverge. Same wall class as CFileMem::~CFileMem @0x157980.
RVA(0x001578b0, 0x51)
CFileMemBase::~CFileMemBase() {
    Reset();
}
