#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <ComOutRef.h>
#include <Crypto/FecCrypt.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDScreen.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/PixelShift.h>
#include <Ints.h>
#include <Io/FileStream.h>
#include <Io/MoviePlayer.h>
#include <Wap32/ScreenGeometry.h>

#include <ddraw.h>
#include <smack.h>
#include <stdio.h>
#include <string.h>

#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64

RVA(0x0017c040, 0x25d)
i32 CMoviePlayer::Init(HWND window, DDModeInfo* mode, u32 coopFlags) {
    if (m_initialized != 0) {
        return 0;
    }
    if (window == NULL) {
        return 0;
    }

    DDModeInfo info;
    if (mode != NULL) {
        info = *mode;
    } else {
        info.width = SCREEN_W_PX;
        info.height = SCREEN_H_PX;
        info.bpp = BPP_PALETTED_8;
    }

    m_borrowedDisplayResources = 0;
    if (DirectDrawCreate(NULL, &m_directDraw, NULL) != 0) {
        return 0;
    }
    ComOutRef<IDirectDraw2> ddOut;
    ddOut.m_asTyped = &m_directDraw2;
    if (m_directDraw->QueryInterface(IID_IDirectDraw2, ddOut.m_asVoid) != 0) {
        return 0;
    }
    if (m_directDraw2->SetCooperativeLevel(static_cast<HWND>(window), coopFlags) != 0) {
        HandleError();
        return 0;
    }
    if (m_directDraw2->SetDisplayMode(info.width, info.height, IDX(info.bpp), 0, 0) != 0) {
        HandleError();
        return 0;
    }

    memset(&m_primaryDesc, 0, sizeof(m_primaryDesc));
    m_primaryDesc.dwSize = sizeof(DDSURFACEDESC);
    m_primaryDesc.dwFlags = DDSD_CAPS;
    m_primaryDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (m_directDraw2->CreateSurface(&m_primaryDesc, &m_primaryRaw, NULL) != 0) {
        HandleError();
        return 0;
    }

    ComOutRef<IDirectDrawSurface> primOut;
    primOut.m_asTyped = &m_primary;
    HRESULT hr = m_primaryRaw->QueryInterface(IID_IDirectDrawSurface3, primOut.m_asVoid);
    if (hr != 0) {
        return 0;
    }

    Snapshot(static_cast<HWND>(window));

    if (mode->bpp == BPP_PALETTED_8) {
        if (m_directDraw2->CreatePalette(
                DDPCAPS_8BIT,
                static_cast<LPPALETTEENTRY>(m_palEntries),
                &m_palette,
                NULL
            )
            != 0) {
            HandleError();
            return 0;
        }
        m_primary->SetPalette(m_palette);
        m_smackBufMode = 0;
    }

    if (mode->bpp == BPP_RGB_24) {
        HandleError();
        return 0;
    }
    if (mode->bpp == BPP_RGB_16) {
        if (SelectSmackBufferFormat16() == 0) {
            HandleError();
            return 0;
        }
    }

    m_screenWidth = info.width;
    m_srcSurf = NULL;
    m_srcSurfRaw = NULL;
    m_screenHeight = info.height;
    m_bpp = info.bpp;
    m_window = window;
    m_streamOpen = 0;
    ShowCursor(0);
    m_initialized = 1;
    FreeAll();
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017c2a0, 0x14e)
int CMoviePlayer::CreateVideoWindow(DDModeInfo* mode, u32 coopFlags) {
    CString cls(AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, NULL));
    if (m_videoWnd != NULL) {
        return 0;
    }
    m_videoWnd = new CWnd;
    if (!m_videoWnd->CreateEx(
            WS_EX_TOPMOST,
            cls,
            "Smacker Video Window",
            WS_POPUP | WS_VISIBLE,
            0,
            0,
            GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN),
            NULL,
            NULL,
            NULL
        )) {
        return 0;
    }
    m_videoWnd->SetFocus();

    return Init(m_videoWnd->GetSafeHwnd(), mode, coopFlags);
}

RVA(0x0017c3f0, 0x120)
i32 CMoviePlayer::InitMode(
    HWND wnd,
    IDirectDraw2* dd2,
    IDirectDrawSurface* primary,
    DDSURFACEDESC desc,
    IDirectSound* dsound
) {
    if (!wnd || !dd2 || !primary) {
        return 0;
    }
    m_directDraw2 = dd2;
    m_borrowedDisplayResources = 1;
    m_primary = primary;
    Snapshot(wnd);
    ColorDepth bpp = static_cast<ColorDepth>(desc.ddpfPixelFormat.dwRGBBitCount);
    if (bpp == BPP_PALETTED_8) {
        if (m_directDraw2->CreatePalette(DDPCAPS_8BIT, m_palEntries, &m_palette, NULL)) {
            HandleError();
            return 0;
        }
        m_primary->SetPalette(m_palette);
        m_smackBufMode = 0;
    }
    if (bpp == BPP_RGB_24) {
        HandleError();
        return 0;
    }
    if (bpp == BPP_RGB_16) {
        if (!SelectSmackBufferFormat16()) {
            HandleError();
            return 0;
        }
    }
    m_screenWidth = desc.dwWidth;
    m_screenHeight = desc.dwHeight;
    m_bpp = bpp;
    m_window = wnd;
    m_streamOpen = 0;
    m_srcSurf = NULL;
    m_srcSurfRaw = NULL;
    m_directSound = dsound;
    ShowCursor(0);
    m_initialized = 1;
    FreeAll();
    return 1;
}

RVA(0x0017c510, 0x5e)
void CMoviePlayer::Teardown() {
    if (!m_initialized) {
        return;
    }
    CloseSmacker();
    FreeAll();
    m_window = NULL;
    m_initialized = 0;
    HandleError();
    if (m_videoWnd) {
        m_videoWnd->DestroyWindow();
        delete m_videoWnd;
        m_videoWnd = NULL;
    }
    ShowCursor(1);
}

RVA(0x0017c570, 0xc0)
i32 CMoviePlayer::OpenLo(
    const char* src,
    MovieLayout mode,
    MovieOpenFlags openFlags,
    POINT* origin,
    RECT* rect
) {
    if (!m_initialized) {
        return 0;
    }

    m_blitMode = mode;
    SmackSoundUseDirectSound(m_directSound);

    u32 smackOpenFlags = 0;
    if (openFlags == MOVIE_OPEN_INTERLACED) {
        m_interlaced = IDX(openFlags);
        smackOpenFlags = SMACKYINTERLACE;
    } else {
        m_interlaced = 0;
    }
    smackOpenFlags |= SMACKTRACKS;
    m_smackHandle = SmackOpen(src, smackOpenFlags, SMACKAUTOEXTRA);
    if (!m_smackHandle) {
        return 0;
    }
    m_streamOpen = 1;
    i32 r = Configure(mode, openFlags, origin, rect);
    if (!r) {
        if (m_srcSurf) {
            m_srcSurf->Release();
            m_srcSurf = NULL;
        }
        if (m_srcSurfRaw) {
            m_srcSurfRaw->Release();
            m_srcSurfRaw = NULL;
        }
        CloseSmacker();
    }
    return r;
}

RVA(0x0017c630, 0xc0)
i32 CMoviePlayer::OpenHi(
    i32 srcHandle,
    MovieLayout mode,
    MovieOpenFlags openFlags,
    POINT* origin,
    RECT* rect
) {
    if (!m_initialized) {
        return 0;
    }

    m_blitMode = mode;
    SmackSoundUseDirectSound(m_directSound);

    u32 smackOpenFlags = 0;
    if (openFlags == MOVIE_OPEN_INTERLACED) {
        smackOpenFlags = SMACKYINTERLACE;
        m_interlaced = IDX(openFlags);
    } else {
        m_interlaced = 0;
    }
    smackOpenFlags |= SMACKFILEHANDLE | SMACKTRACKS;

    SmackSource src;
    src.m_handle = srcHandle;
    m_smackHandle = SmackOpen(src.m_path, smackOpenFlags, SMACKAUTOEXTRA);
    if (!m_smackHandle) {
        return 0;
    }
    m_streamOpen = 1;
    i32 r = Configure(mode, openFlags, origin, rect);
    if (!r) {
        if (m_srcSurf) {
            m_srcSurf->Release();
            m_srcSurf = NULL;
        }
        if (m_srcSurfRaw) {
            m_srcSurfRaw->Release();
            m_srcSurfRaw = NULL;
        }
        CloseSmacker();
    }
    return r;
}

RVA(0x0017c6f0, 0x9c)
i32 CMoviePlayer::Open(
    const char* path,
    i32 entryId,
    MovieLayout mode,
    MovieOpenFlags openFlags,
    POINT* origin,
    RECT* rect
) {
    if (m_initialized == 0) {
        return 0;
    }
    if (!m_decodeStore.Init()) {
        return 0;
    }
    if (!m_decodeStore.ReadArchive(path)) {
        m_decodeStore.Close();
        return 0;
    }
    i32 hi = m_decodeStore.Lookup(static_cast<unsigned int>(entryId));
    if (!hi) {
        m_decodeStore.Close();
        return 0;
    }
    if (!OpenHi(hi, mode, openFlags, origin, rect)) {
        m_decodeStore.Close();
        return 0;
    }
    return 1;
}

RVA(0x0017c790, 0x14a)
MoviePlaybackResult CMoviePlayer::Pump(MoviePumpFlags pumpFlags, i32 count) {
    if (!m_initialized || count < -1 || count == 0) {
        return MOVIE_RESULT_ERROR;
    }
    m_loopCount = 1;
    MSG msg;
    for (;;) {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_SYSKEYDOWN) {
                continue;
            }
            if (msg.message == WM_SYSKEYUP) {
                continue;
            }
            if (msg.message == WM_KEYDOWN) {
                if (HAS(pumpFlags, MOVIE_PUMP_SKIP_ON_KEY)) {
                    return MOVIE_RESULT_KEY_SKIP;
                }
                continue;
            }
            if (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN
                || msg.message == WM_LBUTTONDBLCLK || msg.message == WM_RBUTTONDBLCLK) {
                if (HAS(pumpFlags, MOVIE_PUMP_SKIP_ON_MOUSE)) {
                    return MOVIE_RESULT_MOUSE_SKIP;
                }
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else {
            if (SmackWait(m_smackHandle)) {
                continue;
            }
            if (Frame()) {
                continue;
            }
            if (count != -1 && ++m_loopCount > count) {
                return MOVIE_RESULT_FINISHED;
            }
            SmackSoundOnOff(m_smackHandle, 0);
            SmackGoto(m_smackHandle, 1);
            SmackSoundOnOff(m_smackHandle, 1);
        }
    }
}

RVA(0x0017c8e0, 0xca)
i32 CMoviePlayer::Advance(IDirectDrawSurface* target, i32 loops) {
    if (!target || !m_initialized || loops < -1 || loops == 0) {
        return 0;
    }
    i32 result = 1;
    if (m_loopCount == 0) {
        m_loopCount = result;
    }
    if (SmackWait(m_smackHandle) == 0) {
        IDirectDrawSurface* saved = m_primary;
        m_primary = target;
        result = Frame();
        if (result == 0) {
            if (loops == -1 || ++m_loopCount <= loops) {
                SmackSoundOnOff(m_smackHandle, 0);
                SmackGoto(m_smackHandle, 1);
                SmackSoundOnOff(m_smackHandle, 1);
                result = 1;
            }
        }
        m_primary = saved;
    }
    if (result == 0) {
        m_loopCount = 0;
    }
    return result;
}

RVA(0x0017c9b0, 0x5b)
i32 CMoviePlayer::CloseSmacker() {
    if (!m_streamOpen) {
        return 0;
    }
    m_decodeStore.Close();
    if (!m_smackHandle) {
        return 0;
    }
    SmackClose(m_smackHandle);
    m_smackHandle = NULL;
    if (m_rezBuffer) {
        ::operator delete(m_rezBuffer);
        m_rezBuffer = NULL;
    }
    m_streamOpen = 0;
    return 1;
}

RVA(0x0017ca10, 0x49)
void CMoviePlayer::UploadPalette() {
    u8* src = m_smackHandle->Palette;
    u8* p = &m_palEntries[0].peGreen;
    i32 n = PALETTE_ENTRY_COUNT;
    do {
        p[-1] = *src++;
        p[0] = *src++;
        p[1] = *src++;
        p += 4;
        --n;
    } while (n != 0);
    m_palette->SetEntries(0, 0, PALETTE_ENTRY_COUNT, m_palEntries);
}

RVA(0x0017ca60, 0x35)
void CMoviePlayer::ResetPalette() {
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        m_palEntries[i].peRed = 0;
        m_palEntries[i].peGreen = 0;
        m_palEntries[i].peBlue = 0;
    }
    m_palette->SetEntries(0, 0, PALETTE_ENTRY_COUNT, m_palEntries);
}

RVA(0x0017caa0, 0x13b)
i32 CMoviePlayer::Frame() {
    if (m_smackHandle->NewPalette && m_bpp == BPP_PALETTED_8) {
        UploadPalette();
    }
    i32 hr = m_srcSurf->Lock(NULL, &m_srcDesc, 1, NULL);
    while (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (m_srcSurf->Restore() != 0) {
            goto afterLock;
        }
        hr = m_srcSurf->Lock(NULL, &m_srcDesc, 1, NULL);
    }
    if (hr == 0) {
        SmackToBuffer(
            m_smackHandle,
            0,
            0,
            m_srcDesc.lPitch,
            m_smackHandle->Height,
            m_srcDesc.lpSurface,
            m_smackBufMode
        );
        SmackDoFrame(m_smackHandle);
        m_frameDecoded = true;
        m_srcSurf->Unlock(m_srcDesc.lpSurface);
    }
afterLock:
    if (m_blitMode != MOVIE_SINGLE) {
        while (SmackToBufferRect(m_smackHandle, 0) != 0) {
            BlitRegion(
                m_smackHandle->LastRectx,
                m_smackHandle->LastRecty,
                m_smackHandle->LastRectw,
                m_smackHandle->LastRecth
            );
        }
    } else {
        BlitRegion(0, 0, m_smackHandle->Width, m_smackHandle->Height);
    }
    Smack* s = m_smackHandle;
    if (s->FrameNum == s->Frames - 1) {
        return 0;
    }
    SmackNextFrame(s);
    return 1;
}

RVA(0x0017cbe0, 0x97)
i32 CMoviePlayer::CheckGrid() {
    memset(&m_srcDesc, 0, sizeof(m_srcDesc));
    m_srcDesc.dwSize = sizeof(m_srcDesc);
    m_srcDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    m_srcDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    m_srcDesc.dwHeight = m_smackHandle->Height;
    m_srcDesc.dwWidth = m_smackHandle->Width;
    if (m_directDraw2->CreateSurface(&m_srcDesc, &m_srcSurfRaw, NULL) != 0) {
        return 0;
    }
    ComOutRef<IDirectDrawSurface> srcOut;
    srcOut.m_asTyped = &m_srcSurf;
    if (m_srcSurfRaw->QueryInterface(IID_IDirectDrawSurface3, srcOut.m_asVoid) != 0) {
        return 0;
    }
    if (m_bpp == BPP_PALETTED_8) {
        m_srcSurf->SetPalette(m_palette);
    }
    return 1;
}

RVA(0x0017cc80, 0x109)
void CMoviePlayer::HandleError() {
    if (m_srcSurf) {
        m_srcSurf->Release();
        m_srcSurf = NULL;
    }
    if (m_srcSurfRaw) {
        m_srcSurfRaw->Release();
        m_srcSurfRaw = NULL;
    }
    if (m_bpp == BPP_PALETTED_8) {
        ResetPalette();
    }
    if (m_primary) {
        DDBLTFX fx;
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);
        fx.dwROP = BLACKNESS;
        HRESULT rc = m_primary->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_ROP, &fx);
        if (rc) {
            memset(&fx, 0, sizeof(fx));
            fx.dwSize = sizeof(fx);
            fx.dwFillColor = 0;
            m_primary->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
        }
    }
    if (m_borrowedDisplayResources == 0) {
        if (m_palette) {
            m_palette->Release();
            m_palette = NULL;
        }
        if (m_primary) {
            m_primary->Release();
            m_primary = NULL;
        }
        if (m_primaryRaw) {
            m_primaryRaw->Release();
            m_primaryRaw = NULL;
        }
        if (m_directDraw2) {
            m_directDraw2->RestoreDisplayMode();
            m_directDraw2->Release();
            m_directDraw2 = NULL;
        }
        if (m_directDraw) {
            m_directDraw->Release();
            m_directDraw = NULL;
        }
    }
}

RVA(0x0017cd90, 0x58)
void CMoviePlayer::Snapshot(HWND hWnd) {
    HDC hdc = GetDC(hWnd);
    GetSystemPaletteEntries(hdc, 0, PALETTE_ENTRY_COUNT, m_palEntries);
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        m_palEntries[i].peRed = 0;
        m_palEntries[i].peBlue = 0;
        m_palEntries[i].peGreen = 0;
        m_palEntries[i].peFlags = PC_NOCOLLAPSE;
    }
    ReleaseDC(hWnd, hdc);
}

RVA(0x0017cdf0, 0x1c6)
i32 CMoviePlayer::BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows) {
    RECT dst, src;
    if (m_destRect) {
        dst.left = m_destRect->left;
        dst.top = m_destRect->top;
        dst.right = m_destRect->right;
        dst.bottom = m_destRect->bottom;
    } else {
        dst.left = col * m_tilesAcross + m_originX;
        dst.top = row * m_tilesDown + m_originY;
        dst.right = nCols * m_tilesAcross + dst.left;
        dst.bottom = nRows * m_tilesDown + dst.top;
    }
    src.left = col;
    src.top = row;
    src.right = col + nCols;
    src.bottom = row + nRows;

    for (;;) {
        i32 hr;
        if (m_tilesAcross == 1 && m_tilesDown == 1 && m_destRect == NULL) {
            hr = m_primary->BltFast(dst.left, dst.top, m_srcSurf, &src, DDBLTFAST_WAIT);
            if (hr != static_cast<i32>(DDERR_SURFACELOST)) {
                return hr;
            }
            if (m_primary->IsLost() == static_cast<i32>(DDERR_SURFACELOST)
                && m_primary->Restore() == 0) {
                if (m_bpp == BPP_PALETTED_8) {
                    m_primary->SetPalette(m_palette);
                    UploadPalette();
                }
            } else {
                hr = m_srcSurf->IsLost();
                if (hr != static_cast<i32>(DDERR_SURFACELOST)) {
                    return hr;
                }
                hr = m_srcSurf->Restore();
                if (hr != 0) {
                    return hr;
                }
            }
        } else {
            hr = m_primary->Blt(&dst, m_srcSurf, &src, DDBLT_WAIT, NULL);
            if (hr != static_cast<i32>(DDERR_SURFACELOST)) {
                return hr;
            }
            if (m_primary->IsLost() == static_cast<i32>(DDERR_SURFACELOST)
                && m_primary->Restore() == 0) {
                if (m_bpp == BPP_PALETTED_8) {
                    m_primary->SetPalette(m_palette);
                    UploadPalette();
                }
            } else {
                hr = m_srcSurf->IsLost();
                if (hr != static_cast<i32>(DDERR_SURFACELOST)) {
                    return hr;
                }
                hr = m_srcSurf->Restore();
                if (hr != 0) {
                    return hr;
                }
            }
        }
    }
}

// @early-stop
RVA(0x0017cfc0, 0x2f0)
i32 CMoviePlayer::Configure(MovieLayout mode, MovieOpenFlags openFlags, POINT* origin, RECT* rect) {
    if (origin) {
        if (origin->x > m_screenWidth) {
            return 0;
        }
        if (origin->y > m_screenHeight) {
            return 0;
        }
    }
    if (rect) {
        if (rect->left > rect->right) {
            return 0;
        }
        if (rect->top > rect->bottom) {
            return 0;
        }
        if (static_cast<u32>(rect->right) > m_screenWidth) {
            return 0;
        }
        if (static_cast<u32>(rect->bottom) > m_screenHeight) {
            return 0;
        }
    }
    if (m_smackHandle->Width > m_screenWidth) {
        return 0;
    }
    if (m_smackHandle->Height > m_screenHeight) {
        return 0;
    }
    if (!CheckGrid()) {
        return 0;
    }

    switch (mode) {
        case MOVIE_TILE:
            m_tilesAcross = m_screenWidth / m_smackHandle->Width;
            m_tilesDown = m_screenHeight / m_smackHandle->Height;
            if (HAS(openFlags, MOVIE_OPEN_USE_ORIGIN)) {
                if (!origin) {
                    return 0;
                }
                m_originX = origin->x;
                m_originY = origin->y;
            } else {
                m_originX = (m_screenWidth - m_tilesAcross * m_smackHandle->Width) >> 1;
                m_originY = (m_screenHeight - m_tilesDown * m_smackHandle->Height) >> 1;
            }
            break;
        case MOVIE_SINGLE:
            m_tilesAcross = 1;
            m_tilesDown = 1;
            if (HAS(openFlags, MOVIE_OPEN_USE_ORIGIN)) {
                if (!origin) {
                    return 0;
                }
                m_originX = origin->x;
                m_originY = origin->y;
            } else {
                m_originX = (m_screenWidth - m_smackHandle->Width) >> 1;
                m_originY = (m_screenHeight - m_smackHandle->Height) >> 1;
            }
            break;
        case MOVIE_TILE_OR_STRETCH:
            if (m_screenWidth % m_smackHandle->Width == 0
                && m_screenHeight % m_smackHandle->Height == 0) {
                m_tilesAcross = m_screenWidth / m_smackHandle->Width;
                m_tilesDown = m_screenHeight / m_smackHandle->Height;
                if (HAS(openFlags, MOVIE_OPEN_USE_ORIGIN)) {
                    if (!origin) {
                        return 0;
                    }
                    m_originX = origin->x;
                    m_originY = origin->y;
                } else {
                    m_originX = (m_screenWidth - m_tilesAcross * m_smackHandle->Width) >> 1;
                    m_originY = (m_screenHeight - m_tilesDown * m_smackHandle->Height) >> 1;
                }
            } else {
                m_tilesAcross = 1;
                m_tilesDown = 1;
                m_originX = 0;
                m_originY = 0;
                m_destRect = new RECT;
                m_destRect->top = 0;
                m_destRect->left = 0;
                m_destRect->bottom = m_screenHeight;
                m_destRect->right = m_screenWidth;
                m_blitMode = MOVIE_SINGLE;
            }
            break;
        case MOVIE_DEST_RECT: {
            m_tilesAcross = 1;
            m_tilesDown = 1;
            m_originX = 0;
            m_originY = 0;
            if (!rect) {
                return 0;
            }
            RECT* r = new RECT;
            m_destRect = r;
            r->left = rect->left;
            r->top = rect->top;
            r->right = rect->right;
            r->bottom = rect->bottom;
            break;
        }
        default:
            return 0;
    }

    if (m_forceSingleRow != 0) {
        m_tilesDown = 1;
    }
    m_frameDecoded = false;
    m_loopCount = 0;
    return 1;
}

RVA(0x0017d2b0, 0xe4)
i32 CMoviePlayer::SelectSmackBufferFormat16() {
    i32 r = 0;
    i32 g = 0;
    i32 b = 0;
    i32 i;
    u32 m;
    DDSURFACEDESC desc;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    if (m_directDraw2->GetDisplayMode(&desc) != 0) {
        return 0;
    }

    m = desc.ddpfPixelFormat.dwRBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            r++;
        }
        m >>= 1;
    }
    m = desc.ddpfPixelFormat.dwGBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            g++;
        }
        m >>= 1;
    }
    m = desc.ddpfPixelFormat.dwBBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            b++;
        }
        m >>= 1;
    }

    if (r == RGB555_CHANNEL_BITS && g == RGB555_CHANNEL_BITS && b == RGB555_CHANNEL_BITS) {
        m_smackBufMode = SMACKBUFFER555;
        return 1;
    }
    if (r == RGB555_CHANNEL_BITS && g == RGB565_GREEN_BITS && b == RGB555_CHANNEL_BITS) {
        m_smackBufMode = SMACKBUFFER565;
        return 1;
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017d3a0, 0x259)
i32 CMoviePlayer::AddToPlaylist(
    const char* src,
    i32 openArg,
    MovieLayout mode,
    MovieOpenFlags openFlags,
    POINT* origin,
    RECT* rect,
    MoviePumpFlags pumpFlags,
    i32 count
) {
    if (!m_initialized) {
        return 0;
    }
    if (src == NULL) {
        return 0;
    }

    PLAYLISTINFOSTRUCT* rec = new PLAYLISTINFOSTRUCT;
    rec->m_src = new char[strlen(src) + 1];
    strcpy(rec->m_src, src);
    rec->m_openArg = openArg;
    rec->m_blitMode = mode;
    rec->m_openFlags = openFlags;
    if (origin != NULL) {
        rec->m_origin = new POINT;
        *rec->m_origin = *origin;
    } else {
        rec->m_origin = NULL;
    }
    if (rect != NULL) {
        rec->m_rect = new RECT;
        *rec->m_rect = *rect;
    } else {
        rec->m_rect = NULL;
    }
    rec->m_pumpFlags = pumpFlags;
    rec->m_count = count;
    m_playlist.Add(rec);
    return 1;
}

RVA(0x0017d600, 0xad)
i32 CMoviePlayer::RemoveAt(i32 idx) {
    if (!m_initialized) {
        return 0;
    }
    if (m_playlist.GetSize() < idx) {
        return 0;
    }

    PLAYLISTINFOSTRUCT* rec = m_playlist[idx - 1];
    if (rec->m_src) {
        delete[] rec->m_src;
        rec->m_src = NULL;
    }
    if (rec->m_origin) {
        delete rec->m_origin;
        rec->m_origin = NULL;
    }
    if (rec->m_rect) {
        delete rec->m_rect;
        rec->m_rect = NULL;
    }

    m_playlist.RemoveAt(idx - 1);
    delete rec;
    return 1;
}

RVA(0x0017d6b0, 0x70)
i32 CMoviePlayer::FreeAll() {
    if (!m_initialized) {
        return 0;
    }
    i32 count = m_playlist.GetSize();
    for (i32 i = 0; i < count; i++) {
        if (!RemoveAt(1)) {
            return 0;
        }
    }

    m_playlist.RemoveAll();
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017d720, 0x188)
MoviePlaybackResult CMoviePlayer::PlayList(i32 loops) {
    if (!m_initialized || loops < -1 || loops == 0) {
        return MOVIE_RESULT_ERROR;
    }
    i32 iter = 1;
    do {
        for (i32 i = 0; i < m_playlist.GetSize(); i++) {
            PLAYLISTINFOSTRUCT* clip = m_playlist[i];
            if (clip->m_src == NULL) {
                return MOVIE_RESULT_ERROR;
            }
            if (clip->m_openArg == 0) {
                if (OpenLo(
                        clip->m_src,
                        clip->m_blitMode,
                        clip->m_openFlags,
                        clip->m_origin,
                        clip->m_rect
                    )
                    == 0) {
                    return MOVIE_RESULT_ERROR;
                }
            } else {
                if (Open(
                        clip->m_src,
                        clip->m_openArg,
                        clip->m_blitMode,
                        clip->m_openFlags,
                        clip->m_origin,
                        clip->m_rect
                    )
                    == 0) {
                    return MOVIE_RESULT_ERROR;
                }
            }
            PLAYLISTINFOSTRUCT* c2 = m_playlist[i];
            MoviePlaybackResult result = Pump(c2->m_pumpFlags, c2->m_count);
            if (result != MOVIE_RESULT_FINISHED) {
                CloseSmacker();
                return result;
            }
            if (m_primary != NULL) {
                DDBLTFX fx;
                memset(&fx, 0, sizeof(fx));
                fx.dwSize = sizeof(fx);
                fx.dwROP = BLACKNESS;
                i32 hr = (static_cast<IDirectDrawSurface*>(m_primary))
                             ->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_ROP, &fx);
                if (hr != 0) {
                    memset(&fx, 0, sizeof(fx));
                    fx.dwSize = sizeof(fx);
                    fx.dwFillColor = 0;
                    (static_cast<IDirectDrawSurface*>(m_primary))
                        ->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
                }
            }
            CloseSmacker();
        }
        iter++;
    } while (iter <= loops);
    return MOVIE_RESULT_FINISHED;
}
