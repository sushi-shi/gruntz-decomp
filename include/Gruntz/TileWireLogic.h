// TileWireLogic.h - light decl of CTileWireLogic::WireTileSwitchLogic (0x6c130,
// __thiscall, callee cleans 0xc; reconstructed in triggermgrgrid). The grunt
// arrival/switch commit path reaches it on the reused registry slot (g_gameReg->m_68),
// which the grunt code otherwise views as CGruntTileMgr - so cast that pointer to
// CTileWireLogic at the call. Decl-only (reloc-masked); the body lives in triggermgrgrid.
#ifndef GRUNTZ_TILEWIRELOGIC_H
#define GRUNTZ_TILEWIRELOGIC_H

#include <Ints.h>

class CTileWireLogic {
public:
    i32 WireTileSwitchLogic(void* trigger, i32 x, i32 y); // 0x6c130
};

#endif // GRUNTZ_TILEWIRELOGIC_H
