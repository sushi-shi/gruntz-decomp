#ifndef INCLUDE_GRUNTZ_TRIGGERMGRDTORINLINE_H
#define INCLUDE_GRUNTZ_TRIGGERMGRDTORINLINE_H

#include <rva.h>

#include <Gruntz/TriggerMgr.h>

// The INLINED shape of ~CTriggerMgr. Retail keeps the standalone COMDAT at
// 0x85c50 (TriggerMgr.cpp) for CGruntzMgr::Close and ALSO expands the same body
// at CGruntzMgr::Run's `delete m_cmdGrid` - `call Cleanup`, the vector
// destructor over m_selLists, then m_byteArr / m_recList / m_baseList in
// reverse declaration order (0x83450 @ 0x15cb).
inline CTriggerMgr::~CTriggerMgr() {
    Cleanup();
}

#endif // INCLUDE_GRUNTZ_TRIGGERMGRDTORINLINE_H
