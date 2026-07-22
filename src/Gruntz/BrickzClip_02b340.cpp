#include <rva.h>
#include <Win32.h> // RECT + IntersectRect (Clip) - windows-first: CMapMgr::m_bounds is a RECT
#include <Gruntz/Brickz.h>

// ---------------------------------------------------------------------------
// CMapMgr::Clip (0x02b340) - board dirty-rect clip finaliser. Clip the
// (0,0,m_width,m_height) box against the optional src rect (right/bottom inclusive
// -> +1), store the clipped rect into the +0x60 bound-rect (m_originX..m_boundBottom),
// then derive the +0x70/+0x74 extents. __thiscall(const RECT*) ; ret 0x4.
// (Homed from BattlezMapConfig.cpp; was the placeholder ClipHost_02b340 view -
//  this->m_board is a CMapMgr, so Clip is a real CMapMgr method.)
// @early-stop
// regalloc-rotation + scheduling wall (~81%): the clip logic is faithful, but retail
// keeps `src` in eax and interleaves the rect field loads/stores differently, while cl
// pins `src` in edx - a whole-function register rotation + a scheduling shift. Not
// source-steerable.
RVA(0x0002b340, 0xaa)
void CMapMgr::Clip(const RECT* src) {
    RECT a, b;
    b.left = 0;
    b.top = 0;
    b.right = m_width;
    b.bottom = m_height;
    if (src) {
        a.left = src->left;
        a.top = src->top;
        a.right = src->right + 1;
        a.bottom = src->bottom + 1;
    } else {
        a.left = 0;
        a.top = 0;
        a.right = m_width;
        a.bottom = m_height;
    }
    if (!IntersectRect(&m_bounds, &a, &b)) {
        m_bounds = a;
    }
    m_gridW = m_bounds.right - m_bounds.left;
    m_gridH = m_bounds.bottom - m_bounds.top;
}
