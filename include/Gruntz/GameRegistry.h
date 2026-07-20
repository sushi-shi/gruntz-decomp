#ifndef GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
#define GRUNTZ_GRUNTZ_CGAMEREGISTRY_H

#include <Ints.h>
#include <Gruntz/SoundCue.h>

class CDDrawChildGroup; // +0x30 -> +0x08 the object collection / sprite factory (CreateSprite)
namespace Utils {
    class RegistryHelper; // the +0x38 settings writer (<Utils/RegistryHelper.h>)
}
class CGruntSpawnConfig;
typedef CGruntSpawnConfig CGruntCueSink;    // +0x60 on-screen cue receiver; Grunt.h completes it (or, in
class CState;           // +0x2c current game-state; CState.h completes it
class CWorldSoundSet;   // +0x54 active-level input/spatial-sound object (WorldSoundSet.h)
class CTriggerMgr;
class CBattlezData; // +0x7c the HUD/score accumulator (BattlezData.h completes it)
struct CDDrawSubMgrPages; // +0x30->+0x04 active draw surface (m_drawContext at +0x14)
class CDDrawWorkerRegistry;
typedef CDDrawWorkerRegistry CImageRegistry;
class CSpriteRefTable;
class CLightFxMgr;
#include <DDrawMgr/DDrawSurfaceMgr.h>

#include <Gruntz/TileGrid.h>

class CShadeTableCache; // +0x50 shade-table cache (<DDrawMgr/ShadeTableCache.h>)

#endif // GRUNTZ_GRUNTZ_CGAMEREGISTRY_H
