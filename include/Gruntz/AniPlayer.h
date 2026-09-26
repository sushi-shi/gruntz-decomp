#ifndef GRUNTZ_GRUNTZ_CANIPLAYER_H
#define GRUNTZ_GRUNTZ_CANIPLAYER_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SerialArchive.h>

// @identity-TODO
// Retail xrefs find no caller or address-taking for these methods, and no vtable or
// RTTI names the derived owner; a construction site would prove the identity.
class CAniPlayer : public CSBI_ImageSetAni {
public:
    i32 Start(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
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

#endif // GRUNTZ_GRUNTZ_CANIPLAYER_H
