#ifndef GRUNTZ_IMAGESTATE_H
#define GRUNTZ_IMAGESTATE_H

#include <rva.h>
#include <Gruntz/State.h> // the CState base (m_c/m_2c facets, InitVirtual/Vslot06)

class CImageState : public CState {
public:
    i32 LoadStateImages(); // 0xa09a0
};
SIZE_UNKNOWN();

#endif // GRUNTZ_IMAGESTATE_H
