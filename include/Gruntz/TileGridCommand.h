#ifndef SRC_GRUNTZ_TILEGRIDCOMMAND_H
#define SRC_GRUNTZ_TILEGRIDCOMMAND_H

#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <Wwd/WwdFile.h>          // CDDrawWorkerHost - the canonical plane (the active layer)
#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (Read @+0x2c / Write @+0x30)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created InGameText sprite)
#include <rva.h>                      // SIZE_UNKNOWN class-metadata macros used below

#include <Gruntz/TileTriggerContainer.h>

extern "C" u32 g_frameTime;

struct TgcMap {
    char _pad00[0x5c];
    CDDrawWorkerHost* m_5c; // +0x5c  active layer
};
SIZE_UNKNOWN();

struct TgcGameMgr {
    char _pad00[0x08];
    CDDrawChildGroup* m_08; // +0x08  the sprite factory (CreateSprite @0x1597b0)
    char _pad0c[0x24 - 0x0c];
    TgcMap* m_24; // +0x24  the tile map
};
SIZE_UNKNOWN();

struct TgcRedraw {};
SIZE_UNKNOWN();

#include <Gruntz/TileTriggerLogic.h>

#endif // SRC_GRUNTZ_TILEGRIDCOMMAND_H
