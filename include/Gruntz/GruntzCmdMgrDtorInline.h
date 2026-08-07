#ifndef INCLUDE_GRUNTZ_GRUNTZCMDMGRDTORINLINE_H
#define INCLUDE_GRUNTZ_GRUNTZCMDMGRDTORINLINE_H

#include <rva.h>

#include <Gruntz/GruntzCmdMgr.h>

// The INLINED shape of ~CGruntzCmdMgr. Retail keeps the standalone COMDAT at
// 0x85bd0 (GruntzCmdMgr.cpp) for CGruntzMgr::Close and ALSO expands the same
// body at CGruntzMgr::Run's `delete m_cmdSubMgr` - `call ClearAndReset`, then
// both CPtrList members in reverse declaration order (0x83450 @ 0xff7).
inline CGruntzCmdMgr::~CGruntzCmdMgr() {
    ClearAndReset();
}

#endif // INCLUDE_GRUNTZ_GRUNTZCMDMGRDTORINLINE_H
