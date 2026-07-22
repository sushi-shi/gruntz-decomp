#ifndef GRUNTZ_GRUNTZ_WWDGAMEREG_H
#define GRUNTZ_GRUNTZ_WWDGAMEREG_H

#include <Ints.h>
#include <rva.h> // SIZE class-metadata macro

class CState;           // +0x2c  current game state (== GameRegistry m_curState; State.h)
class CDDrawSurfaceMgr; // +0x30  the ONE world/resource holder (<DDrawMgr/DDrawSurfaceMgr.h>)
class CGruntSpawnConfig;

class CGruntzMapMgr;   // +0x70  level tile board (the RTTI-real CMapMgr-derived map mgr)
class CSpriteRefTable; // +0x74  sprite/animation ref table (GetSel)
class CBattlezData;    // +0x7c  HUD/score + pickup-stat accumulator (<Gruntz/BattlezData.h>)
struct tagRECT;        // GetMessageBounds in/out (== Win32 RECT)

struct WwdGameReg; // retail's opaque decl tag; no layout

#endif // GRUNTZ_GRUNTZ_WWDGAMEREG_H
