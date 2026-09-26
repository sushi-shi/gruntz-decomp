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
    m_drawActive = true;
    m_drawFillCmd = mode;
    m_drawFillArg = table;
}

inline void CResolveNode::SetDrawFillReversed(ShadeMode mode, CShadeTable* table) {
    m_drawActive = true;
    m_drawFillArg = table;
    m_drawFillCmd = mode;
}

#endif // GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H
