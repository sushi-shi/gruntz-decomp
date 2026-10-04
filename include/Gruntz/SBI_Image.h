#ifndef GRUNTZ_SBI_IMAGE_H
#define GRUNTZ_SBI_IMAGE_H

#include <string>

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Ints.h>
#include <MakeRect.h>

#include <stddef.h>

class CDDrawSurfaceMgr;
class CStatusBarMgr;
class CImage;

class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly();
    virtual ~CSBI_RectOnly()  ;

    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const std::string& key,
        i32 unusedFrame
    )  ;
    virtual void Reset()  ;
    virtual i32 Refresh(i32 deltaMs)  ;
};

inline CSBI_RectOnly::~CSBI_RectOnly() {
    Reset();
}

class CSBI_Image : public CSBI_RectOnly {
public:
    CSBI_Image();
    virtual ~CSBI_Image()  ;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
         ;
    virtual void Reset()  ;
    virtual i32 Refresh(i32 deltaMs)  ;
    virtual i32 Render()  ;

    virtual i32 SetupImage(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const std::string& key,
        i32 frame,
        i32 extra
    );

    void SetFrame(CImage* frame) {
        m_frame = frame;
    }

    CImage* m_frame;
};

inline CSBI_RectOnly::CSBI_RectOnly() {
    m_kind = SBI_KIND_RECT_ONLY;
}

inline CSBI_Image::CSBI_Image() {
    m_kind = SBI_KIND_IMAGE;
    m_frame = NULL;
}

inline CSBI_Image::~CSBI_Image() {
    Reset();
}

#endif
