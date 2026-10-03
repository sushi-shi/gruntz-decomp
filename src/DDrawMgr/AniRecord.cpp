#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/AniRecord.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawPaletteResource.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Enums.h>
#include <Gruntz/AniRecordView.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Ints.h>
#include <Pix16.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>

#include <string.h>

i32 g_aniParsedNameLen = 0;

i32 CAniRecordView::Parse(SoundCueRegistry* ctx, const i16* src) {
    const i16* p = src;
    m_flags = static_cast<u16>(*p++);
    m_stepMode = static_cast<WwdAnimStepMode>(*p++);
    m_loopMode = static_cast<WwdAnimLoopMode>(*p++);
    m_positionMode = static_cast<WwdAnimPositionMode>(*p++);
    m_param = *p++;
    m_duration = *p++;
    m_drawValue = *p++;
    m_positionDeltaX = *p++;
    m_positionDeltaY = *p++;
    m_reserved28 = static_cast<u16>(*p++);
    m_cues = NULL;
    m_cueCount = 0;
    g_aniParsedNameLen = 0;
    if (HAS(m_flags, ANI_RECORD_FLAG_HAS_CUES)) {

        Pix16CPtr np;
        np.m_swords = p;
        const char* name = np.m_chars;
        g_aniParsedNameLen = static_cast<i32>(strlen(name)) + 1;
        ResolveIndices(ctx, name);
    }
    return 1;
}

void CAniRecordView::ResolveIndices(SoundCueRegistry* owner, const char* str) {
    if (owner == NULL || str == NULL) {
        return;
    }
    std::vector<std::string> tokens;
    char tok[0x80];
    i32 n = 0;
    const char* s = str;
    while (*s != 0) {
        char c = *s;
        if (c > '!') {
            tok[n++] = c;
        } else {
            tok[n] = 0;
            if (n > 0) {
                tokens.push_back(tok);
            }
            n = 0;
        }
        s++;
    }
    tok[n] = 0;
    if (n > 0) {
        tokens.push_back(tok);
    }
    m_cueCount = static_cast<i32>(tokens.size());
    if (m_cueCount > 0) {
        m_cues = new SoundCue*[m_cueCount];
        for (i32 i = 0; i < m_cueCount; i++) {
            m_cues[i] = owner->FindCue((tokens[i]).c_str());
        }
    }
}

i32 CAniRecordView::GetDurationMs() {
    i32 duration = m_duration;
    i32 durationMs = ANI_FRAME_QUANTUM_MS;
    if (duration > 0) {
        if (HAS(m_flags, ANI_RECORD_FLAG_FRAME_COUNT)) {
            durationMs = duration * ANI_FRAME_QUANTUM_MS;
        } else {
            durationMs = duration;
        }
    }
    return durationMs;
}

i32 CDDrawPaletteResource::LoadPaletteFromFile(char* path, i32 flag) {
    CDDPalette* buf =
        OwnerMgr()->GetDeviceManager()->LoadPaletteFromFile(path, DDPCAPS_8BIT | DDPCAPS_ALLOW256);
    m_palette = buf;
    if (buf == NULL) {
        return 0;
    }
    if (flag & 0x1) {
        m_flags |= 0x1;
        buf->CaptureSystemPalette();
    }
    return 1;
}

i32 CDDrawPaletteResource::CreatePaletteFromRgb(u8* data, i32 flag) {
    CDDPalette* buf =
        OwnerMgr()->GetDeviceManager()->CreateRgbPalette(data, DDPCAPS_8BIT | DDPCAPS_ALLOW256);
    m_palette = buf;
    if (buf == NULL) {
        return 0;
    }
    if (flag & 0x1) {
        m_flags |= 0x1;
        buf->CaptureSystemPalette();
    }
    return 1;
}

i32 CDDrawPaletteResource::CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flag) {
    CDDPalette* buf = OwnerMgr()->GetDeviceManager()->CreatePaletteFromEntries(
        entries,
        DDPCAPS_8BIT | DDPCAPS_ALLOW256
    );
    m_palette = buf;
    if (buf == NULL) {
        return 0;
    }
    if (flag & 0x1) {
        m_flags |= 0x1;
        buf->CaptureSystemPalette();
    }
    return 1;
}

i32 CDDrawPaletteResource::CreatePaletteFromTrailingData(void* data, i32 size, i32 flag) {
    CDDPalette* buf = OwnerMgr()->GetDeviceManager()->CreatePaletteFromTrailingData(
        data,
        size,
        DDPCAPS_8BIT | DDPCAPS_ALLOW256
    );
    m_palette = buf;
    if (buf == NULL) {
        return 0;
    }
    if (flag & 0x1) {
        m_flags |= 0x1;
        buf->CaptureSystemPalette();
    }
    return 1;
}

void CDDrawPaletteResource::Unload() {
    CDDPalette* buf = m_palette;
    if (buf != NULL) {
        OwnerMgr()->GetDeviceManager()->RemovePalette(buf);
        m_palette = NULL;
    }
}

i32 CDDrawPaletteResource::ApplyToFrontSurface() {
    CDDrawFrontSurface* sd = OwnerMgr()->GetDrawTarget()->GetFrontSurface();
    if (sd->m_bpp != BPP_PALETTED_8) {
        return 1;
    }
    return sd->GetSurface()->SetPalette(m_palette, 0);
}
