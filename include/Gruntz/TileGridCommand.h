#ifndef SRC_GRUNTZ_TILEGRIDCOMMAND_H
#define SRC_GRUNTZ_TILEGRIDCOMMAND_H

#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/SerialArchive.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>
#include <rva.h>

#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>

extern "C" u32 g_frameTime;

struct TgcMap {
    char _pad00[0x5c];
    CDDrawWorkerHost* m_5c;
};
SIZE_UNKNOWN();

struct TgcGameMgr {
    char _pad00[0x08];
    CDDrawChildGroup* m_08;
    char _pad0c[0x24 - 0x0c];
    TgcMap* m_24;
};
SIZE_UNKNOWN();

struct TgcRedraw {};
SIZE_UNKNOWN();

#endif // SRC_GRUNTZ_TILEGRIDCOMMAND_H
