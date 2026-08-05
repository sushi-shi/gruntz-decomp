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
        i32 b0,
        i32 b1,
        i32 b2,
        i32 b3,
        i32 b4
    );
    i32 TickToggle(i32 param);
    i32 RenderCel();
    i32 Serialize(CFileMemBase* arc, SerialMode mode, LogicTypeId typeId, i32 pObj);

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
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CANIPLAYER_H
