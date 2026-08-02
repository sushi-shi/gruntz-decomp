#ifndef GRUNTZ_GRUNTZCMDMGR_H
#define GRUNTZ_GRUNTZCMDMGR_H

#include <Mfc.h>
#include <rva.h>
#include <Ints.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/GruntzCommand.h>
class CState;

SIZE_UNKNOWN();

class CGruntzMgr;

class CGruntzCmdMgr {
public:
    void RemoveMatchingTarget(i32 indexByte, i32 typeByte);

    i32 SetMgr(CGruntzMgr* mgr);

    void ClearAndReset();

    i32 ScanTargets(i32 param);

    void DrainBase();

    void Clear();

    void Report1(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g);
    void EnqueueSingle(
        i32 enqueueFlag,
        char targetIndex,
        char gruntIndex,
        char cmdKind,
        i16 posX,
        i16 posY,
        char extraByte,
        char targetType
    );

    void EnqueueMulti(
        i32 enqueueFlag,
        char targetIndex,
        i32 count,
        u8* gruntList,
        char cmdKind,
        i16 posX,
        i16 posY,
        char targetType
    );

    void EnqueueCommand(i32 flag, void* cmd);

    i32 Serialize(CFileMemBase* stream, i32 mode, i32 typeId, i32 pObj);

    i32 IsActive(CFileMemBase* enable);

    void BlitTileMarker(i32 enqueueFlag, i32 targetIndex, i32 x, i32 y, i32 targetType);

    ~CGruntzCmdMgr();

    CPtrList m_base;
    CPtrList m_pendingCommands;
    CGruntzMgr* m_manager;
};
SIZE_UNKNOWN();

i32 __stdcall IsActive2(void* enable);

i32 CALLBACK DebugGruntTypeDialogProc(HWND, UINT, WPARAM, LPARAM);

#endif // GRUNTZ_GRUNTZCMDMGR_H
