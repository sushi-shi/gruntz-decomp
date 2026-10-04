#include <StdAfx.h>

#include <rva.h>

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

DATA(0x002bf3c4)
i32 g_aniParsedCueListBytes = 0;

RVA(0x00168c60, 0xa0)
i32 CAniFrameRecord::Parse(SoundCueRegistry* soundRegistry, const i16* recordWords) {
    const i16* p = recordWords;
    m_flags = static_cast<u16>(*p++);
    m_stepMode = static_cast<WwdAnimStepMode>(*p++);
    m_loopMode = static_cast<WwdAnimLoopMode>(*p++);
    m_positionMode = static_cast<WwdAnimPositionMode>(*p++);
    m_frameParameter = *p++;
    m_duration = *p++;
    m_eventCode = *p++;
    m_positionParameterX = *p++;
    m_positionParameterY = *p++;
    m_reserved28 = static_cast<u16>(*p++);
    m_cues = NULL;
    m_cueCount = 0;
    g_aniParsedCueListBytes = 0;
    if (HAS(m_flags, ANI_RECORD_FLAG_HAS_CUES)) {

        Pix16CPtr cueNameBytes;
        cueNameBytes.m_swords = p;
        const char* cueNames = cueNameBytes.m_chars;
        g_aniParsedCueListBytes = static_cast<i32>(strlen(cueNames)) + 1;
        ResolveSoundCues(soundRegistry, cueNames);
    }
    return 1;
}

RVA(0x00168d00, 0x14c)
void CAniFrameRecord::ResolveSoundCues(SoundCueRegistry* soundRegistry, const char* cueNames) {
    if (soundRegistry == NULL || cueNames == NULL) {
        return;
    }
    CStringArray cueNamesByIndex;
    char cueNameBuffer[0x80];
    i32 cueNameLength = 0;
    const char* cursor = cueNames;
    while (*cursor != 0) {
        char character = *cursor;
        if (character > '!') {
            cueNameBuffer[cueNameLength++] = character;
        } else {
            cueNameBuffer[cueNameLength] = 0;
            if (cueNameLength > 0) {
                cueNamesByIndex.Add(cueNameBuffer);
            }
            cueNameLength = 0;
        }
        cursor++;
    }
    cueNameBuffer[cueNameLength] = 0;
    if (cueNameLength > 0) {
        cueNamesByIndex.Add(cueNameBuffer);
    }
    m_cueCount = cueNamesByIndex.GetSize();
    if (m_cueCount > 0) {
        m_cues = new SoundCue*[m_cueCount];
        for (i32 i = 0; i < m_cueCount; i++) {
            m_cues[i] = soundRegistry->FindCue(cueNamesByIndex.GetAt(i));
        }
    }
}

RVA(0x00168e50, 0x1e)
i32 CAniFrameRecord::GetDurationMs() {
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

RVA_COMPGEN(0x00168e70, 0x27, ?GetAt@CStringArray@@QBE?AVCString@@H@Z)

RVA(0x00168ea0, 0x40)
i32 CDDrawPaletteResource::LoadPaletteFromFile(char* path, i32 flag) {
    CDDPalette* buf =
        GetWorld()->GetDeviceManager()->LoadPaletteFromFile(path, DDPCAPS_8BIT | DDPCAPS_ALLOW256);
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

RVA(0x00168ee0, 0x40)
i32 CDDrawPaletteResource::CreatePaletteFromRgb(u8* data, i32 flag) {
    CDDPalette* buf =
        GetWorld()->GetDeviceManager()->CreateRgbPalette(data, DDPCAPS_8BIT | DDPCAPS_ALLOW256);
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

RVA(0x00168f20, 0x40)
i32 CDDrawPaletteResource::CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flag) {
    CDDPalette* buf = GetWorld()->GetDeviceManager()->CreatePaletteFromEntries(
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

RVA(0x00168f60, 0x45)
i32 CDDrawPaletteResource::CreatePaletteFromTrailingData(void* data, i32 size, i32 flag) {
    CDDPalette* buf = GetWorld()->GetDeviceManager()->CreatePaletteFromTrailingData(
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

RVA(0x00168fb0, 0x1f)
void CDDrawPaletteResource::Unload() {
    CDDPalette* buf = m_palette;
    if (buf != NULL) {
        GetWorld()->GetDeviceManager()->RemovePalette(buf);
        m_palette = NULL;
    }
}

RVA(0x00168fd0, 0x24)
i32 CDDrawPaletteResource::ApplyToFrontSurface() {
    CDDrawFrontSurface* sd = GetWorld()->GetDisplayBuffers()->GetFrontSurface();
    if (sd->m_bpp != BPP_PALETTED_8) {
        return 1;
    }
    return sd->GetSurface()->SetPalette(m_palette, 0);
}
