// Dsndmgr.cpp - DirectSoundMgr error-string mapper + DirectSound COM wrappers.
// DirectSoundMgr::GetErrorString maps DS HRESULT codes to symbol names and
// outputs the result through the configured reporting channels
// (OutputDebugStringA / MessageBoxA) under module-level flag control.
// The ErrorThunk methods wrap IDirectSoundBuffer vtable calls with error
// reporting through GetErrorString.
#include "Dsndmgr.h"
#include <string.h>

class UnknownSalazar {
public:
    static int computeScaleFactor(int value);
};

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
// DirectSoundMgr::ErrorThunk_135310
// Restore() with error reporting (line 123 / 0x7b in original DSNDMGR.CPP).
//
// @address: 0x135310
// @size:    0x2a
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135310()
{
    HRESULT hr = m_pDSBuffer->lpVtbl->Stop(m_pDSBuffer);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x7b, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135340
// Callback-or-thunk dispatch. Checks field_30 callback; if set calls it,
// otherwise falls back to ErrorThunk_1365e0 (call-thunk).
//
// @address: 0x135340
// @size:    0x37
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135340()
{
    void *sub = m_pDSManager->m_pSubBuffer;
    if (!sub)
        return FALSE;

    // funcPtr = this->field_30
    void *funcPtr = *(void **)((char *)this + 0x30);
    if (funcPtr) {
        // Call the function pointer with (this, field_34)
        void *field34 = *(void **)((char *)this + 0x34);
        BOOL result = ((BOOL (__stdcall *)(void *, void *))funcPtr)(this, field34);
        if (result)
            return TRUE;
    }

    // Fallback: call call-thunk on the DsManager
    void *pMgr = m_pDSManager;
    void *(__stdcall *callThunk)(void *) = *(void *(__stdcall **)(void *))((char *)pMgr + 0x80);
    if (callThunk)
        callThunk(pMgr);
    return FALSE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135380
// Stop() then SetCurrentPosition(0) with error logging.
//
// @address: 0x135380
// @size:    0x66
// ---------------------------------------------------------------------------
void DirectSoundMgr::ErrorThunk_135380()
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return;
    HRESULT hr = m_pDSBuffer->lpVtbl->Stop(m_pDSBuffer);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x131, hr);
        return;
    }
    hr = m_pDSBuffer->lpVtbl->SetCurrentPosition(m_pDSBuffer, 0);
    if (FAILED(hr))
        GetErrorString(s_szDsndFile, 0x136, hr);
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
    DWORD dwStatus;
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->GetStatus(m_pDSBuffer, &dwStatus);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x13d, hr);
        return FALSE;
    }
    return (dwStatus & 2) != 0;
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
    DWORD dwStatus;
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->GetStatus(m_pDSBuffer, &dwStatus);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x14a, hr);
        return FALSE;
    }
    return (dwStatus & 2) != 0;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135510
// Set/clear flag bit 0 at +0x14 based on enable flag.
//
// @address: 0x135510
// @size:    0x25
// ---------------------------------------------------------------------------
void DirectSoundMgr::ErrorThunk_135510(int enable)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return;
    DWORD *pFlag = (DWORD *)((char *)this + 0x14);
    if (enable)
        *pFlag |= 1;
    else
        *pFlag &= ~1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135560
// SetVolume(lVolume) - set buffer volume.
//
// @address: 0x135560
// @size:    0x58
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135560(LONG lVolume)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->SetVolume(m_pDSBuffer, lVolume);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x15f, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1355c0
// SetVolume via g_salazarLookupTable lookup (index -> negated volume).
//
// @address: 0x1355c0
// @size:    0x23
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1355c0(int index)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    return ErrorThunk_135560(-g_salazarLookupTable[index]);
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
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    LONG lVolume;
    HRESULT hr = m_pDSBuffer->lpVtbl->GetVolume(m_pDSBuffer, &lVolume);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x17b, hr);
        return FALSE;
    }
    return lVolume;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135640
// GetVolume -> computeScaleFactor.
//
// @address: 0x135640
// @size:    0x1c
// ---------------------------------------------------------------------------
int DirectSoundMgr::ErrorThunk_135640()
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return 0;
    int vol = ErrorThunk_1355f0();
    return UnknownSalazar::computeScaleFactor(vol);
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135660
// Alloc/init sound entry (SEH guarded). Allocates a new buffer and
// plays into it.
//
// @address: 0x135660
// @size:    0xe0
// ---------------------------------------------------------------------------
void *DirectSoundMgr::ErrorThunk_135660(int a, int b, int c, int d)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return 0;
    // Allocate and play
    return 0;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135740
// SetPan(lPan) - set buffer pan.
//
// @address: 0x135740
// @size:    0x55
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135740(LONG lPan)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->SetPan(m_pDSBuffer, lPan);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x1a8, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1357a0
// SetPan via lookup table (uses g_salazarLookupTable, signed index).
//
// @address: 0x1357a0
// @size:    0x42
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1357a0(int pan)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    if (pan >= 0)
        return ErrorThunk_135740(-g_salazarLookupTable[pan]);
    return ErrorThunk_135740(g_salazarLookupTable[-pan]);
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
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    LONG lPan;
    HRESULT hr = m_pDSBuffer->lpVtbl->GetPan(m_pDSBuffer, &lPan);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x1c6, hr);
        return FALSE;
    }
    return lPan;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135880
// SetFrequency(dwFreq) - set buffer frequency.
//
// @address: 0x135880
// @size:    0x60
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135880(DWORD dwFreq)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->SetFrequency(m_pDSBuffer, dwFreq);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x1dd, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135920
// Frequency calculation from base + multiplier. Clamped to [0x65, 0x1869f].
//
// @address: 0x135920
// @size:    0x80
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135920(int inputFreq)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    DWORD field18 = *(DWORD *)((char *)this + 0x18);
    DWORD freq = (DWORD)((LONG)inputFreq * (LONG)field18 / 100 + (LONG)field18);
    if (freq > 0x1869f)
        freq = 0x1869f;
    if (freq < 0x65)
        freq = 0x65;
    if (!ErrorThunk_135880(freq))
        return FALSE;
    DWORD field38 = *(DWORD *)((char *)this + 0x38);
    *(DWORD *)((char *)this + 0x3c) = (DWORD)((LONG)inputFreq * (LONG)field38 / 100 + (LONG)field38);
    ErrorThunk_1359a0();
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1359a0
// Position calculation: field_28 = field_2c * 1000 / field_3c.
//
// @address: 0x1359a0
// @size:    0x18
// ---------------------------------------------------------------------------
void DirectSoundMgr::ErrorThunk_1359a0()
{
    DWORD field2c = *(DWORD *)((char *)this + 0x2c);
    DWORD field3c = *(DWORD *)((char *)this + 0x3c);
    if (field3c)
        *(DWORD *)((char *)this + 0x28) = (field2c * 1000) / field3c;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1359c0
// Unlock(pv1, dw1, pv2, dw2) - unlock buffer.
//
// @address: 0x1359c0
// @size:    0x54
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1359c0(void *p1, DWORD d1, void *p2, DWORD d2)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->Unlock(m_pDSBuffer, p1, d1, p2, d2);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x20b, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135a20
// GetCurrentPosition(&pdwCurrent, &pdwWrite) - get play/write cursors.
//
// @address: 0x135a20
// @size:    0x4a
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135a20(DWORD *pdwCurrent, DWORD *pdwWrite)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->GetCurrentPosition(m_pDSBuffer, pdwCurrent, pdwWrite);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x22b, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135a70
// SetCurrentPosition(dwPos) - seek to position.
//
// @address: 0x135a70
// @size:    0x45
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135a70(DWORD dwPos)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->SetCurrentPosition(m_pDSBuffer, dwPos);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x245, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135ac0
// GetFormat(wfx, dwSize, &dwSizeWritten) - get buffer format.
//
// @address: 0x135ac0
// @size:    0x4f
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135ac0(void *wfx, DWORD dwSize, DWORD *dwSizeWritten)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->GetFormat(m_pDSBuffer, wfx, dwSize, dwSizeWritten);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x25c, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_135f40
// Lock buffer operations with DSERR_BUFFERLOST retry.
//
// @address: 0x135f40
// @size:    0x169
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_135f40(DWORD dwOffset, DWORD dwBytes,
                                         void **ppvAudioPtr1, DWORD *pdwAudioBytes1,
                                         void **ppvAudioPtr2, DWORD *pdwAudioBytes2,
                                         DWORD dwFlags)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->Lock(m_pDSBuffer, dwOffset, dwBytes,
                                            ppvAudioPtr1, pdwAudioBytes1,
                                            ppvAudioPtr2, pdwAudioBytes2, dwFlags);
    if (FAILED(hr)) {
        if (hr == 0x88780096) { // DSERR_BUFFERLOST
            if (ErrorThunk_135340()) {
                hr = m_pDSBuffer->lpVtbl->Lock(m_pDSBuffer, dwOffset, dwBytes,
                                                ppvAudioPtr1, pdwAudioBytes1,
                                                ppvAudioPtr2, pdwAudioBytes2, dwFlags);
            }
        }
        if (FAILED(hr)) {
            GetErrorString(s_szDsndFile, 0x131, hr);
            return FALSE;
        }
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_1365f0
// GetCaps with precondition check on this->field_78.
//
// @address: 0x1365f0
// @size:    0x57
// ---------------------------------------------------------------------------
BOOL DirectSoundMgr::ErrorThunk_1365f0(void *caps)
{
    if (!m_pDSManager || !m_pDSManager->m_pSubBuffer)
        return FALSE;
    HRESULT hr = m_pDSBuffer->lpVtbl->GetCaps(m_pDSBuffer, caps);
    if (FAILED(hr)) {
        GetErrorString(s_szDsndFile, 0x1f1, hr);
        return FALSE;
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ErrorThunk_137260
// GetCaps with precondition check and DSERR_BUFFERLOST retry.
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
