#ifndef GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H
#define GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H

#include <DDrawMgr/PixelFormatMacros.h>
#include <Gruntz/ResolveNode.h>

inline void CResolveNode::ResetDrawFill() {
    m_drawFillArg = NULL;
    m_drawFillCmd = SHADE_COPY;
    m_drawActive = false;
}

inline void CResolveNode::SetDrawFill(ShadeMode mode, CShadeTable* table) {
    SET_DRAW_FILL(this, mode, table);
}

inline void CResolveNode::SetDrawFillReversed(ShadeMode mode, CShadeTable* table) {
    SET_DRAW_FILL_REVERSED(this, mode, table);
}

#endif // GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H
