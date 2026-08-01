#ifndef GRUNTZ_GRUNTZ_CANIPLAYER_H
#define GRUNTZ_GRUNTZ_CANIPLAYER_H

#include <rva.h>

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
        i32 cmd,
        i32 tab,
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
    i32 Serialize(CFileMemBase* arc, i32 mode, i32 typeId, i32 pObj);

    i32 m_54;

    union {
        i64 m_start64;
        struct {
            i32 m_58;
            i32 m_5c;
        };
    };
    union {
        i64 m_window64;
        struct {
            i32 m_60;
            i32 m_64;
        };
    };
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CANIPLAYER_H
