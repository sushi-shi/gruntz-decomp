#ifndef GRUNTZ_CMOVIEPLAYER_H
#define GRUNTZ_CMOVIEPLAYER_H

#include <rva.h>

#include <Crypto/FecCrypt.h>
#include <DDrawMgr/ColorDepth.h>
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

GZ_ENUM_BEGIN(MovieLayout)
    MOVIE_TILE = 0,

    MOVIE_SINGLE = 1,

    MOVIE_TILE_OR_STRETCH = 2,

    MOVIE_DEST_RECT = 3
GZ_ENUM_END(MovieLayout)

GZ_ENUM_FLAGS_BEGIN(MovieOpenFlags, i32)
    MOVIE_OPEN_DEFAULT = 0,
    MOVIE_OPEN_INTERLACED = 0x1,
    MOVIE_OPEN_USE_ORIGIN = 0x10
GZ_ENUM_FLAGS_END(MovieOpenFlags, i32)
GZ_ENUM_FLAGS_OPS(MovieOpenFlags)

GZ_ENUM_FLAGS_BEGIN(MoviePumpFlags, i32)
    MOVIE_PUMP_SKIP_ON_KEY = 0x1,
    MOVIE_PUMP_SKIP_ON_MOUSE = 0x100
GZ_ENUM_FLAGS_END(MoviePumpFlags, i32)
GZ_ENUM_FLAGS_OPS(MoviePumpFlags)

GZ_ENUM_BEGIN(MoviePlaybackResult)
    MOVIE_RESULT_ERROR = 0,
    MOVIE_RESULT_KEY_SKIP = 1,
    MOVIE_RESULT_MOUSE_SKIP = 0x100,
    MOVIE_RESULT_FINISHED = 0x11111111
GZ_ENUM_END(MoviePlaybackResult)

struct PLAYLISTINFOSTRUCT {
    char* m_src;
    i32 m_openArg;
    MovieLayout m_blitMode;
    MovieOpenFlags m_openFlags;
    POINT* m_origin;
    RECT* m_rect;
    MoviePumpFlags m_pumpFlags;
    i32 m_count;
};

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
    i32 SelectSmackBufferFormat16();
    i32 AddToPlaylist(
        const char* src,
        i32 openArg,
        MovieLayout mode,
        MovieOpenFlags openFlags,
        POINT* origin,
        RECT* rect,
        MoviePumpFlags pumpFlags,
        i32 count
    );
    i32 RemoveAt(i32 idx);
    i32 FreeAll();

    void HandleError();
    void ResetPalette();
    void Snapshot(HWND hWnd);
    i32 BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows);
    i32 Configure(MovieLayout mode, MovieOpenFlags openFlags, POINT* origin, RECT* rect);
    i32 CheckGrid();
    void UploadPalette();

    i32 InitMode(
        HWND wnd,
        IDirectDraw2* dd2,
        IDirectDrawSurface* primary,
        DDSURFACEDESC desc,
        struct IDirectSound* dsound
    );

    i32 Open(
        const char* path,
        i32 entryId,
        MovieLayout mode,
        MovieOpenFlags openFlags,
        POINT* origin,
        RECT* rect
    );
    ~CMoviePlayer();

    int CreateVideoWindow(DDModeInfo* mode, u32 coopFlags);
    void Teardown();
    i32
    OpenLo(const char* src, MovieLayout mode, MovieOpenFlags openFlags, POINT* origin, RECT* rect);
    i32
    OpenHi(i32 srcHandle, MovieLayout mode, MovieOpenFlags openFlags, POINT* origin, RECT* rect);
    MoviePlaybackResult Pump(MoviePumpFlags pumpFlags, i32 count);

    i32 Advance(IDirectDrawSurface* target, i32 loops);
    i32 CloseSmacker();
    MoviePlaybackResult PlayList(i32 loops);
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

    ColorDepth m_bpp;
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
        i32 m_interlaced;
    };
    CWnd* m_videoWnd;
    CFecFile m_decodeStore;

    CMoviePlaylist m_playlist;
    i32 m_loopCount;
};

inline CMoviePlayer::~CMoviePlayer() {
    Teardown();
}

#endif // GRUNTZ_CMOVIEPLAYER_H
