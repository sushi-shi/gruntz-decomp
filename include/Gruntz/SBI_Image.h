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
    // Out-of-line at 0x101fa0; the base stores are folded into its body.
    CSBI_RectOnly();
    // Selected allocation sites inline the complete constructor chain.
    enum EInlineSelf {
        INLINE_SELF
    };
    CSBI_RectOnly(EInlineSelf) : CStatusBarItem(CStatusBarItem::NO_SEED) {
        m_kind = SBI_KIND_RECT_ONLY;
    }
    // Selected allocation sites stop inlining at CStatusBarItem.
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

inline CSBI_RectOnly::~CSBI_RectOnly() {
    Reset();
}

class CSBI_Image : public CSBI_RectOnly {
public:
    CSBI_Image() : CSBI_RectOnly(BASE_CALL) {
        m_kind = SBI_KIND_IMAGE;
        m_frame = NULL;
    }
    enum EInlineChain {
        INLINE_CHAIN
    };
    CSBI_Image(EInlineChain) : CSBI_RectOnly(CSBI_RectOnly::INLINE_SELF) {
        m_kind = SBI_KIND_IMAGE;
        m_frame = NULL;
    }
    // Selected allocation sites stop inlining at CSBI_RectOnly.
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

inline CSBI_Image::~CSBI_Image() {
    Reset();
}

#endif // GRUNTZ_SBI_IMAGE_H
