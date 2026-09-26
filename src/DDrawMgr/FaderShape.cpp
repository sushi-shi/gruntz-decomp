#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PixelShift.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderBufferInline.h>
#include <Gruntz/FaderCopyMacros.h>
#include <Gruntz/FaderMode.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/ShapeFaderConfig.h>

#include <math.h>

RVA(0x001816c0, 0x32)
CFaderShape::CFaderShape() {
    m_warpTable = NULL;
    m_targetRowOffsets = NULL;
    m_sourceRowOffsets = NULL;
    m_warpRowOffsets = NULL;
    m_lineBuf = NULL;
    m_shadeRamp = NULL;
    m_previousFrame = 0;
}

RVA_COMPGEN(0x00181700, 0x1e, ??_GCFaderShape@@UAEPAXI@Z)
RVA(0x00181720, 0xb3)
CFaderShape::~CFaderShape() {
    if (m_warpTable) {
        delete[] m_warpTable;
    }
    if (m_targetRowOffsets) {
        delete[] m_targetRowOffsets;
    }
    if (m_sourceRowOffsets) {
        delete[] m_sourceRowOffsets;
    }
    if (m_warpRowOffsets) {
        delete[] m_warpRowOffsets;
    }
    if (m_lineBuf) {
        delete[] m_lineBuf;
    }
    if (m_shadeRamp) {
        delete[] m_shadeRamp;
    }
}

RVA(0x001817e0, 0x315)
i32 CFaderShape::ApplyInit(CFaderConfig* desc) {
    CShapeFaderConfig* pInit = static_cast<CShapeFaderConfig*>(desc);
    i32 i;
    i32 mx;
    m_previousFrame = 0;
    if (pInit == NULL) {
        return 0;
    }

    SelectTarget(m_targetSurface, pInit->m_targetSurface);
    SelectSource(m_sourceSurface, pInit->m_sourceSurface);
    if (m_targetSurface == NULL) {
        return 0;
    }
    if (m_sourceSurface == NULL) {
        return 0;
    }
    if (pInit->m_warpSourceSurface == NULL) {
        m_warpSourceSurface = m_sourceSurface;
    } else {
        m_warpSourceSurface = pInit->m_warpSourceSurface;
    }

    if (!m_cache.Init()) {
        return 0;
    }

    m_targetHeight = m_targetSurface->m_apiDesc.dwHeight;
    m_targetWidth = m_targetSurface->m_apiDesc.dwWidth;
    m_sourceHeight = m_sourceSurface->m_apiDesc.dwHeight;
    m_sourceWidth = m_sourceSurface->m_apiDesc.dwWidth;
    m_warpHeight = m_warpSourceSurface->m_apiDesc.dwHeight;
    m_warpWidth = m_warpSourceSurface->m_apiDesc.dwWidth;
    if (m_targetHeight != m_sourceHeight) {
        return 0;
    }
    if (m_targetWidth != m_sourceWidth) {
        return 0;
    }
    if (m_targetHeight != m_warpHeight) {
        return 0;
    }
    if (m_targetWidth != m_warpWidth) {
        return 0;
    }
    if (m_warpHeight != m_sourceHeight) {
        return 0;
    }
    if (m_warpWidth != m_sourceWidth) {
        return 0;
    }

    if (pInit->m_mode <= FADER_INVALID || pInit->m_mode >= FADER_COUNT) {
        return 0;
    }
    m_mode = pInit->m_mode;
    m_stripCopy = pInit->m_stripCopy;
    m_halfWidth = pInit->m_halfWidth;

    if (m_mode == FADER_SWEEP_FORWARD || m_mode == FADER_SWEEP_REVERSE) {
        if (m_targetWidth < static_cast<i32>((static_cast<double>(m_halfWidth) * 3.14159))) {
            return 0;
        }
    }

    m_warpTable = new i32[m_halfWidth * 2];
    for (i = 0; i < 2 * m_halfWidth; i++) {
        m_warpTable[i] = static_cast<i32>(
            (acos(
                 (static_cast<float>(i) - static_cast<float>(m_halfWidth))
                 / static_cast<float>(m_halfWidth)
             )
             * static_cast<float>(m_halfWidth))
        );
    }

    m_useLut = pInit->m_useLut;
    if (m_targetSurface->m_bitDepth != BPP_PALETTED_8) {
        m_useLut = false;
    }

    if (m_useLut != false) {
        if (pInit->m_shadeTable) {
            m_ownsTable = false;
            m_table = pInit->m_shadeTable;
        } else if (_access(pInit->m_shadeTablePath, 0) == 0) {
            m_table = m_cache.AddFromArray(pInit->m_shadeTablePath);
            if (m_table == NULL) {
                m_useLut = false;
            }
        } else {
            CDDPalette* pal = pInit->m_palette;
            m_table = m_cache.FlashTable(
                pal->m_entries,
                FLASH_SHADE_DARK_RAMP_STEPS,
                FLASH_SHADE_BRIGHT_RAMP_STEPS,
                FLASH_SHADE_START_PERCENT,
                FLASH_SHADE_END_PERCENT
            );
        }

        i32 m = m_halfWidth << 1;
        m_shadeRamp = new u8[m];
        for (i = 0; i < m; i++) {
            i32 t = static_cast<i32>(
                (sin(static_cast<float>(i) / static_cast<float>(m) * 3.14f) * -32.0)
            );
            m_shadeRamp[i] = static_cast<u8>((0x10 - t));
        }
    }

    m_targetRowOffsets = new i32[m_targetHeight];
    m_sourceRowOffsets = new i32[m_sourceHeight];
    m_warpRowOffsets = new i32[m_warpHeight];
    for (i = 0; i < m_targetHeight; i++) {
        m_targetRowOffsets[i] = m_targetSurface->m_apiDesc.lPitch * i;
        m_sourceRowOffsets[i] = m_sourceSurface->m_apiDesc.lPitch * i;
        m_warpRowOffsets[i] = m_warpSourceSurface->m_apiDesc.lPitch * i;
    }

    mx = m_targetWidth;
    if (m_targetHeight > m_targetWidth) {
        mx = m_targetHeight;
    }
    m_lineBuf = new u8[m_targetSurface->m_bytesPerPixel * mx];
    return 1;
}

// @early-stop
RVA(0x00181b00, 0x34f)
void CFaderShape::RenderFrame(i32 frame) {
    m_dstBase = static_cast<u8*>(m_targetSurface->Lock(NULL));
    u8* gather = static_cast<u8*>(m_sourceSurface->Lock(NULL));
    m_straightBase = gather;
    if (m_sourceSurface != m_warpSourceSurface) {
        gather = static_cast<u8*>(m_warpSourceSurface->Lock(NULL));
    }
    m_gatherBase = gather;

    i32 stride = m_halfWidth * 2;
    i32 arc = static_cast<i32>(static_cast<double>(m_halfWidth) * 3.14159);
    u32 seam = 0;
    if (m_mode == FADER_SPLIT_FROM_CENTER && m_stripCopy != false) {
        seam = m_targetWidth / 2;
    }
    if (m_stripCopy == false && frame == 0) {
        i32 targetPitch = m_targetSurface->m_apiDesc.lPitch;
        i32 sourcePitch = m_sourceSurface->m_apiDesc.lPitch;
        i32 n = (targetPitch < sourcePitch) ? targetPitch : sourcePitch;
        i32 row = 0;
        while (row < m_targetHeight) {
            u8* src = m_straightBase + m_sourceRowOffsets[row];
            u8* dst = m_dstBase + m_targetRowOffsets[row];
            CopyBytes(dst, src, n);
            row++;
        }
    }
    if (m_stripCopy != false) {
        if (seam + frame <= static_cast<u32>(m_targetWidth - arc - m_halfWidth)) {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderTile(m_targetWidth / 2 + frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderTile(m_targetWidth / 2 - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        } else {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderWarpTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderWarpTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderWarpTile(m_targetWidth / 2 + frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderWarpTile(
                        m_targetWidth - m_targetWidth / 2 - frame - stride,
                        frame - m_previousFrame
                    );
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        }
    }
    if (m_stripCopy == false) {
        if (seam + frame > static_cast<u32>(arc - m_halfWidth)) {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderTile(frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        } else {
            switch (m_mode) {
                case FADER_SWEEP_FORWARD:
                    RenderWarpTile(frame, frame - m_previousFrame);
                    break;
                case FADER_SWEEP_REVERSE:
                    RenderWarpTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    break;
                case FADER_SPLIT_FROM_CENTER:
                    m_mode = FADER_SWEEP_FORWARD;
                    RenderWarpTile(frame, frame - m_previousFrame);
                    m_mode = FADER_SWEEP_REVERSE;
                    RenderWarpTile(m_targetWidth - frame - stride, frame - m_previousFrame);
                    m_mode = FADER_SPLIT_FROM_CENTER;
                    break;
            }
        }
    }
    m_previousFrame = frame;
    m_targetSurface->Unlock();
    m_sourceSurface->Unlock();
    if (m_sourceSurface != m_warpSourceSurface) {
        m_warpSourceSurface->Unlock();
    }
}

RVA(0x00181e50, 0x7b9)

void CFaderShape::RenderWarpTile(i32 col, i32 stripWidth) {
    i32 stride = m_halfWidth * 2;
    if (stripWidth <= 0) {
        return;
    }
    i32 arc = static_cast<i32>((static_cast<double>(m_halfWidth) * 3.14159));
    i32 bpp = m_targetSurface->m_bytesPerPixel;

    i32 colBase;
    u32 arcSpan;
    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy != false)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy == false)) {
        arcSpan = arc - m_halfWidth;
        i32 tail = m_targetWidth - col - stride;
        colBase = stride - static_cast<i32>(static_cast<float>(stride) / arcSpan * tail);
    } else {
        colBase = col;
    }
    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy == false)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy != false)) {
        arcSpan = arc - m_halfWidth;
        colBase = static_cast<i32>(static_cast<float>(stride) / arcSpan * col);
    }
    i32 base;
    i32 warpIndex;
    i32 warpOffset;
    if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy != false)
        || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy == false)) {
        i32 row = 0;
        if (m_targetHeight > 0) {
            base = bpp * col;
            do {
                u8* dstLine = m_targetRowOffsets[row] + base + m_dstBase;
                u8* gsrc = m_warpRowOffsets[row] + base + m_gatherBase;
                u8* ssrc = m_sourceRowOffsets[row] + base + m_straightBase;
                if (m_useLut != false) {
                    u8* lut = m_table->m_data;
                    i32 i = 0;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i] = ssrc[i];
                            i++;
                        } while (i < colBase);
                    }
                    for (i32 t = colBase; t < stride; t++) {
                        m_lineBuf[t] =
                            lut[static_cast<u32>(m_shadeRamp[t])
                                + static_cast<u32>(gsrc[m_warpTable[t]]) * 0x40];
                    }
                } else {
                    COPY_WARP_SEGMENTS(STRAIGHT, ssrc, WARP, gsrc, colBase, stride);
                }
                CopyBytes(dstLine, m_lineBuf, bpp * stride);
                if (m_stripCopy != false) {
                    dstLine -= bpp * stripWidth;
                    u8* s2 = (col - stripWidth) * bpp + m_sourceRowOffsets[row] + m_straightBase;
                    CopyBytes(dstLine, s2, bpp * stripWidth);
                } else {
                    ClearBytes(dstLine + bpp * stride, bpp * stripWidth);
                }
                row++;
            } while (row < m_targetHeight);
        }
    } else if ((m_mode == FADER_SWEEP_FORWARD && m_stripCopy == false)
               || (m_mode == FADER_SWEEP_REVERSE && m_stripCopy != false)) {
        i32 row = 0;
        if (m_targetHeight > 0) {
            base = bpp * col;
            do {
                u8* dstLine = m_targetRowOffsets[row] + base + m_dstBase;
                u8* gsrc = m_warpRowOffsets[row] + base + m_gatherBase;
                u8* ssrc = m_sourceRowOffsets[row] + base + m_straightBase;
                if (m_useLut != false) {
                    u8* lut = m_table->m_data;
                    i32 i = 0;
                    if (colBase > 0) {
                        do {
                            m_lineBuf[i] =
                                lut[static_cast<u32>(m_shadeRamp[i])
                                    + static_cast<u32>(gsrc[m_warpTable[i]]) * 0x40];
                            i++;
                        } while (i < colBase);
                    }
                    for (i32 t = colBase; t < stride; t++) {
                        m_lineBuf[t] = ssrc[t];
                    }
                } else {
                    COPY_WARP_SEGMENTS(WARP, gsrc, STRAIGHT, ssrc, colBase, stride);
                }
                CopyBytes(dstLine, m_lineBuf, bpp * stride);
                if (m_stripCopy != false) {
                    u8* s2 = (col + stride) * bpp + m_sourceRowOffsets[row] + m_straightBase;
                    dstLine += bpp * stride;
                    CopyBytes(dstLine, s2, bpp * stripWidth);
                } else {
                    ClearBytes(dstLine - bpp * stripWidth, bpp * stripWidth);
                }
                row++;
            } while (row < m_targetHeight);
        }
    }
}

#undef COPY_STRAIGHT8
#undef COPY_WARP8
#undef COPY_STRAIGHT16
#undef COPY_WARP16
#undef COPY_STRAIGHT24
#undef COPY_WARP24
#undef COPY_WARP_SEGMENTS
#undef COPY_PIXEL_OP
#undef COPY_PIXEL_OP_INNER

// @early-stop
RVA(0x00182610, 0x2eb)

void CFaderShape::RenderTile(i32 col, i32 stripWidth) {
    if (stripWidth <= 0) {
        return;
    }
    i32 stride = m_halfWidth * 2;
    i32 rowBytes = stride + stripWidth;
    i32 bpp = m_targetSurface->m_bytesPerPixel;

    i32 x0;
    u8* src2base;
    u8* destBase;
    if (m_mode == FADER_SWEEP_FORWARD) {
        src2base = m_lineBuf;
        x0 = stripWidth;
        destBase = m_straightBase + (col - stripWidth) * bpp;
    } else {
        src2base = m_lineBuf + bpp * stride;
        x0 = 0;
        destBase = m_straightBase + (col + stride) * bpp;
    }
    if (m_mode != FADER_SWEEP_FORWARD && m_mode != FADER_SWEEP_REVERSE) {
        return;
    }

    u8* targetColumnBase = m_dstBase + (col - x0) * bpp;
    u8* warpColumnBase = m_gatherBase + (col - x0) * bpp;

    for (i32 j = 0; j < m_targetHeight; j++) {
        u8* targetRow = targetColumnBase + m_targetRowOffsets[j];
        u8* warpRow = warpColumnBase + m_warpRowOffsets[j];

        if (m_useLut) {
            u8* lut = m_table->m_data;
            for (i32 k = 0; k < stride; k++) {
                u8 b = warpRow[m_warpTable[k]];
                m_lineBuf[x0 + k] = lut[(b << 6) + m_shadeRamp[k]];
            }
        } else if (bpp == PIXEL8_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[x0 + k] = warpRow[m_warpTable[k]];
            }
        } else if (bpp == PIXEL16_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[(x0 + k) * 2] = warpRow[m_warpTable[k] * 2];
                m_lineBuf[(x0 + k) * 2 + 1] = warpRow[m_warpTable[k] * 2 + 1];
            }
        } else if (bpp == PIXEL24_BYTES_PER_PIXEL) {
            for (i32 k = 0; k < stride; k++) {
                m_lineBuf[(x0 + k) * 3] = warpRow[m_warpTable[k] * 3];
                m_lineBuf[(x0 + k) * 3 + 1] = warpRow[m_warpTable[k] * 3 + 1];
                m_lineBuf[(x0 + k) * 3 + 2] = warpRow[m_warpTable[k] * 3 + 2];
            }
        }

        if (m_stripCopy) {
            i32 n = bpp * stripWidth;
            u8* s = destBase + m_sourceRowOffsets[j];
            u8* d = src2base;
            CopyBytes(d, s, n);
        } else {
            ClearBytes(src2base, bpp * stripWidth);
        }

        i32 n = bpp * rowBytes;
        CopyBytes(targetRow, m_lineBuf, n);
    }
}

RVA(0x00182900, 0x35)
i32 CFaderShape::GetFrameCount() {
    GZ_ENUM_STORAGE(FaderMode, u32) mode = m_mode;
    if (mode == FADER_SWEEP_FORWARD || mode == FADER_SWEEP_REVERSE) {
        return m_targetWidth - m_halfWidth * 2;
    }
    if (mode == FADER_SPLIT_FROM_CENTER) {
        return (m_targetWidth - m_halfWidth * 4) / 2;
    }
    return 0;
}
