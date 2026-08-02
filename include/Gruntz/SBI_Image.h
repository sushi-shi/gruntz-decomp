#ifndef GRUNTZ_SBI_IMAGE_H
#define GRUNTZ_SBI_IMAGE_H

#include <rva.h>

#include <Gruntz/SbGeom.h>
#include <Gruntz/StatusBarItem.h>
#include <Ints.h>

class CDDrawSurfaceMgr;
class CStatusBarMgr;
class CImage;

class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly() {
        m_kind = 1;
    }
    virtual ~CSBI_RectOnly() OVERRIDE;

    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 tab,
        RECT rc,
        const char* key,
        i32 a10
    ) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
};
SIZE_UNKNOWN();

inline CSBI_RectOnly::~CSBI_RectOnly() {
    Reset();
}

class CSBI_Image : public CSBI_RectOnly {
public:
    CSBI_Image() {
        m_kind = 3;
        m_frame = 0;
    }
    virtual ~CSBI_Image() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, i32 kind, i32 a, i32 b) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 tab,
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
