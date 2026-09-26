#include <rva.h>

#include <Mfc.h>

#include <Gruntz/FaderSubtypes.h>
#include <Lith/BDefs.h>

#include <math.h>

RVA(0x0017e940, 0x27)
CFaderMesh::CFaderMesh() {}

RVA_COMPGEN(0x0017e970, 0x1e, ??_GCFaderMesh@@UAEPAXI@Z)
RVA(0x0017e990, 0x6b)
CFaderMesh::~CFaderMesh() {}

RVA(0x0017ea00, 0x4fc)
i32 CFaderMesh::ApplyInit(CFaderConfig* descOpaque) {

    CMeshFaderConfig* cfg = static_cast<CMeshFaderConfig*>(descOpaque);

    SelectTarget(m_dstSurface, cfg->m_targetSurface);
    SelectSource(m_sourceSurface, cfg->m_sourceSurface);
    if (cfg->m_flipTarget == NULL) {
        return 0;
    }
    m_primeSurface = cfg->m_primeSource;
    m_flipTarget = cfg->m_flipTarget;
    m_unusedOption = cfg->m_unusedOption;
    m_reverseOrder = cfg->m_reverseOrder;
    m_cols = cfg->m_cols;
    m_rows = cfg->m_rows;

    m_meshBuf.RemoveAll();

    i32 halfW = static_cast<i32>(m_dstSurface->m_apiDesc.dwWidth) / 2;
    i32 halfH = static_cast<i32>(m_dstSurface->m_apiDesc.dwHeight) / 2;
    i32 cellW = static_cast<i32>(m_sourceSurface->m_apiDesc.dwWidth) / m_cols;
    i32 cellH = static_cast<i32>(m_sourceSurface->m_apiDesc.dwHeight) / m_rows;
    float radius = static_cast<float>(sqrt(static_cast<double>((SQR(cellW) + SQR(cellH)))));
    RezElem40 elem;
    for (i32 r = 0; r < m_rows; r++) {
        for (i32 c = 0; c < m_cols; c++) {
            i32 x = c * cellW;
            i32 y = r * cellH;
            CRect dispersedRect(0, 0, cellW, cellH);
            float v =
                static_cast<float>(sqrt(static_cast<double>(SQR(halfW - x) + SQR(halfH - y))));
            float cellR = static_cast<float>(sqrt(static_cast<double>(SQR(halfH) + SQR(halfW))))
                          + radius + 50.0f;
            float u, w;
            if (v > 1.0f) {
                u = (x - halfW) / v;
                w = (y - halfH) / v;
            } else {
                u = 0.0f;
                w = 1.0f;
            }
            dispersedRect.OffsetRect(x, y);
            dispersedRect.OffsetRect(static_cast<i32>((u * cellR)), static_cast<i32>((w * cellR)));

            CRect assembledRect(0, 0, cellW, cellH);
            assembledRect.OffsetRect(x, y);

            if (m_reverseOrder) {
                elem.m_startRect = assembledRect;
                elem.m_endRect = dispersedRect;
            } else {
                elem.m_startRect = dispersedRect;
                elem.m_endRect = assembledRect;
            }
            elem.m_reserved20 = 0;
            elem.m_scale = 1.0f;

            m_meshBuf.Add(elem);
        }
    }
    return 1;
}

// @early-stop
RVA(0x0017ef00, 0x21c)
void CFaderMesh::RenderFrame(i32 frame) {
    if (m_primeSurface != NULL) {
        m_dstSurface->Blt(m_primeSurface);
    } else {
        m_dstSurface->Clear(0);
    }
    for (i32 i = 0; i < m_meshBuf.GetSize(); i++) {
        RezElem40 elem = m_meshBuf[i];
        u32 cur = frame;
        u32 total = GetFrameCount();
        float t = static_cast<float>(cur) / static_cast<float>(total);
        RECT srcRect = elem.m_startRect;
        RECT dstRect;
        RECT boundRect = elem.m_endRect;

        dstRect.left =
            elem.m_startRect.left
            + static_cast<i32>(static_cast<float>(elem.m_endRect.left - elem.m_startRect.left) * t);
        dstRect.top =
            elem.m_startRect.top
            + static_cast<i32>(static_cast<float>(elem.m_endRect.top - elem.m_startRect.top) * t);
        dstRect.right = elem.m_startRect.right
                        + static_cast<i32>(
                            static_cast<float>(elem.m_endRect.right - elem.m_startRect.right) * t
                        );
        dstRect.bottom = elem.m_startRect.bottom
                         + static_cast<i32>(
                             static_cast<float>(elem.m_endRect.bottom - elem.m_startRect.bottom) * t
                         );

        if (dstRect.left < 0 && dstRect.right > 0) {
            boundRect.left = elem.m_endRect.left - dstRect.left;
            dstRect.left = 0;
        } else if (dstRect.right >= static_cast<i32>(m_dstSurface->m_apiDesc.dwWidth)
                   && dstRect.left < static_cast<i32>(m_dstSurface->m_apiDesc.dwWidth)) {
            boundRect.right =
                elem.m_endRect.right - dstRect.right + m_dstSurface->m_apiDesc.dwWidth;
            dstRect.right = m_dstSurface->m_apiDesc.dwWidth - 1;
        }
        if (dstRect.top < 0 && dstRect.bottom > 0) {
            boundRect.top = boundRect.top - dstRect.top;
            dstRect.top = 0;
        } else if (dstRect.bottom >= static_cast<i32>(m_dstSurface->m_apiDesc.dwHeight)
                   && dstRect.top < static_cast<i32>(m_dstSurface->m_apiDesc.dwHeight)) {
            boundRect.bottom =
                boundRect.bottom + (m_dstSurface->m_apiDesc.dwHeight - dstRect.bottom);
            dstRect.bottom = m_dstSurface->m_apiDesc.dwHeight - 1;
        }

        m_dstSurface->BltEx(
            &dstRect,
            m_sourceSurface,
            m_reverseOrder != false ? &srcRect : &boundRect,
            DDBLT_WAIT,
            NULL
        );
    }
    m_flipTarget->Flip(NULL);
}

RVA(0x0017f120, 0x6)
i32 CFaderMesh::GetFrameCount() {
    return 0x1f4;
}

RVA_COMPGEN(0x0017f130, 0x1ce, ?Serialize@?$CArray@URezElem40@@ABU1@@@UAEXAAVCArchive@@@Z)
RVA_COMPGEN(0x0017f300, 0x3, ??0RezElem40@@QAE@XZ)
RVA_COMPGEN(0x0017f310, 0x1e, ??_G?$CArray@URezElem40@@ABU1@@@UAEPAXI@Z)
RVA_COMPGEN(0x0017f330, 0x51, ??1?$CArray@URezElem40@@ABU1@@@UAE@XZ)
RVA_COMPGEN(0x0017f390, 0x164, ?SetSize@?$CArray@URezElem40@@ABU1@@@QAEXHH@Z)
RVA_COMPGEN(0x0017f500, 0x23, ?ConstructElements@@YGXPAURezElem40@@H@Z)
template void CArray<RezElem40, const RezElem40&>::SetSize(int, int);
