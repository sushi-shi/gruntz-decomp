// CStateLeaf8cf30.cpp - a CState-derived leaf game-state whose /GX destructor
// (0x8cf30) is an orphan COMDAT. Same EH leaf-dtor archetype as ~CAttract
// (0x8cd90): stamp the derived vtable (0x5e9dfc), run the member teardown (0x1357)
// under the EH frame, then chain the CState vtable (0x5ea21c) + base dtor (0x3f53).
// Placeholder class name; only OFFSETS + code bytes are load-bearing.
#include <Gruntz/CState.h>
#include <rva.h>

class CStateLeaf8cf30 : public CState {
public:
    virtual ~CStateLeaf8cf30(); // 0x8cf30
    void Teardown();            // 0x1357 (member teardown body)
};

RVA(0x0008cf30, 0x55)
CStateLeaf8cf30::~CStateLeaf8cf30() {
    Teardown();
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CStateLeaf8cf30);
