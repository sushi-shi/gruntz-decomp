#ifndef GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H
#define GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H

#include <DDrawMgr/PixelFormatMacros.h>
#include <Gruntz/ResolveNode.h>

inline void CRenderState::ResetDrawFill() {
    m_shadeTable = NULL;
    m_shadeMode = SHADE_COPY;
    m_hasShadeOverride = false;
}

inline void CRenderState::SetDrawFill(ShadeMode mode, CShadeTable* table) {
    m_hasShadeOverride = true;
    m_shadeMode = mode;
    m_shadeTable = table;
}

inline void CRenderState::SetDrawFillReversed(ShadeMode mode, CShadeTable* table) {
    m_hasShadeOverride = true;
    m_shadeTable = table;
    m_shadeMode = mode;
}

#endif // GRUNTZ_GRUNTZ_RESOLVENODEINLINE_H
