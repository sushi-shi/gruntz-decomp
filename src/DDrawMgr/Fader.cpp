#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <EmptyString.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderMode.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/FxModeDesc.h>
#include <Gruntz/FxModeT1.h>
#include <Ints.h>
#include <Utils/RecordFill.h>
#include <Wap32/ScreenGeometry.h>

#include <ddraw.h>
#include <math.h>
#include <string.h>

RVA(0x0017e450, 0x23)
CFader::CFader() {
    m_table = NULL;
    m_flag = 1;
}

RVA_COMPGEN(0x0017e480, 0x1e, ??_GCFader@@UAEPAXI@Z)
RVA(0x0017e4a0, 0x69)
CFader::~CFader() {
    if (m_table && m_flag) {
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
    if (count >= 1) {
        do {
            if (vsync && m_ptrColl) {
                m_ptrColl->m_device->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
            }
            RenderFrame(frame);
            loops++;
            frame += step;
        } while (frame <= count);
    }
    if (frame != count) {
        RenderFrame(count);
        loops++;
    }
    float fLoops = static_cast<float>(loops);
    DWORD elapsed = GetTickCount() - startTick;
    m_measuredFps = static_cast<i32>((fLoops / (static_cast<float>(elapsed) * 0.001f)));
    EndFade();
}

// @early-stop
RVA(0x0017e620, 0x13b)
void CFader::RunFade(u32 dur, i32 lead, i32 vsync) {
    i32 prev = 0;
    i32 frame = 0;
    i32 count = GetFrameCount();
    if (count < 1) {
        return;
    }
    BeginFade();
    RenderFrame(0);
    Wait(lead);
    i32 loops = 0;
    DWORD startTick = GetTickCount();
    float fStart = static_cast<float>(startTick);
    float fDur = static_cast<float>(dur);
    float fCount = static_cast<float>(count);
    if (count >= 0) {
        do {
            frame =
                static_cast<i32>(((static_cast<float>(GetTickCount()) - fStart) / fDur * fCount));
            if (prev != frame && frame <= count && frame > 0) {
                if (vsync && m_ptrColl) {
                    m_ptrColl->m_device->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
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
    DWORD elapsed = GetTickCount() - startTick;
    m_measuredFps = static_cast<i32>((fLoops / (static_cast<float>(elapsed) * 0.001f)));
    EndFade();
}

RVA(0x0017e760, 0x11)
void CFader::SetTimers(CDDSurface* a, CDDSurface* b) {
    m_timerA = a;
    m_timerB = b;
}

RVA(0x0017e780, 0xa)
void CFader::Set2c(CDDrawPtrCollections* pool) {
    m_ptrColl = pool;
}

RVA(0x0017e790, 0x1)
void CFader::BeginFade() {}

RVA(0x0017e7a0, 0x1)
void CFader::EndFade() {}

RVA(0x0017e7b0, 0x9)
CFxModeDesc::CFxModeDesc() {
    m_type = FXMODE_UNTAGGED;
}

RVA(0x0017e7c0, 0x7a)
CFxModeT1::CFxModeT1() {
    m_type = FXMODE_SHAPE;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_warpSourceSurface = NULL;
    m_halfWidth = 0x32;
    m_mode = FADER_SWEEP_FORWARD;
    m_stripCopy = 1;
    m_useLut = 0;
    m_shadeTable = NULL;
    m_shadeTablePath = g_emptyString;
    m_palette = NULL;
}

// @early-stop
RVA(0x0017e840, 0x37)
CFxModeT2::CFxModeT2() {
    m_type = FXMODE_LIGHT;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_clearMode = 1;
    m_spanCount = 0;
    m_centerX = SCREEN_HALF_W_PX;
    m_centerY = SCREEN_HALF_H_PX;
    m_shadeTable = NULL;
}

RVA(0x0017e880, 0x28)
CFxModeT3::CFxModeT3() {
    m_type = FXMODE_SINE;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_clearToBlack = 1;
    m_intensityPercent = 0xf;
}

RVA(0x0017e8b0, 0x27)
CFxModeT4::CFxModeT4() {
    m_type = FXMODE_RADIAL;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_palette = NULL;
    m_shadeTable = NULL;
    m_param0c = 1;
}

RVA(0x0017e8e0, 0x27)
CFxModeT5::CFxModeT5() {
    m_type = FXMODE_FLAT;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_param0c = 0;
    m_splitPercent = 0;
    m_durationPercent = 0x19;
}

RVA(0x0017e910, 0x29)
CFxModeT6::CFxModeT6() {
    m_type = FXMODE_MESH;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_flipTarget = NULL;
    m_reverseOrder = 0;
    m_param18 = 0;
    m_cols = 0;
    m_rows = 0;
}

RVA(0x0017e940, 0x27)
CFaderMesh::CFaderMesh() {}

RVA_COMPGEN(0x0017e970, 0x1e, ??_GCFaderMesh@@UAEPAXI@Z)
RVA(0x0017e990, 0x6b)
CFaderMesh::~CFaderMesh() {}

// @early-stop
RVA(0x0017ea00, 0x4fc)
i32 CFaderMesh::ApplyInit(CFxModeDesc* descOpaque) {

    CFxModeT6* cfg = static_cast<CFxModeT6*>(descOpaque);

    if (cfg->m_targetSurface == NULL) {
        m_dstSurface = m_timerA;
    } else {
        m_dstSurface = cfg->m_targetSurface;
    }
    if (cfg->m_sourceSurface == NULL) {
        m_bltSrc = m_timerB;
    } else {
        m_bltSrc = cfg->m_sourceSurface;
    }
    if (cfg->m_flipTarget == NULL) {
        return 0;
    }
    m_primeSrc = cfg->m_primeSource;
    m_flipTarget = cfg->m_flipTarget;
    m_desc18 = cfg->m_param18;
    m_recOrderFlag = cfg->m_reverseOrder;
    m_cols = cfg->m_cols;
    m_rows = cfg->m_rows;

    CRezBufferObject* mesh = &m_meshBuf;
    mesh->SetSize(0, -1);

    i32 halfH = m_dstSurface->m_width / 2;
    i32 halfW = m_dstSurface->m_height / 2;
    i32 dx = m_bltSrc->m_width / m_cols;
    i32 dy = m_bltSrc->m_height / m_rows;
    float radius = static_cast<float>(sqrt(static_cast<double>((dx * dx + dy * dy))));
    if (m_rows <= 0) {
        return 1;
    }

    for (i32 row = 0; row < m_rows; row++) {
        i32 cellW2 = halfW * halfW;
        i32 cellD = halfW * halfW + halfH * halfH;
        float cellR = sqrt(static_cast<double>(cellD)) + radius - g_fxBias;
        if (m_cols <= 0) {
            continue;
        }
        for (i32 col = 0; col < m_cols; col++) {
            i32 d2 = halfH * halfH + cellW2;
            double v = sqrt(static_cast<double>(d2));
            float normX, normY;
            if (v > g_fxEps) {
                normY = static_cast<float>((row - halfH) / v);
                normX = static_cast<float>((col - halfW) / v);
            } else {
                normY = 0.0f;
                normX = 1.0f;
            }

            RECT pt48;
            pt48.left = 0;
            pt48.top = 0;
            pt48.right = dx;
            pt48.bottom = dy;
            OffsetRect(&pt48, row, col);
            i32 ox = static_cast<i32>((cellR * normX));
            i32 oy = static_cast<i32>((cellR * normY));
            OffsetRect(&pt48, oy, ox);

            RECT pt64;
            pt64.left = 0;
            pt64.top = 0;
            pt64.right = d2;
            pt64.bottom = dy;
            OffsetRect(&pt64, row, col);

            RezElem40 elem;
            if (m_recOrderFlag) {
                elem.m_startRect = pt64;
                elem.m_endRect = pt48;
            } else {
                elem.m_startRect = pt48;
                elem.m_endRect = pt64;
            }
            elem.m_reserved20 = 0;
            elem.m_scale = 1.0f;

            i32 idx = mesh->m_nSize;
            i32 newSize = idx + 1;
            // Reserve raw capacity: unused serialized mesh slots are zero-filled explicitly.
            if (newSize == 0) {
                if (mesh->m_pData) {
                    ::operator delete(mesh->m_pData);
                    mesh->m_pData = NULL;
                }
                mesh->m_nMaxSize = 0;
                mesh->m_nSize = 0;
            } else if (mesh->m_pData == NULL) {
                mesh->m_pData =
                    static_cast<RezElem40*>(::operator new(newSize * sizeof(RezElem40)));
                memset(mesh->m_pData, 0, newSize * sizeof(RezElem40));
                mesh->m_nMaxSize = newSize;
                mesh->m_nSize = newSize;
            } else if (newSize <= mesh->m_nMaxSize) {
                if (newSize > idx) {
                    memset(&mesh->m_pData[idx], 0, (newSize - idx) * sizeof(RezElem40));
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
                i32 newMax = mesh->m_nMaxSize + grow;

                if (newSize >= newMax) {
                    newMax = newSize;
                }
                RezElem40* nd = static_cast<RezElem40*>(::operator new(newMax * sizeof(RezElem40)));
                memcpy(nd, mesh->m_pData, mesh->m_nSize * sizeof(RezElem40));
                memset(&nd[mesh->m_nSize], 0, (newSize - mesh->m_nSize) * sizeof(RezElem40));
                ::operator delete(mesh->m_pData);
                mesh->m_pData = nd;
                mesh->m_nSize = newSize;
                mesh->m_nMaxSize = newMax;
            }
            memcpy(&mesh->m_pData[idx], &elem, sizeof(RezElem40));
        }
    }
    return 1;
}

// @early-stop
RVA(0x0017ef00, 0x21c)
void CFaderMesh::RenderFrame(i32 frame) {
    CDDSurface* dst = m_dstSurface;
    if (m_primeSrc != NULL) {
        dst->Blt(m_primeSrc);
    } else {
        dst->Clear(0);
    }
    if (m_meshBuf.m_nSize > 0) {
        float ff = static_cast<float>(frame);
        RezElem40* recs = m_meshBuf.m_pData;
        for (i32 i = 0; i < m_meshBuf.m_nSize; i++) {
            RezElem40* rec = &recs[i];
            i32 r0 = rec->m_startRect.left, r1 = rec->m_startRect.top;
            i32 r2 = rec->m_startRect.right, r3 = rec->m_startRect.bottom;
            i32 r4 = rec->m_endRect.left, r5 = rec->m_endRect.top;
            i32 r6 = rec->m_endRect.right, r7 = rec->m_endRect.bottom;
            float t = ff / static_cast<float>(GetFrameCount());

            i32 x0 = r0 + static_cast<i32>((static_cast<float>((r4 - r0)) * t));
            i32 y0 = r1 + static_cast<i32>((static_cast<float>((r5 - r1)) * t));
            i32 x1 = r2 + static_cast<i32>((static_cast<float>((r6 - r2)) * t));
            i32 y1 = r3 + static_cast<i32>((static_cast<float>((r7 - r3)) * t));

            i32 bx0 = r4, by0 = r5, bx1 = r6, by1 = r7;
            if (x0 < 0 && x1 > 0) {
                bx0 = r4 - x0;
                x0 = 0;
            } else if (x1 <= dst->m_width && x0 > dst->m_width) {
                bx1 = r6 - x1 + dst->m_width;
                x1 = dst->m_width - 1;
            }
            if (y0 < 0 && y1 > 0) {
                by0 = r5 - y0;
                y0 = 0;
            } else if (y1 <= dst->m_height && y0 > dst->m_height) {
                by1 = r7 - y1 + dst->m_height;
                y1 = dst->m_height - 1;
            }

            RECT dstRect = {x0, y0, x1, y1};
            RECT srcRect;
            if (m_recOrderFlag != 0) {
                srcRect.left = r0;
                srcRect.top = r1;
                srcRect.right = r2;
                srcRect.bottom = r3;
            } else {
                srcRect.left = bx0;
                srcRect.top = by0;
                srcRect.right = bx1;
                srcRect.bottom = by1;
            }
            dst->BltEx(&dstRect, m_bltSrc, &srcRect, 0x1000000, 0);
        }
    }
    m_flipTarget->Flip(0);
}

RVA(0x0017f120, 0x6)
i32 CFaderMesh::GetFrameCount() {
    return 0x1f4;
}
