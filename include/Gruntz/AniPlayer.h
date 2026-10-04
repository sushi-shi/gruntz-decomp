#ifndef GRUNTZ_GRUNTZ_CANIPLAYER_H
#define GRUNTZ_GRUNTZ_CANIPLAYER_H

#include <string>

#include <Ints.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SerialArchive.h>

class CAniPlayer : public CSBI_ImageSetAni {
public:
    i32 Start(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const std::string& key,
        i32 frameStart,
        i32 frameEnd,
        i32 intervalMs,
        i32 loop,
        i32 step
    );
    i32 TickToggle(i32 unused);
    i32 RenderCel();
    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    ClockInterval m_timing;
};

#endif
