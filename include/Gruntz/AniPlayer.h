#ifndef GRUNTZ_GRUNTZ_CANIPLAYER_H
#define GRUNTZ_GRUNTZ_CANIPLAYER_H

#include <rva.h>

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

    union {
        i64 m_start64;
        struct {
            i32 m_startLo;
            i32 m_startHi;
        };
    };
    union {
        i64 m_window64;
        struct {
            i32 m_windowLo;
            i32 m_windowHi;
        };
    };
};

#endif // GRUNTZ_GRUNTZ_CANIPLAYER_H
