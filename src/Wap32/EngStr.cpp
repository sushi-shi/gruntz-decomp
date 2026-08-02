#include <rva.h>

#include <Wap32/EngStr.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Ints.h>

// @early-stop
RVA(0x00115440, 0x45)
void EngStr_DrawText(
    CDDrawSurfaceMgr* obj,
    CString* text,
    RECT* dst,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
) {
    CDDrawSurfaceChildA* pair = obj->m_drawTarget->m_frontPair;

    if (pair == 0) {
        return;
    }
    EngStr_RenderText(obj, text, dst, pair->m_surface, fontSel, shadow, r, g, b, flag);
}
