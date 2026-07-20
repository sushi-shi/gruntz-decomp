#ifndef GRUNTZ_GRUNTZ_BZSTATE_H
#define GRUNTZ_GRUNTZ_BZSTATE_H

#include <Ints.h>
#include <Mfc.h> // CMapStringToPtr (the embedded find table)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created idle-grunt sprites)

#include <rva.h>

class CString; // <Mfc.h>; used by reference in BzState::FormatHudText's declaration

#include <Gruntz/BattlezData.h> // CBattlezData (g_gameReg->m_scoreHud)

#include <DDrawMgr/DDrawSubMgrLeafScan.h> // the real keyed sound-cue cache
#include <Gruntz/LeafCue.h>               // the real cue element

#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)

#endif // GRUNTZ_GRUNTZ_BZSTATE_H
