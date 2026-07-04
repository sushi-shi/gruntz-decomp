// CStateLeaf8cf30.cpp - CHelpState::~CHelpState, the help-screen game-state's /GX
// destructor (0x8cf30), an orphan COMDAT. IDENTITY RECOVERED: the derived vtable it
// stamps, 0x5e9dfc, is ??_7CHelpState@@6B@ (config/vtable_names.csv 0x1e9dfc). It is
// the true CHelpState : CState dtor; BacklogStateLoaders.cpp models the same
// CHelpState (LoadAssets/RealizeAnchor) standalone in its own TU - the accepted
// dual-view. Same EH leaf-dtor archetype as ~CAttract (0x8cd90): stamp the derived
// vtable, run the member teardown (0x1357) under the EH frame, then chain the CState
// vtable (0x5ea21c) + base dtor (0x3f53). Only OFFSETS + code bytes are load-bearing.
#include <Gruntz/State.h>
#include <rva.h>

class CHelpState : public CState {
public:
    virtual ~CHelpState() OVERRIDE; // 0x8cf30
    void Teardown();                // 0x1357 (member teardown body)
};

RVA(0x0008cf30, 0x55)
CHelpState::~CHelpState() {
    Teardown();
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CHelpState);
