#ifndef GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H
#define GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H

#include <DDrawMgr/PixelFormatMacros.h>
#include <Gruntz/ResolveNode.h>

static inline void ResetResolveDrawFill(CResolveNode* node) {
    node->m_drawFillArg = NULL;
    node->m_drawFillCmd = SHADE_COPY;
    node->m_drawActive = false;
}

static __inline void setDrawFill(CResolveNode* node, ShadeMode mode, CShadeTable* table) {
    SET_DRAW_FILL(node, mode, table);
}

static __inline void setDrawFillReversed(CResolveNode* node, ShadeMode mode, CShadeTable* table) {
    SET_DRAW_FILL_REVERSED(node, mode, table);
}

#endif // GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H
