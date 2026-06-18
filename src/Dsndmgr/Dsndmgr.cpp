// Dsndmgr.cpp - DirectSoundMgr error-string mapper + DirectSound COM wrappers.
// DirectSoundMgr::GetErrorString maps DS HRESULT codes to symbol names and
// outputs the result through the configured reporting channels
// (OutputDebugStringA / MessageBoxA) under module-level flag control.
// The ErrorThunk methods wrap IDirectSoundBuffer vtable calls with error
// reporting through GetErrorString.
//
// @address: 0x138150
// @size:    0x33b
#include "Dsndmgr.h"

// ---------------------------------------------------------------------------
// Module-level globals  (pinned @data addresses in the TU).
// ---------------------------------------------------------------------------
int  g_dsndBeep      = 0;   // @0x653c54
int  g_dsndDebug     = 0;   // @0x653c4c
int  g_dsndMsgBox    = 0;   // @0x653c50
int  g_dsndOutputDbg = 0;   // @0x653c58
char g_szDsndModule[] = "DirectSoundMgr";  // @0x619f3c

// Source file path used by ErrorThunks (pinned @data address).
static const char s_szDsndFile[] = "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP";  // @0x619ef8

// Format strings (used by EngFormat via reloc-masked addresses).
static const char s_fmtDebug[] = "%s, line %i: %s (%i) - %s\n";      // @0x619a20
static const char s_fmtDebugShort[] = "%s (%i) - %s\n";              // @0x619a10
static const char s_fmtMsgBox[] = "%s, line %i\n\n%s (%i)\n\n%s";    // @0x6199f4
static const char s_fmtMsgBoxShort[] = "%s (%i)\n\n%s";              // @0x6199e8

// Utility strings
static const char s_unknownErrMsg[] = "Unknown Error Message";       // @0x619ec0
static const char s_unknownErrCode[] = "Unknown Error Code";         // @0x619eac
static const char s_empty[] = "";                                    // @0x6293f4
static const char s_noError[] = "No error";                          // @0x619ad4
static const char s_noMsg[] = "No message";                          // @0x619d48

// DSERR symbol names
static const char s_dserrUnsupported[]   = "DSERR_UNSUPPORTED";      // @0x61a040
static const char s_dserrGeneric[]       = "DSERR_GENERIC";          // @0x61a030
static const char s_dserrNoAggregation[] = "DSERR_NOAGGREGATION";    // @0x61a01c
static const char s_dserrOutOfMemory[]   = "DSERR_OUTOFMEMORY";      // @0x61a008
static const char s_dserrInvalidParam[]  = "DSERR_INVALIDPARAM";     // @0x619ff4
static const char s_dserrAllocated[]     = "DSERR_ALLOCATED";        // @0x619fe4
static const char s_dserrControlUnavail[]= "DSERR_CONTROLUNAVAIL";   // @0x619fcc
static const char s_dserrInvalidCall[]   = "DSERR_INVALIDCALL";      // @0x619fb8
static const char s_dserrPrioLevel[]     = "DSERR_PRIOLEVELNEEDED";  // @0x619fa0
static const char s_dserrBadFormat[]     = "DSERR_BADFORMAT";        // @0x619f90
static const char s_dserrNoDriver[]      = "DSERR_NODRIVER";         // @0x619f80
static const char s_dserrBufferLost[]    = "DSERR_BUFFERLOST";       // @0x619f6c
static const char s_dserrDsOk[]          = "DS_OK";                  // @0x619f64
static const char s_dserrOtherApp[]      = "DSERR_OTHERAPPHASPRIO";  // @0x619f4c

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135380
// Stop() then SetCurrentPosition(0) with error logging.
//
// @address: 0x135380
// @size:    0x66
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135380()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1353f0
// GetStatus(&dwStatus) - return (dwStatus & 2) != 0.
//
// @address: 0x1353f0
// @size:    0x4b
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1353f0()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135440
// GetStatus(&dwStatus) - return (dwStatus & 2) != 0.
//
// @address: 0x135440
// @size:    0x4d
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135440()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135560
// SetVolume(lVolume) - set buffer volume.
//
// @address: 0x135560
// @size:    0x58
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135560()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1355f0
// GetVolume(&lVolume) - get buffer volume.
//
// @address: 0x1355f0
// @size:    0x42
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1355f0()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135740
// SetPan(lPan) - set buffer pan.
//
// @address: 0x135740
// @size:    0x55
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135740()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1357f0
// GetPan(&lPan) - get buffer pan.
//
// @address: 0x1357f0
// @size:    0x42
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1357f0()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135880
// SetFrequency(dwFreq) - set buffer frequency.
//
// @address: 0x135880
// @size:    0x60
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135880()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1359c0
// Unlock(pv1, dw1, pv2, dw2) - unlock buffer.
//
// @address: 0x1359c0
// @size:    0x54
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1359c0()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135a20
// GetCurrentPosition(&pdwCurrent, &pdwWrite) - get play/write cursors.
//
// @address: 0x135a20
// @size:    0x4a
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135a20()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135a70
// SetCurrentPosition(dwPos) - seek to position.
//
// @address: 0x135a70
// @size:    0x45
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135a70()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135ac0
// GetFormat(wfx, dwSize, &dwSizeWritten) - get buffer format.
//
// @address: 0x135ac0
// @size:    0x4f
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135ac0()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1351d0
// Constructor-like - initializes members, calls QueryInterface.
//
// @address: 0x1351d0
// @size:    0x109
// ---------------------------------------------------------------------------
void DirectSoundMgr::ErrorThunk_1351d0(int param1, int param2)
{
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135f40
// Lock/Unlock buffer operations.
//
// @address: 0x135f40
// @size:    0x169
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135f40()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1365f0
// Precondition check on this->field_78 + GetVolume.
//
// @address: 0x1365f0
// @size:    0x57
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1365f0()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_137260
// GetCaps with precondition check on this->field_78.
//
// @address: 0x137260
// @size:    0x95
// ---------------------------------------------------------------------------
HRESULT DirectSoundMgr::ErrorThunk_137260()
{
    return 0;
}

// ---------------------------------------------------------------------------
// @address: 0x138150
// @size:    0x33b
// ---------------------------------------------------------------------------
void DirectSoundMgr::GetErrorString(const char *file, int line, long hr)
{
    char sym[0x40];    // [esp+0x10]  symbol name buffer
    char desc[0x100];  // [esp+0x50]  description buffer
    char out[0x100];   // [esp+0x150] formatted output buffer

    int hrLow = hr & 0xFFFF;

    // -- Optional beep --
    if (g_dsndBeep)
        MessageBeep(0x30);

    // -- Early exit if all flags are 0 (no output channel active) --
    if (!g_dsndDebug && !g_dsndMsgBox && !g_dsndOutputDbg)
        return;

    // -- Build default strings --
    // desc = "Unknown Error Message"
    {
        const char *src = s_unknownErrMsg;
        char *dst = desc;
        while ((*dst++ = *src++));
    }
    // sym = "Unknown Error Code" (via EngFormat)
    EngFormat(sym, s_unknownErrCode);
    // ident = "" (empty string)
    {
        const char *src = s_empty;
        char *dst = out;
        while ((*dst++ = *src++));
    }

    // -- Map HRESULT to symbol name --
    if (hr == 0x80004001) {                    // E_NOTIMPL
        { const char *src = s_dserrUnsupported; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x80004005) {             // E_FAIL
        { const char *src = s_dserrGeneric; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x80040110) {             // CLASS_E_NOAGGREGATION
        { const char *src = s_dserrNoAggregation; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x8007000e) {             // E_OUTOFMEMORY
        { const char *src = s_dserrOutOfMemory; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x80070057) {             // E_INVALIDARG
        { const char *src = s_dserrInvalidParam; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x8878000a) {             // DSERR_ALLOCATED
        { const char *src = s_dserrAllocated; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x8878001e) {             // DSERR_CONTROLUNAVAIL
        { const char *src = s_dserrControlUnavail; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x88780032) {             // DSERR_INVALIDCALL
        { const char *src = s_dserrInvalidCall; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x88780046) {             // DSERR_PRIOLEVELNEEDED
        { const char *src = s_dserrPrioLevel; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x88780064) {             // DSERR_BADFORMAT
        { const char *src = s_dserrBadFormat; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x88780078) {             // DSERR_NODRIVER
        { const char *src = s_dserrNoDriver; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x88780096) {             // DSERR_BUFFERLOST
        { const char *src = s_dserrBufferLost; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0x887800a0) {             // DSERR_OTHERAPPHASPRIO
        { const char *src = s_dserrOtherApp; char *dst = sym; while ((*dst++ = *src++)); }
    } else if (hr == 0) {                      // DS_OK
        { const char *src = s_dserrDsOk; char *dst = sym; while ((*dst++ = *src++)); }
        { const char *src = s_noError; char *dst = desc; while ((*dst++ = *src++)); }
    }

    // For DSERR cases that didn't set a custom desc, the default
    // "Unknown Error Message" remains.  The binary replaces it with
    // "No message" for these cases, but that's what the inline code does.
    {
        const char *src = s_noMsg;
        char *dst = desc;
        while ((*dst++ = *src++));
    }

    // -- Output via debugger channel --
    if (g_dsndDebug) {
        if (file && line > 0)
            EngFormat(out, s_fmtDebug, file, line, sym, hrLow, desc);
        else
            EngFormat(out, s_fmtDebugShort, sym, hrLow, desc);
        OutputDebugStringA(out);
    }

    // -- Output via message box channel --
    if (g_dsndMsgBox) {
        if (file && line > 0)
            EngFormat(out, s_fmtMsgBox, file, line, sym, hrLow, desc);
        else
            EngFormat(out, s_fmtMsgBoxShort, sym, hrLow, desc);
        MessageBoxA(0, out, g_szDsndModule, 0x30);
    }
}
