#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderSubtypes.h>
#include <Lith/BDefs.h>

#include <math.h>

RVA(0x0017f9a0, 0x24)
CFaderRadial::CFaderRadial() {
    m_maxRadius = 0;
    m_unusedZero = 0;
    m_cells = NULL;
    m_unusedOne = 1;
}
RVA_COMPGEN(0x0017f9d0, 0x1e, ??_GCFaderRadial@@UAEPAXI@Z)

RVA(0x0017f9f0, 0x4f)
CFaderRadial::~CFaderRadial() {
    FreeBuffer();
}

RVA(0x0017fa40, 0x1f3)
i32 CFaderRadial::ApplyInit(CFaderConfig* desc) {
    CRadialFaderConfig* cfg = static_cast<CRadialFaderConfig*>(desc);
    if (cfg->m_targetSurface == NULL) {
        m_dstSurface = m_primarySurface;
    } else {
        m_dstSurface = cfg->m_targetSurface;
    }

    if (cfg->m_sourceSurface == NULL) {
        m_srcSurface = m_secondarySurface;
    } else {
        m_srcSurface = cfg->m_sourceSurface;
    }

    if (cfg->m_shadeTable == NULL) {

        CDDPalette* pal = cfg->m_palette;
        m_table = m_cache.HueRampTable(pal->m_entries, 0x10, 0);
        m_ownsTable = true;
    } else {
        m_table = cfg->m_shadeTable;
        m_ownsTable = false;
    }
    if (m_table == NULL) {
        return 0;
    }

    CDDSurface* s = m_srcSurface;
    m_fadeDivisor = static_cast<float>(static_cast<i32>(s->m_apiDesc.dwWidth)) * 0.5f;
    m_center.m_x = static_cast<i32>(s->m_apiDesc.dwWidth) / 2;
    m_center.m_y = static_cast<i32>(s->m_apiDesc.dwHeight) / 2;
    m_cells = new CFaderRadialCell[s->m_apiDesc.dwHeight * s->m_apiDesc.dwWidth];

    i32 cx = m_center.m_x;
    i32 cy = m_center.m_y;
    m_maxRadius = static_cast<i32>((sqrt(static_cast<double>((SQR(cx) + SQR(cy)))) * 10000.0));

    for (i32 y = 0; y < static_cast<i32>(m_srcSurface->m_apiDesc.dwHeight); y++) {
        for (i32 x = 0; x < static_cast<i32>(m_srcSurface->m_apiDesc.dwWidth); x++) {
            i32 dx = x - m_center.m_x;
            i32 dy = y - m_center.m_y;
            CFaderRadialCell cell;
            cell.m_radius = static_cast<float>(
                (static_cast<double>(m_maxRadius)
                 - sqrt(static_cast<double>((SQR(dx) + SQR(dy)))) * 10000.0 + 1.0)
            );
            float fade = cell.m_radius / m_fadeDivisor + 1.0f;
            cell.m_velocity.m_x = static_cast<float>(dx) * fade;
            cell.m_velocity.m_y = static_cast<float>(m_center.m_y - y) * fade;
            cell.m_pixel = m_srcSurface->GetPixel(x, y);
            m_cells[y * m_srcSurface->m_apiDesc.dwWidth + x] = cell;
        }
    }
    return 1;
}

RVA(0x0017fc40, 0x11)
void CFaderRadial::FreeBuffer() {
    if (m_cells) {
        delete[] m_cells;
    }
}

RVA(0x0017fc60, 0x136)
void CFaderRadial::RenderFrame(i32 frame) {
    u8* scratch = new u8[m_dstSurface->m_apiDesc.dwWidth];
    m_dstSurface->Clear(0);
    m_srcSurface->Lock(NULL);
    u8* base = static_cast<u8*>(m_dstSurface->Lock(NULL));
    if (m_table->m_data == NULL) {
        return;
    }

    for (i32 i = 0; i < static_cast<i32>(m_srcSurface->m_apiDesc.dwWidth)
                            * static_cast<i32>(m_srcSurface->m_apiDesc.dwHeight);
         i++) {
        float d = m_cells[i].m_radius - static_cast<float>(static_cast<u32>(frame));
        if (d > 1.0f) {
            float sf = d / m_fadeDivisor + 1.0f;
            i32 px = m_center.m_x + static_cast<i32>((m_cells[i].m_velocity.m_x / sf));
            i32 py = m_center.m_y - static_cast<i32>((m_cells[i].m_velocity.m_y / sf));
            if (px > 0 && px < static_cast<i32>(m_dstSurface->m_apiDesc.dwWidth) && py > 0
                && py < static_cast<i32>(m_dstSurface->m_apiDesc.dwHeight)) {
                base[py * m_dstSurface->m_apiDesc.lPitch + px] = m_cells[i].m_pixel;
            }
        }
    }

    m_srcSurface->m_ddSurface->Unlock(NULL);
    m_dstSurface->Unlock();
    delete[] scratch;
}
RVA(0x0017fda0, 0x8)
i32 CFaderRadial::GetFrameCount() {
    return m_maxRadius;
}
