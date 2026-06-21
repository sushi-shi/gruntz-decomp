// DirectSoundMgr.cpp - DirectSoundMgr::GetErrorString, the Dsndmgr module's
// HRESULT->error-string diagnostic reporter (C:\Proj\Dsndmgr\, the DSound
// sibling of CDirectDrawMgr::GetErrorString / DDrawMgr's DIRSURF.CPP).
//
// Maps a DirectSound error code to a "<DSERR_NAME> (<code>) - <description>"
// string and, depending on three reporting-mode globals, beeps, logs it via
// OutputDebugStringA and/or pops a MessageBox. `this` is unused; the work is
// driven entirely by the (file, line, hr) arguments the call sites push.
//
// Same archetype as CDirectDrawMgr::GetErrorString. The retail body differs in
// (1) the DSound case set / strings (almost every case message is "No message"),
// (2) the case dispatch is a cmp/je binary-search tree (sparse DSERR values),
// not a jump table, and (3) the log path emits the formatted line through
// OutputDebugStringA (an FF15 [IAT] indirect call) rather than the DDraw
// version's direct DDrawLogLine call.
//
// The function self-identifies its module via the strings it references (every
// DSERR_* name + "DirectSoundMgr"); names of locals are placeholders, the
// switch case VALUES and string contents are load-bearing.
#include <Dsndmgr/DirectSoundMgr.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA / OutputDebugStringA + BOOL/HWND/LPCSTR/UINT come
// from the real <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's
// uType is MB_ICONEXCLAMATION (0x30); the old hand-rolled macro mislabeled that
// value as the "hand" icon, whose real windows.h value is 0x10.
#include <Win32.h>

// Reporting-mode globals (live in .data). g_logEnabled drives the
// OutputDebugStringA path, g_msgBoxEnabled the MessageBox path; g_beepEnabled
// gates the startup beep, g_thirdEnabled is a third "any output wanted" gate
// checked at entry.
DATA(0x253c54)
extern "C" int g_beepEnabled; // 0x653c54
DATA(0x253c4c)
extern "C" int g_logEnabled; // 0x653c4c
DATA(0x253c50)
extern "C" int g_msgBoxEnabled; // 0x653c50
DATA(0x253c58)
extern "C" int g_thirdEnabled; // 0x653c58

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x2293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// The __FILE__ string every wrapper passes to GetErrorString (the DSNDMGR.CPP
// source path, a single $SG pooled constant referenced by the whole run).
#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

// DSOUND.dll ordinal #1 - the DirectSound device creator. Declared WITHOUT
// dllimport so the call is a direct `e8 rel32` to the incremental-link thunk
// (reloc-masked), matching retail - not an `ff 15 [IAT]` indirect.
extern "C" long __stdcall DirectSoundCreate(void* lpGuid, IDirectSoundZ** ppDS, void* pUnkOuter);

// The buffer-wrapper class's retail vftable (0x5ef6b8). The ctor stamps the vptr
// from this address directly (a reloc-masked DIR32 store) - a transitional
// workaround while the class's virtuals aren't all matched, so the class is kept
// non-polymorphic and the compiler emits no vtable of its own.
DATA(0x1ef6b8)
extern void* const g_DirectSoundMgrVtbl[];

// ---------------------------------------------------------------------------
// DirectSoundMgr ctor (__thiscall). Wraps a held IDirectSoundBuffer: stamps the
// vptr, caches the buffer (m_0c) + owning manager (m_10), zero-inits the cached
// caps/state, then (if a buffer was given) reads its caps into m_40 and caches
// the initial frequency/pan/volume guarded by the matching capability bits, each
// query reported through GetErrorString on failure.
RVA(0x1351d0, 0x109)
DirectSoundMgr::DirectSoundMgr(IDirectSoundBufferZ* buf, DirectSoundMgr* owner) {
    *(void**)this = (void*)g_DirectSoundMgrVtbl;
    m_0c = buf;
    m_10 = owner;
    m_14 = 0;
    m_28 = 0;
    m_30 = 0;
    m_34 = 0;
    m_38 = 0;
    m_3c = 0;
    if (buf == 0) {
        return;
    }

    DSBCAPS caps;
    caps.dwSize = 0x14;
    if (buf->vtbl->GetCaps(buf, &caps) == 0) {
        m_40 = caps.dwFlags;
    } else {
        m_40 = 0;
    }

    if ((m_40 & 0x20) == 0x20) {
        int hr = buf->vtbl->GetFrequency(buf, (unsigned long*)&m_18) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x58, hr);
        }
    }
    m_24 = m_18;

    if ((m_40 & 0x40) == 0x40) {
        int hr = buf->vtbl->GetPan(buf, &m_1c) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x60, hr);
        }
    } else {
        m_1c = 0;
    }

    if ((m_40 & 0x80) == 0x80) {
        int hr = buf->vtbl->GetVolume(buf, &m_20) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x68, hr);
        }
    } else {
        m_20 = 0;
    }
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Restore (__thiscall). Thin IDirectSoundBuffer::Restore wrapper;
// on a nonzero HRESULT routes through GetErrorString and returns 0, else 1. The
// HRESULT is normalized to a 0/1 bool (`!= 0`, the neg/sbb/neg idiom) and that
// bool is what is both tested and forwarded as the reporter's hr.
RVA(0x135310, 0x2a)
int DirectSoundMgr::Restore() {
    int hr = m_0c->vtbl->Restore(m_0c) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x7b, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::StopAndRewind (__thiscall). Gated on the owning manager's
// init flag; Stop() then rewind via SetCurrentPosition(0), each reported on
// failure.
RVA(0x135380, 0x66)
int DirectSoundMgr::StopAndRewind() {
    if (m_10->m_78 == 0) {
        return 0;
    }
    int hr = m_0c->vtbl->Stop(m_0c) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x99, hr);
        return 0;
    }
    hr = m_0c->vtbl->SetCurrentPosition(m_0c, 0) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x9e, hr);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::IsPlaying (__thiscall). GetStatus, report on failure, return
// the "playing" status bit.
RVA(0x1353f0, 0x4b)
int DirectSoundMgr::IsPlaying() {
    if (m_10->m_78 == 0) {
        return 0;
    }
    unsigned long status;
    int hr = m_0c->vtbl->GetStatus(m_0c, &status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xac, hr);
        return 0;
    }
    return (status & 1) == 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::IsLooping (__thiscall). GetStatus, report on failure, return
// the "looping" status bit.
RVA(0x135440, 0x4d)
int DirectSoundMgr::IsLooping() {
    if (m_10->m_78 == 0) {
        return 0;
    }
    unsigned long status;
    int hr = m_0c->vtbl->GetStatus(m_0c, &status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xbb, hr);
        return 0;
    }
    return (status & 2) == 2;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetVolume (__thiscall). Gated on init + the volume capability
// bit (m_40 & 0x80); SetVolume, report on failure.
RVA(0x135560, 0x58)
int DirectSoundMgr::SetVolume(long vol) {
    if (m_10->m_78 == 0) {
        return 0;
    }
    if ((m_40 & 0x80) != 0x80) {
        return 0;
    }
    int hr = m_0c->vtbl->SetVolume(m_0c, vol) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xf6, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetVolume (__thiscall). GetVolume out-param; on failure report
// and return 0, else return the queried volume.
RVA(0x1355f0, 0x42)
long DirectSoundMgr::GetVolume() {
    if (m_10->m_78 == 0) {
        return 0;
    }
    long vol;
    int hr = m_0c->vtbl->GetVolume(m_0c, &vol) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x10e, hr);
        return 0;
    }
    return vol;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetPan (__thiscall). Gated on init + the pan capability bit
// (m_40 & 0x40); SetPan, report on failure.
RVA(0x135740, 0x55)
int DirectSoundMgr::SetPan(long pan) {
    if (m_10->m_78 == 0) {
        return 0;
    }
    if ((m_40 & 0x40) != 0x40) {
        return 0;
    }
    int hr = m_0c->vtbl->SetPan(m_0c, pan) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x141, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetPan (__thiscall). GetPan out-param; on failure report and
// return 0, else return the queried pan.
RVA(0x1357f0, 0x42)
long DirectSoundMgr::GetPan() {
    if (m_10->m_78 == 0) {
        return 0;
    }
    long pan;
    int hr = m_0c->vtbl->GetPan(m_0c, &pan) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x15e, hr);
        return 0;
    }
    return pan;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetFrequency (__thiscall). Gated on init + the frequency
// capability bit (m_40 & 0x20); SetFrequency, report on failure (HRESULT tested
// directly here, not normalized), and cache the value in m_24 on success.
RVA(0x135880, 0x60)
int DirectSoundMgr::SetFrequency(unsigned long freq) {
    if (m_10->m_78 == 0) {
        return 0;
    }
    if ((m_40 & 0x20) != 0x20) {
        return 0;
    }
    long hr = m_0c->vtbl->SetFrequency(m_0c, freq);
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x180, hr);
        return 0;
    }
    m_24 = freq;
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Unlock (__thiscall). Pass-through IDirectSoundBuffer::Unlock;
// report on failure.
RVA(0x1359c0, 0x54)
int DirectSoundMgr::Unlock(void* p1, unsigned long n1, void* p2, unsigned long n2) {
    if (m_10->m_78 == 0) {
        return 0;
    }
    int hr = m_0c->vtbl->Unlock(m_0c, p1, n1, p2, n2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1bb, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetCurrentPosition (__thiscall). Pass-through; report on
// failure.
RVA(0x135a20, 0x4a)
int DirectSoundMgr::GetCurrentPosition(unsigned long* play, unsigned long* write) {
    if (m_10->m_78 == 0) {
        return 0;
    }
    int hr = m_0c->vtbl->GetCurrentPosition(m_0c, play, write) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1c8, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetCurrentPosition (__thiscall). Pass-through; report on
// failure.
RVA(0x135a70, 0x45)
int DirectSoundMgr::SetCurrentPosition(unsigned long pos) {
    if (m_10->m_78 == 0) {
        return 0;
    }
    int hr = m_0c->vtbl->SetCurrentPosition(m_0c, pos) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1d5, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetFormat (__thiscall). Pass-through; report on failure.
RVA(0x135ac0, 0x4f)
int DirectSoundMgr::GetFormat(void* fmt, unsigned long size, unsigned long* written) {
    if (m_10->m_78 == 0) {
        return 0;
    }
    int hr = m_0c->vtbl->GetFormat(m_0c, fmt, size, written) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1e2, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Create (__thiscall). Brings up the DirectSound device:
// DirectSoundCreate into m_14 (its HRESULT normalized into a stored bool), then
// SetCooperativeLevel(hwnd, level). A failed coop call is reported and the device
// released; on success the coop params are cached (m_88/m_8c), m_7c cleared and
// the m_78 "initialized" flag set.
RVA(0x136550, 0x8c)
int DirectSoundMgr::Create(void* hwnd, unsigned long level, unsigned long flags) {
    int created = DirectSoundCreate(0, &m_14, 0) != 0;
    if (created) {
        return 0;
    }
    int hr = m_14->vtbl->SetCooperativeLevel(m_14, hwnd, level) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x3b0, hr);
        m_14->vtbl->Release(m_14);
        return 0;
    }
    m_88 = level;
    m_8c = flags;
    m_7c = 0;
    m_78 = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetCooperativeLevel (__thiscall). Re-issues
// IDirectSound::SetCooperativeLevel on an already-created device; caches the new
// level in m_88. Gated on the m_78 init flag.
RVA(0x1365f0, 0x57)
int DirectSoundMgr::SetCooperativeLevel(void* hwnd, unsigned long level) {
    if (m_78 == 0) {
        return 0;
    }
    int hr = m_14->vtbl->SetCooperativeLevel(m_14, hwnd, level) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x3cf, hr);
        return 0;
    }
    m_88 = level;
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::CreatePrimaryBuffer (__thiscall). Once the device is up
// (m_78) and the cooperative level is exclusive-ish (m_88 == 1), lazily creates
// the primary sound buffer into m_84 via IDirectSound::CreateSoundBuffer with a
// stack DSBUFFERDESC (dwSize 0x14, dwFlags = m_8c | DSBCAPS_PRIMARYBUFFER).
RVA(0x137260, 0x95)
int DirectSoundMgr::CreatePrimaryBuffer() {
    if (m_78 == 0) {
        return 0;
    }
    if (m_88 != 1) {
        return 0;
    }
    if (m_84 == 0) {
        DSBUFFERDESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = 0x14;
        desc.dwFlags = m_8c | 1;
        int hr = m_14->vtbl->CreateSoundBuffer(m_14, &desc, (IDirectSoundZ**)&m_84, 0) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x6ab, hr);
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetErrorString
RVA(0x138150, 0x33b)
void DirectSoundMgr::GetErrorString(char* file, int line, long hr) {
    char szCode[64];  // error-code name
    char szMsg[256];  // description
    char szLine[512]; // formatted output line

    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled) {
        return;
    }

    int code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case (int)0x80004001:
            strcpy(szCode, "DSERR_UNSUPPORTED");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80004005:
            strcpy(szCode, "DSERR_GENERIC");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80040110:
            strcpy(szCode, "DSERR_NOAGGREGATION");
            strcpy(szMsg, "No message");
            break;
        case (int)0x8007000e:
            strcpy(szCode, "DSERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80070057:
            strcpy(szCode, "DSERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case (int)0x8878000a:
            strcpy(szCode, "DSERR_ALLOCATED");
            strcpy(szMsg, "No message");
            break;
        case (int)0x8878001e:
            strcpy(szCode, "DSERR_CONTROLUNAVAIL");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780032:
            strcpy(szCode, "DSERR_INVALIDCALL");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780046:
            strcpy(szCode, "DSERR_PRIOLEVELNEEDED");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780064:
            strcpy(szCode, "DSERR_BADFORMAT");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780078:
            strcpy(szCode, "DSERR_NODRIVER");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780096:
            strcpy(szCode, "DSERR_BUFFERLOST");
            strcpy(szMsg, "No message");
            break;
        case (int)0x887800a0:
            strcpy(szCode, "DSERR_OTHERAPPHASPRIO");
            strcpy(szMsg, "No message");
            break;
        case 0:
            strcpy(szCode, "DS_OK");
            strcpy(szMsg, "No error");
            break;
        default:
            break;
    }

    if (g_logEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
        OutputDebugStringA(szLine);
    }
    if (g_msgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA((HWND)0, szLine, "DirectSoundMgr", MB_ICONEXCLAMATION);
    }
}
