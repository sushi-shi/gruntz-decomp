#include <Ints.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>  // the real render "object" (world mgr; m_drawTarget)
#include <DDrawMgr/DDrawSubMgrPages.h> // the pages (m_frontPair)
#include <DDrawMgr/DDrawSurfacePair.h> // the front pair (m_surface)
#include <rva.h>
#include <Wap32/EngStr.h>

// EngStr text-draw forwarder (__cdecl). Fetches the render config off
// obj->m_sub->m_10; when present, forwards eight caller args to the text-render
// worker with the config's font draw-method pointer spliced in as the 4th arg.
// 0x115440.
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
    CDDrawSurfaceChildA* pair =
        obj->m_drawTarget->m_frontPair; // the real chain (ex the Sub/Cfg facets)
    // retail spends a byte here that cl5 will not: `test eax,eax; jne +1; ret` - an
    // INLINE 1-byte early ret, where cl5 jumps past the `add esp,0x28` to the tail ret
    // (`74 35`) from every spelling tried (early return / positive if / if-else).
    // Those 4 bytes are the whole residual; the 0x35-byte body is byte-identical.
    if (pair == 0) {
        return;
    }
    EngStr_RenderText(obj, text, dst, pair->m_surface, fontSel, shadow, r, g, b, flag);
}
