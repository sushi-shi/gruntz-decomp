// ImageState.h - CImageState, a CState-derived front-end state whose slot-8 loader
// installs the "MENU" image namespace.
//
// Its concrete RTTI name is unrecovered (the 0xa09a0 body is reached non-virtually
// via an ILT thunk), so it is modeled as a minimal CState subclass: the +0x0c view
// (m_c) and +0x2c source (m_2c) are the inherited CDDrawSurfaceMgr/CResSource facets,
// and the per-state image hook is CState's slot 6. Defined in src/Gruntz/StateImages.cpp.
//
// Names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_IMAGESTATE_H
#define GRUNTZ_IMAGESTATE_H

#include <rva.h>
#include <Gruntz/State.h> // the CState base (m_c/m_2c facets, InitVirtual/Vslot06)

class CImageState : public CState {
public:
    i32 LoadStateImages(); // 0xa09a0
};
SIZE_UNKNOWN(CImageState);

#endif // GRUNTZ_IMAGESTATE_H
