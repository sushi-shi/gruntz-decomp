#ifndef GRUNTZ_GRUNTZ_WARPSTONEFLY_H
#define GRUNTZ_GRUNTZ_WARPSTONEFLY_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/DoubleVector.h>
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

    i32 Init(CStatusBarMgr* owner, i32 srcX, i32 srcY, WarpStoneFragment fragment);
    i32 Tick(u32 dt);
    i32 Draw();

    i32 SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    WarpStoneFragment m_arrivalMode;
    Coord m_target;
    char m_padc[0x10 - 0xc];
    DoubleVector2 m_current;
    double m_velocityScale;
    DoubleVector2 m_direction;
    CImage* m_sprite;
    CStatusBarMgr* m_owner;
};

#endif // GRUNTZ_GRUNTZ_WARPSTONEFLY_H
