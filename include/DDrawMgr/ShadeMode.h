#ifndef GRUNTZ_DDRAWMGR_SHADEMODE_H
#define GRUNTZ_DDRAWMGR_SHADEMODE_H

#include <Enums.h>

// Which shading path CDDrawShadeBlit runs, and which CShadeTable slot feeds it.
//
// Recovered from the arms themselves: every value below names the expression
// CDDrawShadeBlit::ConvertRow (src/DDrawMgr/DDrawShadeBlit.cpp) evaluates for
// that value, and CDDrawShadeBlit::Select maps the same value to the global
// CShadeTable the arm reads. `dst`/`src` are the destination and source pixels,
// `level` is CDDrawShadeBlit::m_light, `table` is m_palDescr.
GZ_ENUM_BEGIN(ShadeMode)
// No table at all - Blit() routes drawType 1 to BlitCopyForward/Mirrored,
// which never call a ConvertRow. It is also the value a CDDrawShadeBlit is
// born with (CDDrawShadeBlit::CDDrawShadeBlit).
    SHADE_COPY = 1,

    // dst = table[dst * 256 + src] - the source byte is a shade LEVEL applied to
    // the destination, not a colour. CImage::Build selects it for a PID that
    // carries PID_SRC_8BPP_SHADE.
    SHADE_DST_BY_SRC = 2,

    // dst = table[dst * 256 + level] - the destination shaded by m_light, source
    // ignored. CGruntzMgr::CheatEclipseToggle drives it with a random level.
    SHADE_DST_BY_LEVEL = 3,

    // dst = table[src * 256 + level] - the source shaded by m_light.
    SHADE_SRC_BY_LEVEL = 4,

    // dst = level - every pixel written as one palette index.
    SHADE_FILL_LEVEL = 5,

    // dst = table[(table[dst + 256] - table[src + 256]) * level / 255
    //             + table[src + 256]] - a src->dst cross-fade by m_light,
    // through the table's second page (the +0x100 intensity ramp).
    SHADE_LERP_LEVEL = 6,

    // 16-bit: dst = table16[grey16[dst] + (src >> 4) * 0x1000] - the destination
    // re-shaded through the ramp page the source's top nibble picks. grey16 is
    // the SHADE_GREY_TABLE slot. Blit() rejects it unless the source is 8bpp and
    // the destination 16bpp.
    SHADE_DST_BY_SRC_16 = 7,

    // 16-bit: source and destination combined per 5-bit channel through
    // m_lutBank0/1/2, the bank picked by (m_light >> 3) * 0x800.
    SHADE_ALPHA_16 = 8,

    // Not a draw path - no ConvertRow arm and no CDDrawShadeBlit::Select arm.
    // The only use is SetShadeDescr(table, 9), which stores into g_greyShadeTable,
    // the grey16 table SHADE_DST_BY_SRC_16 reads. CLightFxMgr::Init passes
    // CShadeTableCache::GreyTable().
    SHADE_GREY_TABLE = 9,

    // 16-bit: dst = table16[src] - an 8-bit source expanded through Lut16, the
    // destination never read.
    SHADE_PAL_16 = 10,

    // SHADE_PAL_16, then blended into the destination through the same
    // m_lutBank0/1/2 as SHADE_ALPHA_16.
    SHADE_PAL_ALPHA_16 = 11
GZ_ENUM_END(ShadeMode)

#endif // GRUNTZ_DDRAWMGR_SHADEMODE_H
