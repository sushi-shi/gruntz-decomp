#ifndef GRUNTZ_CMOVIEPLAYER_H
#define GRUNTZ_CMOVIEPLAYER_H

#include <rva.h>

#include <Crypto/FecCrypt.h>
#include <Enums.h>
#include <Ints.h>
#include <Wap32/Object.h>

#include <afxtempl.h>
#include <ddraw.h>
#include <stddef.h>

union SmackSource {
    const char* m_path;
    i32 m_handle;
};

struct SmackTag;
class CWnd;
struct DDModeInfo;

// How CMoviePlayer::Configure places the decoded Smacker frame on the screen.
// Recovered from Configure's own arms (src/DDrawMgr/DDPageMgr.cpp): each value
// picks a different m_tilesAcross/m_tilesDown/m_destRect setup. Advance() then
// reads it back: only MOVIE_SINGLE skips the per-dirty-rect BlitRegion loop.
GZ_ENUM_BEGIN(MovieLayout)
// A full grid of copies, screenW/frameW by screenH/frameH, centred - or at
// `origin` when the caller passes flags & 0x10.
    MOVIE_TILE = 0,

    // Exactly one copy, centred (or at `origin`).
    MOVIE_SINGLE = 1,

    // MOVIE_TILE when the frame size divides the screen exactly; otherwise one
    // copy stretched through a whole-screen m_destRect, and m_blitMode is
    // rewritten to MOVIE_SINGLE because that is what the layout has become.
    MOVIE_TILE_OR_STRETCH = 2,

    // One copy blitted into the caller's explicit `rect`.
    MOVIE_DEST_RECT = 3
GZ_ENUM_END(MovieLayout)

struct PLAYLISTINFOSTRUCT {
    char* m_src;
    i32 m_openArg;
    MovieLayout m_blitMode;
    i32 m_useDS;
    POINT* m_origin;
    RECT* m_rect;
    i32 m_flags;
    i32 m_count;
};
SIZE(0x20);

typedef CArray<PLAYLISTINFOSTRUCT*, PLAYLISTINFOSTRUCT*> CMoviePlaylist;

class CMoviePlayer {
public:
    CMoviePlayer() {
        m_window = NULL;
        m_initialized = 0;
        m_smackHandle = NULL;
        m_directDraw2 = NULL;
        m_directDraw = NULL;
        m_primary = NULL;
        m_primaryRaw = NULL;
        m_srcSurf = NULL;
        m_srcSurfRaw = NULL;
        m_borrowedDisplayResources = 0;
        m_palette = NULL;
        m_frameDecoded = false;
        m_blitMode = MOVIE_TILE;
        m_tilesAcross = 0;
        m_tilesDown = 0;
        m_destRect = NULL;
        m_originX = 0;
        m_originY = 0;
        m_forceSingleRow = 0;
        m_smackBufMode = 0;
        m_videoWnd = NULL;
        m_directSound = NULL;
    }

    i32 Init(HWND window, DDModeInfo* mode, u32 coopFlags);
    i32 CheckMode16();
    i32 RemoveAt(i32 idx);
    i32 FreeAll();

    void HandleError();
    void ResetPalette();
    void Snapshot(HWND hWnd);
    i32 BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows);
    i32 Configure(MovieLayout mode, i32 flags, POINT* origin, RECT* rect);
    i32 CheckGrid();
    void UploadPalette();

    i32 InitMode(
        HWND wnd,
        IDirectDraw2* dd2,
        IDirectDrawSurface* primary,
        DDSURFACEDESC desc,
        struct IDirectSound* dsound
    );

    i32 Open(const char* path, i32 entryId, MovieLayout mode, i32 useDS, POINT* origin, RECT* rect);
    ~CMoviePlayer();

    int CreateVideoWindow(DDModeInfo* mode, u32 coopFlags);
    void Teardown();
    i32 OpenLo(const char* src, MovieLayout mode, i32 useDS, POINT* origin, RECT* rect);
    i32 OpenHi(i32 srcHandle, MovieLayout mode, i32 useDS, POINT* origin, RECT* rect);
    i32 Pump(i32 flags, i32 count);

    i32 Advance(IDirectDrawSurface* target, i32 loops);
    i32 CloseSmacker();
    i32 PlayList(i32 loops);
    i32 Frame();

    HWND m_window;
    i32 m_initialized;
    i32 m_streamOpen;
    i32 m_borrowedDisplayResources;

    SmackTag* m_smackHandle;
    IDirectDraw2* m_directDraw2;
    IDirectDraw* m_directDraw;

    IDirectDrawSurface* m_primary;
    IDirectDrawSurface* m_primaryRaw;
    IDirectDrawSurface* m_srcSurf;

    IDirectDrawSurface* m_srcSurfRaw;
    IDirectDrawPalette* m_palette;

    DDSURFACEDESC m_primaryDesc;

    DDSURFACEDESC m_srcDesc;

    PALETTEENTRY m_palEntries[0x100];
    struct IDirectSound* m_directSound;

    i32 m_frameDecoded;
    u32 m_smackBufMode;

    MovieLayout m_blitMode;
    u32 m_screenWidth;
    u32 m_screenHeight;

    i32 m_bpp;
    i32 m_tilesAcross;
    i32 m_tilesDown;
    i32 m_originX;
    i32 m_originY;
    union {
        RECT* m_destRect;
        u8* m_rezBuffer;
    };
    union {
        i32 m_forceSingleRow;
        i32 m_useDS;
    };
    CWnd* m_videoWnd;
    CFecFile m_decodeStore;

    CMoviePlaylist m_playlist;
    i32 m_loopCount;
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

#ifndef MOVIEPLAYER_OOL_DTOR
inline CMoviePlayer::~CMoviePlayer() {
    Teardown();
}
#endif

#endif // GRUNTZ_CMOVIEPLAYER_H
