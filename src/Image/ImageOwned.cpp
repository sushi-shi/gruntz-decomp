#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Ints.h>
#include <Io/FileStream.h>
#include <Pix16.h>

#include <ddraw.h>
#include <string.h>

RVA(0x00148ce0, 0x2f)
CDDrawShadeBlit::CDDrawShadeBlit() {
    m_rleData = NULL;
    m_rleLen = 0;
    m_palDescr = NULL;
    m_drawType = SHADE_COPY;
    m_light = 0x80;
    m_doubleScanlines = 0;
    m_palette = NULL;
    m_srcBpp = 1;
    m_dstBpp = 1;
    m_colorKey = -1;
}

RVA(0x00148d10, 0x25)
void CDDrawShadeBlit::Teardown() {
    if (m_rleData) {
        ::operator delete(m_rleData);
    }
    if (m_palette) {
        ::operator delete(m_palette);
    }
}

// @early-stop
RVA(0x00148d40, 0x202)
i32 CDDrawShadeBlit::BuildRle(
    void* pixels,
    i32 width,
    i32 height,
    i32 stride,
    i32 keyVal,
    void* palette
) {
    u8* src = static_cast<u8*>(pixels);
    if (src == NULL) {
        return 0;
    }
    m_colorKey = keyVal;
    if (stride == -1) {
        stride = width;
    }
    m_width = width;
    m_height = height;

    CByteArray ba;
    ba.SetSize(0x3e8, 0);

    i32 row = 0;
    if (m_height > 0) {
        do {
            i32 i = 0;
            i32 runStart = 0;
            if (m_width > 0) {
                do {
                    if (static_cast<i32>(src[i]) != keyVal) {

                        while (i < m_width && (i - runStart) < 0x7e
                               && static_cast<i32>(src[i]) != keyVal) {
                            i++;
                        }
                        ba.SetAtGrow(ba.GetSize(), static_cast<u8>((i - runStart)));
                        for (i32 j = runStart; j < i; j++) {
                            ba.SetAtGrow(ba.GetSize(), src[j]);
                        }
                        runStart = i;
                    } else {

                        while (i < m_width && (i - runStart) < 0x7e
                               && static_cast<i32>(src[i]) == keyVal) {
                            i++;
                        }
                        ba.SetAtGrow(ba.GetSize(), static_cast<u8>(((i - runStart) | 0x80)));
                        runStart = i;
                    }
                } while (i < m_width);
            }
            row++;
            src += stride;
        } while (row < m_height);
    }

    if (m_rleData != NULL) {
        ::operator delete(m_rleData);
    }
    m_rleLen = ba.GetSize();
    m_rleData = static_cast<u8*>(::operator new(ba.GetSize()));
    i32 n = m_rleLen;
    for (i32 k = 0; k < n; k++) {
        m_rleData[k] = ba.GetData()[k];
    }

    if (palette != NULL) {
        if (m_palette != NULL) {
            ::operator delete(m_palette);
        }
        m_palette = static_cast<PALETTEENTRY*>(::operator new(0x400));
        memcpy(m_palette, palette, 0x400);
    }
    return 1;
}

RVA(0x00148f50, 0x61)
i32 CDDrawShadeBlit::BuildFromSurface(CDDSurface* surf, i32 keyVal, void* palette) {
    if (surf == NULL) {
        return 0;
    }
    m_colorKey = keyVal;
    void* bits = surf->Lock(0);
    if (bits == NULL) {
        return 0;
    }
    i32 r = BuildRle(bits, surf->m_width, surf->m_height, surf->m_pitch, keyVal, palette);
    surf->m_ddSurface->Unlock(0);
    return r;
}

RVA(0x00148fc0, 0x104)
i32 CDDrawShadeBlit::LoadFromFile(CString name, i32 fmt) {
    CFile file;
    if (!file.Open(name, 0x8000, 0)) {
        return 0;
    }
    void* buf = ::operator new(file.GetLength());
    file.Read(buf, file.GetLength());
    i32 r = Build(static_cast<PidHeader*>(buf), file.GetLength(), fmt);
    file.Close();
    ::operator delete(buf);
    return r;
}

// @early-stop
RVA(0x001490d0, 0x173)
i32 CDDrawShadeBlit::Build(PidHeader* src, i32 size, i32 fmt) {
    i32 flags = src->flags;

    if ((HAS(flags, PID_SRC_8BPP_SHADE)) || (HAS(flags, PID_SRC_8BPP))) {
        if (static_cast<u8>(fmt) == 0x10) {
            m_srcBpp = 1;
            m_dstBpp = 2;
        } else {
            m_srcBpp = 1;
            m_dstBpp = 1;
        }
    } else if (static_cast<u8>(fmt) == 0x10) {
        m_srcBpp = 2;
        m_dstBpp = 2;
    } else {
        m_srcBpp = 1;
        m_dstBpp = 1;
    }

    if (HAS(src->flags, PID_FILL_IS_WORD)) {
        m_colorKey = static_cast<u8>(src->fill);
    } else {
        m_colorKey = -1;
    }

    i32 stride = size - 0x20;
    m_rleLen = stride;
    if (static_cast<u8>(fmt) != 0x8 && static_cast<u8>(fmt) != 0x10) {
        return 0;
    }

    if (HAS(src->flags, PID_EMBEDDED_PALETTE)) {
        stride -= 0x300;
        m_rleLen = stride;
        if (static_cast<u8>(fmt) == 0x10) {
            if (m_palette != NULL) {
                ::operator delete(m_palette);
            }
            m_palette = static_cast<PALETTEENTRY*>(::operator new(0x400));

            RecordBytes<PidHeader> blob;
            blob.m_rec = src;
            i32 i = 0;
            i32 d = 0;
            do {
                d++;
                m_palette[d - 1].peRed = (blob.m_bytes + m_rleLen)[i + 0x20];
                i += 3;
                m_palette[d - 1].peGreen = (blob.m_bytes + m_rleLen)[i + 0x1e];
                m_palette[d - 1].peBlue = (blob.m_bytes + m_rleLen)[i + 0x1f];
            } while (i < 0x300);
        }
    }

    m_width = src->width;
    m_height = src->height;
    if (m_rleData != NULL) {
        ::operator delete(m_rleData);
    }
    m_rleData = static_cast<u8*>(::operator new(m_rleLen));

    memcpy(m_rleData, src + 1, m_rleLen);

    if (m_srcBpp == 2) {
        void* remapped = EncodeRle16(m_rleData);
        ::operator delete(m_rleData);
        m_rleData = static_cast<u8*>(remapped);
        ::operator delete(m_palette);
        m_palette = NULL;
    }
    return 1;
}

// @early-stop
RVA(0x00149250, 0x158)
i32 CDDrawShadeBlit::DecodeFrame(CString name, CImageFrameRebuildDesc desc) {
    if (m_srcBpp != 1) {
        return 0;
    }

    CFile file;
    if (file.Open(name, 0x9001, 0) == 0) {
        return 0;
    }
    file.Write(&desc, sizeof(desc));
    file.Write(m_rleData, m_rleLen);
    if (desc.f1 & 0x80) {
        if (m_palette == NULL) {
            return 0;
        }
        for (i32 i = 0; i < 0x100; i++) {
            file.Write(&m_palette[i].peRed, sizeof(m_palette[i].peRed));
            file.Write(&m_palette[i].peGreen, sizeof(m_palette[i].peGreen));
            file.Write(&m_palette[i].peBlue, sizeof(m_palette[i].peBlue));
        }
    }
    file.Close();
    return 1;
}

RVA(0x001493b0, 0xfd)

i32 CDDrawShadeBlit::Rebuild(CString name, i32 offsetX, i32 offsetY) {
    if (m_srcBpp != 1) {
        return 0;
    }
    CImageFrameRebuildDesc desc;
    i32 flags = 0x3d;
    if (m_palette != NULL) {
        flags = 0xbd;
    }
    desc.f0 = 0;
    desc.f2 = m_width;
    desc.f4 = offsetX;
    desc.f3 = m_height;
    desc.f5 = offsetY;
    desc.f6 = 0;
    desc.f7 = 0;
    if (m_colorKey != -1) {
        flags |= 0x100;
        desc.f6 = static_cast<u8>(m_colorKey);
    }
    if (m_palette != NULL) {
        flags |= 0x80;
    }
    desc.f1 = flags;
    return DecodeFrame(name, desc);
}

RVA(0x001494b0, 0x11a)
i32 CDDrawShadeBlit::Decompress(void* dest) {
    if (m_srcBpp != 1) {
        return 0;
    }
    if (dest == NULL) {
        return 0;
    }
    i32 fill = m_colorKey;
    if (fill == -1) {
        fill = 0;
    }
    i32 x = 0;
    i32 cursor = 0;
    for (i32 y = 0; y < m_height;) {
        if (m_rleData[cursor] & 0x80) {
            memset(static_cast<u8*>(dest) + y * m_width + x, fill, m_rleData[cursor] - 0x80);
            x += m_rleData[cursor] - 0x80;
            cursor += 1;
        } else {
            memcpy(
                static_cast<u8*>(dest) + y * m_width + x,
                m_rleData + cursor + 1,
                m_rleData[cursor]
            );
            x += m_rleData[cursor];
            cursor += m_rleData[cursor] + 1;
        }
        if (x >= m_width) {
            y++;
            x = 0;
        }
    }
    return 1;
}
