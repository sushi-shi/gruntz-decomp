#include <rva.h>

#include <Gruntz/Brickz.h>

// @early-stop
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
