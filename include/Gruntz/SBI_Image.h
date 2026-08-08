#ifndef GRUNTZ_SBI_IMAGE_H
#define GRUNTZ_SBI_IMAGE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbGeom.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Ints.h>

#include <stddef.h>

class CDDrawSurfaceMgr;
class CStatusBarMgr;
class CImage;

class CSBI_RectOnly : public CStatusBarItem {
public:
    // Out-of-line at 0x101fa0 (the base ctor is folded into its body).  Retail's
    // deep chains all `call` it: CSBI_ImageSetAni 2/2, CSBI_WarlordHead 4/4,
    // CSBI_StatzTabArrow 1/1, CSBI_WellGoo 1/1.
    CSBI_RectOnly();
    // `new CSBI_RectOnly` runs the whole chain inline in retail (the three sites in
    // BuildStatusBarTabs carry no ctor call at all), so this one seeds the base.
    enum EInlineSelf {
        INLINE_SELF
    };
    CSBI_RectOnly(EInlineSelf) : CStatusBarItem(CStatusBarItem::NO_SEED) {
        m_kind = SBI_KIND_RECT_ONLY;
    }
    // The CSBI_Image chain takes this one: every `new CSBI_Image` in retail's four
    // status-bar builders is `call ??0CStatusBarItem` + the derived stores inline.
    enum EBaseCall {
        BASE_CALL
    };
    CSBI_RectOnly(EBaseCall) {
        m_kind = SBI_KIND_RECT_ONLY;
    }
    virtual ~CSBI_RectOnly() OVERRIDE;

    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
        i32 a10
    ) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
};
SIZE(0x30);

inline CSBI_RectOnly::~CSBI_RectOnly() {
    Reset();
}

class CSBI_Image : public CSBI_RectOnly {
public:
    CSBI_Image() : CSBI_RectOnly(BASE_CALL) {
        m_kind = SBI_KIND_IMAGE;
        m_frame = NULL;
    }
    // The chains below CSBI_Image cut one level higher - they `call ??0CSBI_RectOnly`.
    enum ECallRectOnly {
        CALL_RECTONLY
    };
    CSBI_Image(ECallRectOnly) : CSBI_RectOnly() {
        m_kind = SBI_KIND_IMAGE;
        m_frame = NULL;
    }
    virtual ~CSBI_Image() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode kind, LogicTypeId a, i32 b) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
        i32 frame,
        i32 extra
    );

    CImage* m_frame;
};
SIZE(0x34);

inline CSBI_Image::~CSBI_Image() {
    Reset();
}

#endif // GRUNTZ_SBI_IMAGE_H
