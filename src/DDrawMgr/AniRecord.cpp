#include <rva.h>

#include <DDrawMgr/AniRecord.h>

#include <Mfc.h>

#include <DDrawMgr/AniRecordBase2.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawDeviceManager.h>
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
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>

#include <string.h>

DATA(0x002bf3c4)
i32 g_aniParsedNameLen = 0;

RVA(0x00168c60, 0xa0)
i32 CAniRecordView::Parse(SoundCueRegistry* ctx, const i16* src) {
    const i16* p = src;
    m_flags = static_cast<u16>(*p++);
    m_stepMode = static_cast<WwdAnimStepMode>(*p++);
    m_loopMode = static_cast<WwdAnimLoopMode>(*p++);
    m_positionMode = static_cast<WwdAnimPositionMode>(*p++);
    m_param = *p++;
    m_frameTime = *p++;
    m_drawValue = *p++;
    m_positionDeltaX = *p++;
    m_positionDeltaY = *p++;
    m_reserved28 = static_cast<u16>(*p++);
    m_cues = NULL;
    m_cueCount = 0;
    g_aniParsedNameLen = 0;
    if (m_flags & 0x2) {

        Pix16CPtr np;
        np.m_swords = p;
        const char* name = np.m_chars;
        g_aniParsedNameLen = static_cast<i32>(strlen(name)) + 1;
        ResolveIndices(ctx, name);
    }
    return 1;
}

// @early-stop
// The GetAt(i) return is the CString temporary consumed directly by Lookup, and
// the comma expression keeps it alive through the m_cues[i] store.
RVA(0x00168d00, 0x14c)
void CAniRecordView::ResolveIndices(SoundCueRegistry* owner, const char* str) {
    if (owner == NULL || str == NULL) {
        return;
    }
    CStringArray tokens;
    char tok[0x80];
    i32 n = 0;
    const char* s = str;
    while (*s != 0) {
        char c = *s;
        if (c > 0x21) {
            tok[n++] = c;
        } else {
            tok[n] = 0;
            if (n > 0) {
                tokens.SetAtGrow(tokens.GetSize(), tok);
            }
            n = 0;
        }
        s++;
    }
    tok[n] = 0;
    if (n > 0) {
        tokens.SetAtGrow(tokens.GetSize(), tok);
    }
    m_cueCount = tokens.GetSize();
    if (m_cueCount > 0) {
        m_cues = new SoundCue*[m_cueCount];
        for (i32 i = 0; i < m_cueCount; i++) {
            SoundCue* v = NULL;
            m_cues[i] = (MapLookup(owner->m_cues, tokens.GetAt(i), v), v);
        }
    }
}

RVA(0x00168e50, 0x1e)
i32 CAniRecordView::GetSize() {
    i32 n = m_frameTime;
    i32 size = 0x16;
    if (n > 0) {
        if (m_flags & 0x1) {
            size = n * 22;
        } else {
            size = n;
        }
    }
    return size;
}

RVA_COMPGEN(0x00168e70, 0x27, ?GetAt@CStringArray@@QBE?AVCString@@H@Z)

RVA(0x00168ea0, 0x40)
i32 CAniRecordBase2::LoadPaletteFromFile(char* path, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_deviceManager->LoadPaletteFromFile(path, 0x44);
    m_buf = buf;
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
i32 CAniRecordBase2::CreatePaletteFromRgb(u8* data, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_deviceManager->CreateRgbPalette(data, 0x44);
    m_buf = buf;
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
i32 CAniRecordBase2::CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_deviceManager->CreatePaletteFromEntries(entries, 0x44);
    m_buf = buf;
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
i32 CAniRecordBase2::CreatePaletteFromTrailingData(void* data, i32 size, i32 flag) {
    CDDPalette* buf = OwnerMgr()->m_deviceManager->CreatePaletteFromTrailingData(data, size, 0x44);
    m_buf = buf;
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
void CAniRecordBase2::Unload() {
    CDDPalette* buf = m_buf;
    if (buf != NULL) {
        OwnerMgr()->m_deviceManager->RemovePalette(buf);
        m_buf = NULL;
    }
}

RVA(0x00168fd0, 0x24)
i32 CAniRecordBase2::PushPalette() {
    CDDrawSurfaceChildA* sd = OwnerMgr()->m_drawTarget->m_frontPair;
    if (sd->m_bpp != BPP_PALETTED_8) {
        return 1;
    }
    return sd->m_surface->SetPalette(m_buf, 0);
}
