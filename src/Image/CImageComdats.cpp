#include <rva.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/State.h>
#include <Image/CImage.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezTypeTag.h>
#include <Wwd/WwdFile.h>

#include <ddraw.h>
#include <stdio.h>

// @identity-TODO DrawScreenTextImage@CState - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA(0x000d5c10, 0x10d)
i32 CState::DrawScreenTextImage(const char* name) {
    char buf[0x40];
    sprintf(buf, "\\SCREENZ\\%sTEXT", name);
    CParseSource* src = SymTab2c()->ResolveQualified(buf, IMGTAG_DIP);
    if (src == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* world = m_world;
    CDDrawSurfacePair* page = world->m_drawTarget->m_backPair;
    if (page == NULL) {
        return 0;
    }
    CImage img(0, world);
    if (img.Resolve(src, 1) == 0) {
        return 0;
    }
    img.RenderFrame(page, 0x140, 0x158, 0);
    return 1;
}

// @identity-TODO IsReady@CWapObj - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x000d5d70, 0x16, ??1CLoadable@@UAE@XZ)

RVA(0x000d5da0, 0x6)
i32 CWapObj::IsReady() {
    return 1;
}

// @identity-TODO IsLoaded@CWapObj - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA(0x000d5dc0, 0xb)
i32 CWapObj::IsLoaded() {

    return (static_cast<CImage*>(this))->m_width > 0;
}

// @identity-TODO GetClassId@CImage - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA(0x000d5de0, 0x6)
LoadableClassId CImage::GetClassId() {
    return CLASSID_IMAGE;
}

// @identity-TODO FlipHorizontal@CImage - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA(0x000d5e00, 0x3)
void CImage::FlipHorizontal(void*) {}

// @identity-TODO FlipBoth@CImage - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA(0x000d5e20, 0x1b)
void CImage::FlipBoth(void* arg) {
    FlipVertical(arg);
    FlipHorizontal(arg);
}

// @identity-TODO ?_GCImage - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x000d5e50, 0x1e, ??_GCImage@@UAEPAXI@Z)

// @identity-TODO ?1CImage - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (20 fns) came from the static library. It belongs to another compiland.
RVA_COMPGEN(0x000d5e80, 0x5b, ??1CImage@@UAE@XZ)
