#ifndef GRUNTZ_GRUNTZ_WARPSTONEFLY_H
#define GRUNTZ_GRUNTZ_WARPSTONEFLY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/WarpStoneFragment.h>
#include <Ints.h>

class CStatusBarMgr;
class CFileMemBase;

class CImage;

class CWarpStoneFly {
public:
    CWarpStoneFly();

    i32 Init(void* owner, i32 srcX, i32 srcY, WarpStoneFragment fragment);
    i32 Tick(i32 dt);
    i32 Draw();

    i32 Sync(CFileMemBase* s, SerialMode op, LogicTypeId typeId, i32 pObj);

    WarpStoneFragment m_arrivalMode;
    i32 m_targetX;
    i32 m_targetY;
    char m_padc[0x10 - 0xc];
    double m_currentX;
    double m_currentY;
    double m_velocityScale;
    double m_xDirection;
    double m_yDirection;
    CImage* m_sprite;
    CStatusBarMgr* m_owner;
};
SIZE(0x40);
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_WARPSTONEFLY_H
