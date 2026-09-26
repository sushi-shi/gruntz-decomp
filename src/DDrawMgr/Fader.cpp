#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderMode.h>
#include <Gruntz/ShapeFaderConfig.h>
#include <Wap32/ScreenGeometry.h>

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
    m_measuredFps =
        static_cast<i32>((fLoops / (static_cast<float>(GetTickCount() - startTick) * 0.001f)));
    EndFade();
}

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
    m_measuredFps =
        static_cast<i32>((fLoops / (static_cast<float>(GetTickCount() - startTick) * 0.001f)));
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
    m_kind = FADER_CONFIG_LIGHT;
    m_targetSurface = NULL;
    m_sourceSurface = NULL;
    m_clearMode = true;
    m_spanCount = 0;
    m_center = CPoint(SCREEN_HALF_W_PX, SCREEN_HALF_H_PX);
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
