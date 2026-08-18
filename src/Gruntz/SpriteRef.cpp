#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SpriteRefTable.h>
#include <Io/GameSave.h>

RVA(0x000e2df0, 0x3f0)
i32 CSpriteRef::Build(CShadeTableCache* cache, CShadeTable* shade, ColorTint kind) {
    m_cache = cache;
    m_alphaKey = shade;
    u8 r1, g1, b1;
    u8 r2, g2, b2;
    u8 r3, g3, b3;
    switch (kind) {
        case TINT_ORANGE:
            r2 = 0xff;
            g2 = 0x80;
            b2 = 0x00;
            r1 = 0xc0;
            g1 = 0x60;
            b1 = 0x00;
            r3 = 0x80;
            g3 = 0x40;
            b3 = 0x00;
            break;
        case TINT_GREEN:
            r2 = 0x00;
            g2 = 0xff;
            b2 = 0x00;
            r1 = 0x00;
            g1 = 0xc0;
            b1 = 0x00;
            r3 = 0x00;
            g3 = 0x80;
            b3 = 0x00;
            break;
        case TINT_BLUE:
            r2 = 0x00;
            g2 = 0x00;
            b2 = 0xff;
            r1 = 0x00;
            g1 = 0x00;
            b1 = 0xc0;
            r3 = 0x00;
            g3 = 0x00;
            b3 = 0x80;
            break;
        case TINT_RED:
            r2 = 0xff;
            g2 = 0x00;
            b2 = 0x00;
            r1 = 0xc0;
            g1 = 0x00;
            b1 = 0x00;
            r3 = 0x80;
            g3 = 0x00;
            b3 = 0x00;
            break;
        case TINT_HOTPINK:
            r2 = 0xff;
            g2 = 0x00;
            b2 = 0x80;
            r1 = 0xc0;
            g1 = 0x00;
            b1 = 0x60;
            r3 = 0x80;
            g3 = 0x00;
            b3 = 0x40;
            break;
        case TINT_YELLOW:
            r2 = 0xff;
            g2 = 0xff;
            b2 = 0x00;
            r1 = 0xc0;
            g1 = 0xc0;
            b1 = 0x00;
            r3 = 0x80;
            g3 = 0x80;
            b3 = 0x00;
            break;
        case TINT_PINK:
            r2 = 0xff;
            g2 = 0x00;
            b2 = 0xff;
            r1 = 0xc0;
            g1 = 0x00;
            b1 = 0xc0;
            r3 = 0x80;
            g3 = 0x00;
            b3 = 0x80;
            break;
        case TINT_DKBLUE:
            r2 = 0x00;
            g2 = 0x00;
            b2 = 0x80;
            r1 = 0x00;
            g1 = 0x00;
            b1 = 0x60;
            r3 = 0x00;
            g3 = 0x00;
            b3 = 0x40;
            break;
        case TINT_DKGREEN:
            r2 = 0x00;
            g2 = 0x80;
            b2 = 0x00;
            r1 = 0x00;
            g1 = 0x60;
            b1 = 0x00;
            r3 = 0x00;
            g3 = 0x40;
            b3 = 0x00;
            break;
        case TINT_TURQ:
            r2 = 0x00;
            g2 = 0x80;
            b2 = 0x80;
            r1 = 0x00;
            g1 = 0x60;
            b1 = 0x60;
            r3 = 0x00;
            g3 = 0x40;
            b3 = 0x40;
            break;
        case TINT_DKRED:
            r2 = 0x80;
            g2 = 0x00;
            b2 = 0x00;
            r1 = 0x60;
            g1 = 0x00;
            b1 = 0x00;
            r3 = 0x40;
            g3 = 0x00;
            b3 = 0x00;
            break;
        case TINT_PURPLE:
            r2 = 0x80;
            g2 = 0x00;
            b2 = 0x80;
            r1 = 0x60;
            g1 = 0x00;
            b1 = 0x60;
            r3 = 0x40;
            g3 = 0x00;
            b3 = 0x40;
            break;
        case TINT_DKYELLOW:
            r2 = 0x80;
            g2 = 0x80;
            b2 = 0x00;
            r1 = 0x60;
            g1 = 0x60;
            b1 = 0x00;
            r3 = 0x40;
            g3 = 0x40;
            b3 = 0x00;
            break;
        case TINT_GREY:
            r2 = 0x80;
            g2 = 0x80;
            b2 = 0x80;
            r1 = 0x60;
            g1 = 0x60;
            b1 = 0x60;
            r3 = 0x40;
            g3 = 0x40;
            b3 = 0x40;
            break;
        case TINT_CYAN:
            r2 = 0x00;
            g2 = 0xff;
            b2 = 0xff;
            r1 = 0x00;
            g1 = 0xc0;
            b1 = 0xc0;
            r3 = 0x00;
            g3 = 0x80;
            b3 = 0x80;
            break;
        case TINT_WHITE:
            r2 = 0xff;
            g2 = 0xff;
            b2 = 0xff;
            r1 = 0xc0;
            g1 = 0xc0;
            b1 = 0xc0;
            r3 = 0x80;
            g3 = 0x80;
            b3 = 0x80;
            break;
        case TINT_BLACK:
            r2 = 0x40;
            g2 = 0x40;
            b2 = 0x40;
            r1 = 0x20;
            g1 = 0x20;
            b1 = 0x20;
            r3 = 0x20;
            g3 = 0x20;
            b3 = 0x20;
            break;
        default:
            return 0;
    }
    m_teamColor1 = static_cast<u16>(
        ((static_cast<u8>((static_cast<u8>(r1) >> static_cast<u8>(g_rDown))) << g_rUp)
         | (static_cast<u8>((static_cast<u8>(g1) >> static_cast<u8>(g_gDown))) << g_gUp)
         | static_cast<u8>((static_cast<u8>(b1) >> static_cast<u8>(g_bDown))))
    );
    m_teamColor2 = static_cast<u16>(
        ((static_cast<u8>((static_cast<u8>(g2) >> static_cast<u8>(g_gDown))) << g_gUp)
         | (static_cast<u8>((static_cast<u8>(r2) >> static_cast<u8>(g_rDown))) << g_rUp)
         | static_cast<u8>((static_cast<u8>(b2) >> static_cast<u8>(g_bDown))))
    );
    m_teamColor3 = static_cast<u16>(
        ((static_cast<u8>((static_cast<u8>(r3) >> static_cast<u8>(g_rDown))) << g_rUp)
         | (static_cast<u8>((static_cast<u8>(g3) >> static_cast<u8>(g_gDown))) << g_gUp)
         | static_cast<u8>((static_cast<u8>(b3) >> static_cast<u8>(g_bDown))))
    );
    return 1;
}

RVA(0x000e32e0, 0x25)
void CSpriteRef::Free() {
    CShadeTableCache* cache = m_cache;
    if (cache && m_alphaKey) {
        cache->FindRemove(m_alphaKey);
        m_cache = NULL;
        m_alphaKey = NULL;
    }
}
