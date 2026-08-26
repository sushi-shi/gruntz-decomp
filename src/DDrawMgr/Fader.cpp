#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderConfig.h>
#include <Gruntz/FaderMode.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/ShapeFaderConfig.h>
#include <Ints.h>
#include <Utils/RecordFill.h>
#include <Wap32/ScreenGeometry.h>

#include <ddraw.h>
#include <math.h>
#include <string.h>

DATA(0x001f07bc)
static const float kMsToSeconds = 0.001f;

RVA(0x0017e450, 0x23)
CFader::CFader() {
    m_table = NULL;
    m_ownsTable = true;
}

RVA_COMPGEN(0x0017e480, 0x1e, ??_GCFader@@UAEPAXI@Z)
RVA(0x0017e4a0, 0x69)
CFader::~CFader() {
    if (m_table && m_ownsTable) {
        m_cache.FindRemove(m_table);
        m_table = NULL;
    }
}

RVA(0x0017e510, 0x23)
void CFader::Wait(i32 delay) {
    DWORD target = GetTickCount() + delay;
    while (GetTickCount() < target) {
    }
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017e540, 0xd8)
void CFader::RunFadeStepped(i32 step, i32 lead, i32 vsync) {
    i32 count = GetFrameCount();
    if (count < 1) {
        return;
    }
    BeginFade();
    RenderFrame(0);
    Wait(lead);
    DWORD startTick = GetTickCount();
    i32 loops = 0;
    i32 frame = 1;
    while (frame <= count) {
        if (vsync && m_deviceManager) {
            m_deviceManager->m_device->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
        }
        RenderFrame(frame);
        loops++;
        frame += step;
    }
    if (frame != count) {
        RenderFrame(count);
        loops++;
    }
    float fLoops = static_cast<float>(loops);
    m_measuredFps = static_cast<i32>(
        (fLoops / (static_cast<float>(GetTickCount() - startTick) * kMsToSeconds))
    );
    EndFade();
}

// @early-stop
RVA(0x0017e620, 0x13b)
void CFader::RunFade(u32 dur, i32 lead, i32 vsync) {
    i32 frame = 0;
    i32 prev = 0;
    i32 count = GetFrameCount();
    if (count < 1) {
        return;
    }
    BeginFade();
    RenderFrame(0);
    Wait(lead);
    i32 loops = 0;
    DWORD startTick = GetTickCount();
    if (count >= 0) {
        float fStart = static_cast<float>(startTick);
        float fDur = static_cast<float>(dur);
        float fCount = static_cast<float>(count);
        do {
            frame =
                static_cast<i32>(((static_cast<float>(GetTickCount()) - fStart) / fDur * fCount));
            if (prev != frame && frame <= count && frame > 0) {
                if (vsync && m_deviceManager) {
                    m_deviceManager->m_device->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
                }
                RenderFrame(frame);
                loops++;
            }
            prev = frame;
        } while (frame <= count);
    }
    if (frame != count) {
        RenderFrame(count);
        loops++;
    }
    float fLoops = static_cast<float>(loops);
    m_measuredFps = static_cast<i32>(
        (fLoops / (static_cast<float>(GetTickCount() - startTick) * kMsToSeconds))
    );
    EndFade();
}

RVA(0x0017e760, 0x11)
void CFader::SetDefaultSurfaces(CDDSurface* primary, CDDSurface* secondary) {
    m_primarySurface = primary;
    m_secondarySurface = secondary;
}

RVA(0x0017e780, 0xa)
void CFader::SetDeviceManager(CDDrawDeviceManager* manager) {
    m_deviceManager = manager;
}

RVA(0x0017e790, 0x1)
void CFader::BeginFade() {}

RVA(0x0017e7a0, 0x1)
void CFader::EndFade() {}

RVA(0x0017e7b0, 0x9)
CFaderConfig::CFaderConfig() {
    m_kind = FADER_CONFIG_UNTAGGED;
}

RVA(0x0017e7c0, 0x7a)
CShapeFaderConfig::CShapeFaderConfig() {
    m_kind = FADER_CONFIG_SHAPE;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_warpSourceSurface = NULL;
    m_halfWidth = 0x32;
    m_mode = FADER_SWEEP_FORWARD;
    m_stripCopy = true;
    m_useLut = false;
    m_shadeTable = NULL;
    m_shadeTablePath = "";
    m_palette = NULL;
}

RVA(0x0017e840, 0x37)
CLightFaderConfig::CLightFaderConfig() {
    m_centerX = SCREEN_HALF_W_PX;
    m_kind = FADER_CONFIG_LIGHT;
    m_sourceSurface = NULL;
    m_targetSurface = NULL;
    m_clearMode = true;
    m_spanCount = 0;
    m_centerY = SCREEN_HALF_H_PX;
    m_shadeTable = NULL;
}

RVA(0x0017e880, 0x28)
CSineFaderConfig::CSineFaderConfig() {
    m_kind = FADER_CONFIG_SINE;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_clearToBlack = true;
    m_intensityPercent = 0xf;
}

RVA(0x0017e8b0, 0x27)
CRadialFaderConfig::CRadialFaderConfig() {
    m_kind = FADER_CONFIG_RADIAL;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_palette = NULL;
    m_shadeTable = NULL;
    m_unusedOption = 1;
}

RVA(0x0017e8e0, 0x27)
CFlatFaderConfig::CFlatFaderConfig() {
    m_kind = FADER_CONFIG_FLAT;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_unusedOption = 0;
    m_splitPercent = 0;
    m_durationPercent = 0x19;
}

RVA(0x0017e910, 0x29)
CMeshFaderConfig::CMeshFaderConfig() {
    m_kind = FADER_CONFIG_MESH;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_flipTarget = NULL;
    m_reverseOrder = false;
    m_unusedOption = 0;
    m_cols = 0;
    m_rows = 0;
}

RVA(0x0017e940, 0x27)
CFaderMesh::CFaderMesh() {}

RVA_COMPGEN(0x0017e970, 0x1e, ??_GCFaderMesh@@UAEPAXI@Z)
RVA(0x0017e990, 0x6b)
CFaderMesh::~CFaderMesh() {}

RVA(0x0017ea00, 0x4fc)
i32 CFaderMesh::ApplyInit(CFaderConfig* descOpaque) {

    CMeshFaderConfig* cfg = static_cast<CMeshFaderConfig*>(descOpaque);

    if (cfg->m_targetSurface == NULL) {
        m_dstSurface = m_primarySurface;
    } else {
        m_dstSurface = cfg->m_targetSurface;
    }
    if (cfg->m_sourceSurface == NULL) {
        m_sourceSurface = m_secondarySurface;
    } else {
        m_sourceSurface = cfg->m_sourceSurface;
    }
    if (cfg->m_flipTarget == NULL) {
        return 0;
    }
    m_primeSurface = cfg->m_primeSource;
    m_flipTarget = cfg->m_flipTarget;
    m_unusedOption = cfg->m_unusedOption;
    m_reverseOrder = cfg->m_reverseOrder;
    m_cols = cfg->m_cols;
    m_rows = cfg->m_rows;

    CRezBufferObject* mesh = &m_meshBuf;
    mesh->SetSize(0, -1);

    i32 halfW = m_dstSurface->m_width / 2;
    i32 halfH = m_dstSurface->m_height / 2;
    i32 cellW = m_sourceSurface->m_width / m_cols;
    i32 cellH = m_sourceSurface->m_height / m_rows;
    float radius = static_cast<float>(sqrt(static_cast<double>((cellW * cellW + cellH * cellH))));
    if (m_rows <= 0) {
        return 1;
    }

    RezElem40 elem;
    i32 y = 0;
    i32 ay = halfH;
    i32 negH = -cellH;
    i32 r = 0;
    do {
        if (m_cols > 0) {
            elem.m_reserved20 = 0;
            elem.m_scale = 1.0f;
            i32 rowD2 = ay * ay;
            float cellR = static_cast<float>(
                sqrt(static_cast<double>(halfH * halfH + halfW * halfW)) + radius - g_fxBias
            );
            i32 x = 0;
            i32 bx = halfW;
            i32 negW = -cellW;
            i32 i = 0;
            do {
                RECT pt48;
                pt48.left = 0;
                pt48.top = 0;
                pt48.right = cellW;
                pt48.bottom = cellH;
                i32 d2 = bx * bx + rowD2;
                double v = sqrt(static_cast<double>(d2));
                float u, w;
                if (v > g_fxEps) {
                    u = static_cast<float>((x - halfW) / v);
                    w = static_cast<float>((y - halfH) / v);
                } else {
                    u = 0.0f;
                    w = 1.0f;
                }
                OffsetRect(&pt48, x, y);
                OffsetRect(&pt48, static_cast<i32>((u * cellR)), static_cast<i32>((w * cellR)));

                RECT pt64;
                pt64.left = 0;
                pt64.top = 0;
                pt64.right = d2;
                pt64.bottom = cellH;
                OffsetRect(&pt64, x, y);

                if (m_reverseOrder) {
                    elem.m_startRect = pt64;
                    elem.m_endRect = pt48;
                } else {
                    elem.m_startRect = pt48;
                    elem.m_endRect = pt64;
                }

                i32 idx = mesh->m_nSize;
                i32 newSize = idx + 1;
                if (newSize == 0) {
                    if (mesh->m_pData) {
                        delete[] mesh->m_pData;
                        mesh->m_pData = NULL;
                    }
                    mesh->m_nMaxSize = 0;
                    mesh->m_nSize = 0;
                } else if (mesh->m_pData == NULL) {
                    mesh->m_pData = static_cast<RezElem40*>(
                        static_cast<void*>(new BYTE[newSize * sizeof(RezElem40)])
                    );
                    memset(mesh->m_pData, 0, newSize * sizeof(RezElem40));
                    mesh->m_nMaxSize = newSize;
                    mesh->m_nSize = newSize;
                } else if (newSize <= mesh->m_nMaxSize) {
                    if (newSize > idx) {
                        memset(&mesh->m_pData[idx], 0, (newSize - idx) * sizeof(RezElem40));
                    } else if (idx > newSize) {
                        RezElem40* gone = &mesh->m_pData[newSize];
                        i32 nGone = idx - newSize;
                        for (; nGone--; gone++) {
                        }
                    }
                    mesh->m_nSize = newSize;
                } else {
                    i32 grow = mesh->m_nGrowBy;
                    if (grow == 0) {
                        grow = idx / 8;
                        if (grow < 4) {
                            grow = 4;
                        } else if (grow > 0x400) {
                            grow = 0x400;
                        }
                    }
                    i32 newMax;
                    if (newSize < mesh->m_nMaxSize + grow) {
                        newMax = mesh->m_nMaxSize + grow;
                    } else {
                        newMax = newSize;
                    }
                    RezElem40* nd = static_cast<RezElem40*>(
                        static_cast<void*>(new BYTE[newMax * sizeof(RezElem40)])
                    );
                    memcpy(nd, mesh->m_pData, mesh->m_nSize * sizeof(RezElem40));
                    memset(&nd[mesh->m_nSize], 0, (newSize - mesh->m_nSize) * sizeof(RezElem40));
                    delete[] mesh->m_pData;
                    mesh->m_pData = nd;
                    mesh->m_nSize = newSize;
                    mesh->m_nMaxSize = newMax;
                }
                mesh->m_pData[idx] = elem;

                x += cellW;
                bx += negW;
                i++;
            } while (i < m_cols);
        }
        y += cellH;
        ay += negH;
        r++;
    } while (r < m_rows);
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
    for (i32 i = 0; i < m_meshBuf.m_nSize; i++) {
        RezElem40 elem = m_meshBuf.m_pData[i];
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
        } else if (dstRect.right >= m_dstSurface->m_width && dstRect.left < m_dstSurface->m_width) {
            boundRect.right = elem.m_endRect.right - dstRect.right + m_dstSurface->m_width;
            dstRect.right = m_dstSurface->m_width - 1;
        }
        if (dstRect.top < 0 && dstRect.bottom > 0) {
            boundRect.top = boundRect.top - dstRect.top;
            dstRect.top = 0;
        } else if (dstRect.bottom >= m_dstSurface->m_height
                   && dstRect.top < m_dstSurface->m_height) {
            boundRect.bottom = boundRect.bottom + (m_dstSurface->m_height - dstRect.bottom);
            dstRect.bottom = m_dstSurface->m_height - 1;
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

RVA_COMPGEN(0x0017f310, 0x1e, ??_GCRezBufferObject@@UAEPAXI@Z)
RVA_COMPGEN(0x0017f330, 0x51, ??1CRezBufferObject@@UAE@XZ)
