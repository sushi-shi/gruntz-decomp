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
DATA(0x00253c54)
extern "C" i32 g_beepEnabled; // 0x653c54
DATA(0x00253c4c)
extern "C" i32 g_logEnabled; // 0x653c4c
DATA(0x00253c50)
extern "C" i32 g_msgBoxEnabled; // 0x653c50
DATA(0x00253c58)
extern "C" i32 g_thirdEnabled; // 0x653c58

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x002293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// The __FILE__ string every wrapper passes to GetErrorString (the DSNDMGR.CPP
// source path, a single $SG pooled constant referenced by the whole run).
#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

// DSOUND SDK magics, defined locally (the real values) rather than pulling
// dsound.h - same convention as DirectInputMgr2.cpp. Values are load-bearing
// immediates; the names just spell out what the code masks/compares against.
#define DSBCAPS_CTRLFREQUENCY 0x20 // GetCaps flag: frequency control available
#define DSBCAPS_CTRLPAN 0x40       // GetCaps flag: pan control available
#define DSBCAPS_CTRLVOLUME 0x80    // GetCaps flag: volume control available
#define DSBCAPS_PRIMARYBUFFER 0x01 // CreateSoundBuffer flag: the primary buffer
#define DSBSTATUS_PLAYING 0x01     // GetStatus bit: buffer is playing
#define DSBSTATUS_LOOPING 0x02     // GetStatus bit: buffer is looping (DX5-era value)
#define DSBLOCK_ENTIREBUFFER 0x02  // Lock flag: lock the whole buffer
#define DSSCL_NORMAL 0x01          // cooperative level: normal (shared) access

// DSOUND.dll ordinal #1 - the DirectSound device creator. Declared WITHOUT
// dllimport so the call is a direct `e8 rel32` to the incremental-link thunk
// (reloc-masked), matching retail - not an `ff 15 [IAT]` indirect.
extern "C" i32 __stdcall DirectSoundCreate(void* lpGuid, IDirectSoundZ** ppDS, void* pUnkOuter);

// The buffer-wrapper class's retail vftable (0x5ef6b8). The ctor stamps the vptr
// from this address directly (a reloc-masked DIR32 store) - a transitional
// workaround while the class's virtuals aren't all matched, so the class is kept
// non-polymorphic and the compiler emits no vtable of its own.
DATA(0x001ef6b8)
extern void* const g_DirectSoundMgrVtbl[];

// The clone-instance class's retail vftable (0x5ef6bc), stamped by ~DirectSoundMgr
// at entry - same transitional reloc-masked DIR32 store as the buffer vtable above.
DATA(0x001ef6bc)
extern void* const g_DirectSoundCloneVtbl[];

// The base-subobject vftable (0x5ef6c0) BaseDtor stamps before tail-stamping the
// buffer vtable - the next stage of the base destruction restamp chain.
DATA(0x001ef6c0)
extern void* const g_DirectSoundBaseVtbl[];

// Volume (DSound hundredths-of-dB, [-10000..0]) -> 0..100 linear percent. A free
// __cdecl helper (0x135110): caller pops the one arg (`add esp,4`).
extern "C" i32 ConvertVolumeToPercent(i32 vol);

// The volume->attenuation lookup table SetVolumeByIndex indexes (0x653ab8). Defined
// (and filled at startup) in SoundDevice.cpp as g_volumeTable; only referenced here,
// so no DATA() (the definition carries the label).
extern i32 g_volumeTable[100];

// The pan lookup table SetPanByIndex indexes (0x653c48, immediately after the
// volume table). Read at indices <= 0 from this base: a positive arg reads the
// negated entry, a negative arg the entry direct.
extern i32 g_panTable[];

// The 0x28-byte "playing voice" node minted by CloneAndPlay. Its 6-arg __thiscall
// ctor (0x136fe0) stamps a vtable (0x5ef6d0) + the play params; CloneAndPlay then
// links the node's anchor (m_link@+0x04) into the owner's voice list (owner+0xc).
// The ctor is external (modeled here, defined in 0x136fe0); a placement-new call
// lowers to `mov ecx,voice; call 0x136fe0` reloc-masked.
struct DSoundMgrVoice {
    void* m_vtbl; // +0x00
    struct Link { // +0x04  the intrusive list-anchor
        Link* m_next;
        Link* m_prev;
    } m_link;
    char m_pad0c[0x28 - 0x0c]; // full node is 0x28 bytes (the `new` size operand)
    DSoundMgrVoice(
        i32 key,
        i32 pct,
        i32 mode,
        DirectSoundMgr* owner,
        i32 slot,
        i32 stamp
    ); // 0x136fe0
};

// Owner voice-list helpers (intrusive doubly-linked list, __thiscall on the list
// head). Insert-at-head (0x1390e0) takes the anchor.
struct DSoundMgrList {
    void* m_head;                  // +0x00
    void* m_tail;                  // +0x04
    void InsertHead(void* anchor); // 0x1390e0
};

// The owner's voice-list lookup/free (0x136f60): walks owner+0xc, unlinking +
// deleting every voice whose (node, mask) match - __thiscall on the list head,
// 2 stack args (node, mask). Same helper SoundDevice/SoundStream model as Reap.
struct DSoundVoiceList {
    void* m_head;
    void* m_tail;
    void Reap(void* node, i32 mask); // 0x136f60
};
SIZE_UNKNOWN(DSoundVoiceList); // {head,tail} list-head view

// The intrusive clone-list helper (0x1391e0): unlink a clone node (its m_node44
// anchor) from a clone-list head - __thiscall on the head, 1 stack arg (anchor).
struct DSoundCloneList {
    void* m_head;              // +0x00
    void* m_tail;              // +0x04
    void Unlink(void* anchor); // 0x1391e0
};
SIZE_UNKNOWN(DSoundCloneList); // {head,tail} list-head view

// operator new / operator delete (engine allocator), reloc-masked rel32.
void* operator new(u32);
void operator delete(void*);

// The clone-instance ctor (0x136180, __thiscall ret 0xc => 3 args): chains the
// base DirectSoundMgr ctor (0x1351d0), stamps the clone vtbl (0x5ef6c0), seeds the
// clone back-pointer (m_4c=self), the original-buffer back-ptr (m_reacquireOwner), m_50=1,
// the copied param block (sample/reacquire/rate fields) and ComputeDuration (0x1359a0).
// External, reloc-masked (its /GX EH frame lives there); modeled as a tiny helper
// so the placement-new `mov ecx,alloc; push ...; call` falls out.
struct DSoundCloneCtor {
    inline DSoundCloneCtor(
        IDirectSoundBufferZ* buf,
        DirectSoundMgr* owner,
        DirectSoundMgr* original
    );
    inline void* operator new(u32);

    DSoundCloneCtor* Construct(
        IDirectSoundBufferZ* buf,
        DirectSoundMgr* owner,
        DirectSoundMgr* original
    ); // 0x136180

    char _pad[0x58]; // shell; real object is 0x58 B (the new(0x58) operand)
};
SIZE(DSoundCloneCtor, 0x58); // measured: new(0x58) -> ctor 0x136180

inline DSoundCloneCtor::DSoundCloneCtor(
    IDirectSoundBufferZ* buf,
    DirectSoundMgr* owner,
    DirectSoundMgr* original
) {
    Construct(buf, owner, original);
}

inline void* DSoundCloneCtor::operator new(u32) {
    return ::operator new(0x58);
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::RestampBufferVtbl (__thiscall). Single store of the buffer
// class's vftable into *this; the chained tail of BaseDtor. (mov [ecx],vtbl; ret)
RVA(0x00135300, 0x7)
void DirectSoundMgr::RestampBufferVtbl() {
    *(void**)this = (void*)g_DirectSoundMgrVtbl;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr ctor (__thiscall). Wraps a held IDirectSoundBuffer: stamps the
// vptr, caches the buffer (m_buffer) + owning manager (m_owner), zero-inits the cached
// caps/state, then (if a buffer was given) reads its caps into m_caps and caches
// the initial frequency/pan/volume guarded by the matching capability bits, each
// query reported through GetErrorString on failure.
RVA(0x001351d0, 0x109)
DirectSoundMgr::DirectSoundMgr(IDirectSoundBufferZ* buf, DirectSoundMgr* owner) {
    *(void**)this = (void*)g_DirectSoundMgrVtbl;
    m_buffer = buf;
    m_owner = owner;
    m_device = 0;
    m_durationMs = 0;
    m_reacquireCb = 0;
    m_reacquireCtx = 0;
    m_rateBase = 0;
    m_sampleRate = 0;
    if (buf == 0) {
        return;
    }

    DSBCAPS caps;
    caps.dwSize = sizeof(DSBCAPS);
    if (buf->vtbl->GetCaps(buf, &caps) == 0) {
        m_caps = caps.dwFlags;
    } else {
        m_caps = 0;
    }

    if ((m_caps & DSBCAPS_CTRLFREQUENCY) == DSBCAPS_CTRLFREQUENCY) {
        i32 hr = buf->vtbl->GetFrequency(buf, &m_freq) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x58, hr);
        }
    }
    m_setFreq = m_freq;

    if ((m_caps & DSBCAPS_CTRLPAN) == DSBCAPS_CTRLPAN) {
        i32 hr = buf->vtbl->GetPan(buf, &m_pan) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x60, hr);
        }
    } else {
        m_pan = 0;
    }

    if ((m_caps & DSBCAPS_CTRLVOLUME) == DSBCAPS_CTRLVOLUME) {
        i32 hr = buf->vtbl->GetVolume(buf, &m_volume) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x68, hr);
        }
    } else {
        m_volume = 0;
    }
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Restore (__thiscall). Thin IDirectSoundBuffer::Restore wrapper;
// on a nonzero HRESULT routes through GetErrorString and returns 0, else 1. The
// HRESULT is normalized to a 0/1 bool (`!= 0`, the neg/sbb/neg idiom) and that
// bool is what is both tested and forwarded as the reporter's hr.
RVA(0x00135310, 0x2a)
i32 DirectSoundMgr::Restore() {
    i32 hr = m_buffer->vtbl->Restore(m_buffer) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x7b, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ReacquireBuffer (__thiscall). Gated on the owning manager's init
// flag. If an instance-specific reacquire callback is installed (m_reacquireCb, a __cdecl
// fn-ptr taking (m_reacquireCtx, this)), invoke it and return 1 on success; otherwise tail
// into the owner's ReacquireViaCallback (0x1365e0). m_reacquireCb/m_reacquireCtx are the per-buffer
// callback + context (zero-inited in the ctor).
RVA(0x00135340, 0x37)
i32 DirectSoundMgr::ReacquireBuffer() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32(__cdecl * cb)(i32, DirectSoundMgr*) = (i32(__cdecl*)(i32, DirectSoundMgr*))m_reacquireCb;
    if (cb != 0) {
        if (cb(m_reacquireCtx, this) != 0) {
            return 1;
        }
    }
    return m_owner->ReacquireViaCallback();
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::StopAndRewind (__thiscall). Gated on the owning manager's
// init flag; Stop() then rewind via SetCurrentPosition(0), each reported on
// failure.
RVA(0x00135380, 0x66)
i32 DirectSoundMgr::StopAndRewind() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->Stop(m_buffer) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x99, hr);
        return 0;
    }
    hr = m_buffer->vtbl->SetCurrentPosition(m_buffer, 0) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x9e, hr);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::IsPlaying (__thiscall). GetStatus, report on failure, return
// the "playing" status bit.
RVA(0x001353f0, 0x4b)
i32 DirectSoundMgr::IsPlaying() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    u32 status;
    i32 hr = m_buffer->vtbl->GetStatus(m_buffer, &status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xac, hr);
        return 0;
    }
    return (status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::IsLooping (__thiscall). GetStatus, report on failure, return
// the "looping" status bit.
RVA(0x00135440, 0x4d)
i32 DirectSoundMgr::IsLooping() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    u32 status;
    i32 hr = m_buffer->vtbl->GetStatus(m_buffer, &status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xbb, hr);
        return 0;
    }
    return (status & DSBSTATUS_LOOPING) == DSBSTATUS_LOOPING;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetField3 (__thiscall, 1 arg). Gated on init. Sets/clears bit 0
// of the +0x14 play-flags word (the IDirectSoundBuffer::Play looping flag). +0x14
// overlaps m_device in the manager shape; the buffer shape uses it as m_playFlags.
RVA(0x00135510, 0x25)
void DirectSoundMgr::SetField3(i32 on) {
    if (m_owner->m_initialized == 0) {
        return;
    }
    if (on) {
        m_playFlags |= 1;
    } else {
        m_playFlags &= ~1;
    }
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetVolume (__thiscall). Gated on init + the volume capability
// bit (m_caps & 0x80); SetVolume, report on failure.
RVA(0x00135560, 0x58)
i32 DirectSoundMgr::SetVolume(i32 vol) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLVOLUME) != DSBCAPS_CTRLVOLUME) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->SetVolume(m_buffer, vol) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xf6, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetVolumeByIndex (__thiscall, 1 arg). Gated on init. Maps a
// 0..99 index through the volume table and forwards to SetVolume (tail value).
RVA(0x001355c0, 0x23)
i32 DirectSoundMgr::SetVolumeByIndex(i32 idx) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return SetVolume(g_volumeTable[idx]);
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetVolume (__thiscall). GetVolume out-param; on failure report
// and return 0, else return the queried volume.
RVA(0x001355f0, 0x42)
i32 DirectSoundMgr::GetVolume() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 vol;
    i32 hr = m_buffer->vtbl->GetVolume(m_buffer, &vol) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x10e, hr);
        return 0;
    }
    return vol;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetVolumePercent (__thiscall). Gated on init; reads the raw
// DSound volume via GetVolume and maps it to a 0..100 linear percent through the
// __cdecl helper ConvertVolumeToPercent.
RVA(0x00135640, 0x1c)
i32 DirectSoundMgr::GetVolumePercent() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return ConvertVolumeToPercent(GetVolume());
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::CloneAndPlay (__thiscall, ret 0xc => 3 args = key, mode, slot).
// Gated on init. First reaps any matching finished voices from the owner's voice
// list (owner+0xc). When mode==0 it just re-applies the volume via SetVolumeByIndex;
// otherwise new's a 0x28-byte DSoundMgrVoice for the requested play and links its
// anchor into the owner's voice list (new/ctor in a /GX EH frame, the voice ctor
// being the destructible local). Returns 1 on a successful dispatch, 0 if the
// device is down or the voice allocation/ctor failed.
RVA(0x00135660, 0xe0)
i32 DirectSoundMgr::CloneAndPlay(i32 key, i32 mode, i32 slot) {
    DirectSoundMgr* owner = m_owner;
    if (owner->m_initialized == 0) {
        return 0;
    }
    ((DSoundVoiceList*)&owner->m_buffer)->Reap(this, 1);

    if (mode == 0) {
        SetVolumeByIndex(key);
        return 1;
    }

    DSoundMgrVoice* voice = new DSoundMgrVoice(key, GetVolumePercent(), mode, this, slot, -1);
    if (voice == 0) {
        return 0;
    }
    ((DSoundMgrList*)&m_owner->m_buffer)->InsertHead(&voice->m_link);
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetPan (__thiscall). Gated on init + the pan capability bit
// (m_caps & 0x40); SetPan, report on failure.
RVA(0x00135740, 0x55)
i32 DirectSoundMgr::SetPan(i32 pan) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLPAN) != DSBCAPS_CTRLPAN) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->SetPan(m_buffer, pan) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x141, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetPanByIndex (__thiscall, 1 arg). Gated on init. Maps a signed
// index through the pan table (read from its base toward lower addresses): a
// non-negative idx forwards the negated entry, a negative idx the entry direct.
RVA(0x001357a0, 0x42)
i32 DirectSoundMgr::SetPanByIndex(i32 idx) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if (idx >= 0) {
        return SetPan(-g_panTable[-idx]);
    }
    return SetPan(g_panTable[idx]);
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetPan (__thiscall). GetPan out-param; on failure report and
// return 0, else return the queried pan.
RVA(0x001357f0, 0x42)
i32 DirectSoundMgr::GetPan() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 pan;
    i32 hr = m_buffer->vtbl->GetPan(m_buffer, &pan) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x15e, hr);
        return 0;
    }
    return pan;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetFrequency (__thiscall). Gated on init + the frequency
// capability bit (m_caps & 0x20); SetFrequency, report on failure (HRESULT tested
// directly here, not normalized), and cache the value in m_setFreq on success.
RVA(0x00135880, 0x60)
i32 DirectSoundMgr::SetFrequency(u32 freq) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLFREQUENCY) != DSBCAPS_CTRLFREQUENCY) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->SetFrequency(m_buffer, freq);
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x180, hr);
        return 0;
    }
    m_setFreq = freq;
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetField2 (__thiscall, 1 arg). Gated on init. Adjusts playback
// frequency by `pct` percent of the cached base (clamped to [101, 99999]) via
// SetFrequency, records the matching m_sampleRate rate (pct% of m_rateBase), then recomputes the
// duration. The SetFrequency result is what is returned. Signed mul/div by 100.
RVA(0x00135920, 0x80)
i32 DirectSoundMgr::SetField2(i32 pct) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 v = pct * (i32)m_freq / 100 + (i32)m_freq;
    if ((u32)v >= 0x186a0) {
        v = 0x1869f;
    }
    if ((u32)v <= 0x64) {
        v = 0x65;
    }
    i32 r = SetFrequency(v);
    m_sampleRate = pct * m_rateBase / 100 + m_rateBase;
    ComputeDuration();
    return r;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ComputeDuration (__thiscall, no args). m_durationMs = m_sampleCount*1000/m_sampleRate
// (unsigned), the playback duration in ms from the sample count and the (freshly
// set) sample rate. The *1000 is the strength-reduced lea*5/*5/*5/shl3 chain.
RVA(0x001359a0, 0x18)
void DirectSoundMgr::ComputeDuration() {
    m_durationMs = m_sampleCount * 1000 / m_sampleRate;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Unlock (__thiscall). Pass-through IDirectSoundBuffer::Unlock;
// report on failure.
RVA(0x001359c0, 0x54)
i32 DirectSoundMgr::Unlock(void* p1, u32 n1, void* p2, u32 n2) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->Unlock(m_buffer, p1, n1, p2, n2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1bb, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetCurrentPosition (__thiscall). Pass-through; report on
// failure.
RVA(0x00135a20, 0x4a)
i32 DirectSoundMgr::GetCurrentPosition(u32* play, u32* write) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->GetCurrentPosition(m_buffer, play, write) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1c8, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetCurrentPosition (__thiscall). Pass-through; report on
// failure.
RVA(0x00135a70, 0x45)
i32 DirectSoundMgr::SetCurrentPosition(u32 pos) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->SetCurrentPosition(m_buffer, pos) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1d5, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetFormat (__thiscall). Pass-through; report on failure.
RVA(0x00135ac0, 0x4f)
i32 DirectSoundMgr::GetFormat(void* fmt, u32 size, u32* written) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->GetFormat(m_buffer, fmt, size, written) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1e2, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::~DirectSoundMgr (__thiscall). Stamps the clone-class vftable,
// then drains its clone list (m_cloneHead): for each clone the back-pointer
// (node->m_inst) is handed to RemoveClone, which Releases its buffer, unlinks it,
// and scalar-deletes it - re-reading the head each pass since RemoveClone shrinks
// the list. Once empty, BaseDtor runs the base-subobject destruction. A /GX EH
// frame wraps the loop (the `mov [esp+0x10],1/-1` stores are the unwind levels).
// @early-stop
// EH-dtor wall (docs/patterns/eh-dtor-needs-base-subobject.md): the clone-loop +
// vptr stamp + BaseDtor call are byte-exact, but the /GX EH frame retail emits for
// the non-trivial base subobject is unreachable while the class is modeled non-
// polymorphically (manual vptr stamp, no real base). Modeling the base hierarchy
// would re-shape the ctor + emit a ??_7/??_G and risk the 20 exact siblings; defer
// to the final sweep when the whole class is modeled. 54.45%.
RVA(0x00135bb0, 0x63)
DirectSoundMgr::~DirectSoundMgr() {
    *(void**)this = (void*)g_DirectSoundCloneVtbl;
    while (m_cloneHead != 0) {
        RemoveClone(m_cloneHead->m_inst);
    }
    BaseDtor();
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Clone (__thiscall, ret 0x4 => 1 arg). Gated on owner init.
// Constructs a 0x58-byte clone instance (clone ctor 0x136180 chaining the base
// ctor with this->m_buffer / this->m_owner and recording the original=this), then
// asks the owner's IDirectSound device to DuplicateSoundBuffer this->m_buffer into
// the clone's m_buffer; on failure reports via GetErrorString(0x217) and returns 0.
// On success links the clone's anchor (m_node44) into this->m_cloneHead list,
// stamps the play key (clone->m_50) and returns the clone.
RVA(0x00135c20, 0xf6)
DirectSoundMgr* DirectSoundMgr::Clone(i32 a) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DSoundCloneCtor* clone = new DSoundCloneCtor(m_buffer, m_owner, (DirectSoundMgr*)this);
    if (clone == 0) {
        return 0;
    }
    DirectSoundMgr* c = (DirectSoundMgr*)clone;
    IDirectSoundZ* dev = m_owner->m_device;
    i32 hr = dev->vtbl->DuplicateSoundBuffer(dev, m_buffer, &c->m_buffer) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x217, hr);
        return 0;
    }
    ((DSoundMgrList*)&m_cloneHead)->InsertHead(&c->m_node44);
    c->m_playKey = a;
    return c;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::RemoveClone (__thiscall, ret 4 => 1 arg). Gated on init. For a
// real clone (clone != this), Releases its held buffer and clears m_buffer; then
// unlinks the clone's anchor (m_node44) from this->m_cloneHead list; finally, when the
// clone is a distinct non-null object, scalar-deletes it through its own vtable.
RVA(0x00135d20, 0x47)
void DirectSoundMgr::RemoveClone(DirectSoundMgr* clone) {
    if (m_owner->m_initialized == 0) {
        return;
    }
    if (clone != this) {
        IDirectSoundBufferZ* buf = clone->m_buffer;
        buf->vtbl->Release(buf);
        clone->m_buffer = 0;
    }
    ((DSoundCloneList*)&m_cloneHead)->Unlink(&clone->m_node44);
    if (clone != this && clone != 0) {
        ((DSoundCloneBase*)clone)->ScalarDtor(1);
    }
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::LockConvert (__thiscall). Writes a source buffer (src) into
// the held sound buffer, handling the circular wraparound via Lock's two output
// regions (p1/n1 then p2/n2). The third arg is a "convert 16->8" flag, not a
// size: when zero, each region is a plain byte memcpy of src; when nonzero, each
// source 16-bit sample is downconverted to one 8-bit byte ((s + 0x8000) >> 8 -
// signed->unsigned offset, take the high byte). Region 2's source continues at
// src + n1. Lock failure (line 701) and Unlock failure (line 737) route through
// GetErrorString; gated on the owning manager's init flag.
RVA(0x00135f40, 0x169)
i32 DirectSoundMgr::LockConvert(void* src, u32 lockBytes, u32 convert) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }

    void* p1;
    void* p2;
    u32 n1;
    u32 n2;
    i32 hr =
        m_buffer->vtbl->Lock(m_buffer, 0, lockBytes, &p1, &n1, &p2, &n2, DSBLOCK_ENTIREBUFFER) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x2bd, hr);
        return 0;
    }

    if (convert == 0) {
        // Plain byte copy of each region.
        if (n1 > 0) {
            memcpy(p1, src, n1);
        }
        if (n2 > 0) {
            memcpy(p2, (char*)src + n1, n2);
        }
    } else {
        // 16-bit signed -> 8-bit unsigned downconversion, per region.
        if (n1 > 0) {
            char* d = (char*)p1;
            i16* s = (i16*)src;
            char* end = (char*)p1 + n1;
            while (d < end) {
                *d = (char)((u32)(*s + 0x8000) >> 8);
                ++s;
                ++d;
            }
        }
        if (n2 > 0) {
            char* d = (char*)p2;
            i16* s = (i16*)((char*)src + n1);
            char* end = (char*)p2 + n2;
            while (d < end) {
                *d = (char)((u32)(*s + 0x8000) >> 8);
                ++s;
                ++d;
            }
        }
    }

    hr = m_buffer->vtbl->Unlock(m_buffer, p1, n1, p2, n2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x2e1, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::StopAllClones (__thiscall, no args). Gated on init. Walks the
// clone list (m_cloneHead), calling StopAndRewind on each clone (node->m_inst) so every
// playing voice halts and rewinds; the list itself is left intact.
RVA(0x00136150, 0x22)
void DirectSoundMgr::StopAllClones() {
    if (m_owner->m_initialized == 0) {
        return;
    }
    for (CloneNode* node = m_cloneHead; node != 0; node = node->m_next) {
        node->m_inst->StopAndRewind();
    }
}

// ---------------------------------------------------------------------------
// DSoundBaseSub (0x136230 ctor) / DSoundCloneInst (0x135b10 ctor): a two-level
// DirectSoundMgr-derived clone hierarchy. DSoundBaseSub chains the base
// DirectSoundMgr ctor, stamps the base-subobject vftable (0x5ef6c0) and self-links;
// DSoundCloneInst chains DSoundBaseSub, stamps the clone vftable (0x5ef6bc), and
// links its own node into its (empty) clone list. Both stamps are manual reloc-
// masked DIR32 stores (the classes stay non-polymorphic). The DSoundBaseSub ctor
// body is DEFINED AFTER DSoundCloneInst so cl only sees its declaration at the
// DSoundCloneInst call site => an out-of-line `call 0x136230` (not an inline of the
// small base ctor). DSoundCloneInst's potentially-throwing InsertHead after base
// construction forces the /GX EH frame + unwind state.
class DSoundBaseSub : public DirectSoundMgr {
public:
    DSoundBaseSub(IDirectSoundBufferZ* buf, DirectSoundMgr* owner);
};
SIZE_UNKNOWN(DSoundBaseSub); // DirectSoundMgr-derived; retail clone alloc 0x58 (< C++ sizeof)

class DSoundCloneInst : public DSoundBaseSub {
public:
    DSoundCloneInst(IDirectSoundBufferZ* buf, DirectSoundMgr* owner);
};
SIZE_UNKNOWN(DSoundCloneInst); // DirectSoundMgr-derived; retail clone alloc 0x58 (< C++ sizeof)

// @early-stop
// EH-state-count wall: code bytes are byte-identical (verified base-vs-target
// llvm-objdump -dr) except the /GX unwind state machine. Retail advances TWO
// states (mov [esp+0x10] dword 0 after the base ctor, then mov byte 1 before the
// throwing InsertHead) and its scope-table push is 0xb; cl collapses to ONE state
// (single dword 0, no byte-1 bump) since the DSoundCloneInst level shares
// ~DSoundBaseSub's dtor, so its scope-table push is 0 and the lea edx schedules
// earlier. Logic complete; residual is the EH state assignment only.
RVA(0x00135b10, 0x6b)
DSoundCloneInst::DSoundCloneInst(IDirectSoundBufferZ* buf, DirectSoundMgr* owner)
    : DSoundBaseSub(buf, owner) {
    DSoundMgrList* list = (DSoundMgrList*)&m_cloneHead;
    list->m_head = 0;
    list->m_tail = 0;
    *(void**)this = (void*)g_DirectSoundCloneVtbl;
    list->InsertHead(&m_node44);
    m_playKey = 1;
}

RVA(0x00136230, 0x2d)
DSoundBaseSub::DSoundBaseSub(IDirectSoundBufferZ* buf, DirectSoundMgr* owner)
    : DirectSoundMgr(buf, owner) {
    *(void**)this = (void*)g_DirectSoundBaseVtbl;
    m_node44.m_inst = (DirectSoundMgr*)this;
    m_reacquireOwner = (DirectSoundMgr*)this;
    m_playKey = 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::BaseDtor (__thiscall). The base-subobject teardown: stamp the
// base vftable (0x5ef6c0), then tail into RestampBufferVtbl (0x135300) to stamp the
// buffer vftable (0x5ef6b8) - the two-stage restamp chain ~DirectSoundMgr runs last.
RVA(0x00136260, 0xb)
void DirectSoundMgr::BaseDtor() {
    *(void**)this = (void*)g_DirectSoundBaseVtbl;
    RestampBufferVtbl();
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Play (__thiscall, no args). Gated on init. Plays the buffer with
// the +0x14 looping flag; on DSERR_BUFFERLOST asks m_reacquireOwner to reacquire and retries
// once. A retry failure or a non-buffer-lost HRESULT is reported via GetErrorString;
// a failed reacquire returns 0 silently. Same shape as Lock (0x136370).
RVA(0x00136270, 0x8b)
i32 DirectSoundMgr::Play() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->Play(m_buffer, 0, 0, m_playFlags) != 0;
    if (hr != 0) {
        if (hr == (i32)0x88780096) {
            if (m_reacquireOwner->ReacquireBuffer() == 0) {
                return 0;
            }
            i32 hr2 = m_buffer->vtbl->Play(m_buffer, 0, 0, m_playFlags) != 0;
            if (hr2 != 0) {
                GetErrorString(DSNDMGR_FILE, 0x34c, hr2);
                return 0;
            }
        } else {
            GetErrorString(DSNDMGR_FILE, 0x356, hr);
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ApplyAndPlay (__thiscall, ret 0x10 => 4 args). Gated on owner
// init. Apply the four play params - SetVolumeByIndex, SetPanByIndex, a second and
// a third field setter - then start playback (Play, 0x136270, with its own
// DSERR_BUFFERLOST reacquire-retry). The third setter's result is not folded into
// the success flag; every other failure clears it. Returns the accumulated 0/1.
RVA(0x00136300, 0x6f)
i32 DirectSoundMgr::ApplyAndPlay(i32 vol, i32 pan, i32 freq, i32 d) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 ok = 1;
    if (SetVolumeByIndex(vol) == 0) {
        ok = 0;
    }
    if (SetPanByIndex(pan) == 0) {
        ok = 0;
    }
    if (SetField2(freq) == 0) {
        ok = 0;
    }
    SetField3(d);
    if (Play() == 0) {
        ok = 0;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Lock (__thiscall, 7 args). Pass-through IDirectSoundBuffer::Lock;
// on DSERR_BUFFERLOST (0x88780096) it asks m_reacquireOwner to reacquire the buffer and, if that
// succeeds, retries the Lock once. A retry failure or a non-buffer-lost HRESULT is
// reported via GetErrorString; a failed reacquire returns 0 silently.
// @early-stop
// tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md): the Lock
// COM dispatch, neg/sbb/neg HRESULT bool, reacquire + retry are byte-exact, but the
// two GetErrorString(0x37c/0x386) report+return-0 blocks tail-merge into one shared
// `push str;call;add esp,0xc` where retail duplicates them inline. Optimizer layout
// choice, not steerable from source (tried both branch orderings). ~67.7%.
RVA(0x00136370, 0xcc)
i32 DirectSoundMgr::Lock(u32 off, u32 bytes, void** p1, u32* n1, void** p2, u32* n2, u32 flags) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->vtbl->Lock(m_buffer, off, bytes, p1, n1, p2, n2, flags) != 0;
    if (!hr) {
        return 1;
    }
    if (hr == (i32)0x88780096) {
        if (m_reacquireOwner->ReacquireBuffer() == 0) {
            return 0;
        }
        hr = m_buffer->vtbl->Lock(m_buffer, off, bytes, p1, n1, p2, n2, flags) != 0;
        if (!hr) {
            return 1;
        }
        GetErrorString(DSNDMGR_FILE, 0x37c, hr);
        return 0;
    }
    GetErrorString(DSNDMGR_FILE, 0x386, hr);
    return 0;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Create (__thiscall). Brings up the DirectSound device:
// DirectSoundCreate into m_device (its HRESULT normalized into a stored bool), then
// SetCooperativeLevel(hwnd, level). A failed coop call is reported and the device
// released; on success the coop params are cached (m_coopLevel/m_bufferFlags), m_7c cleared and
// the m_initialized "initialized" flag set.
RVA(0x00136550, 0x8c)
i32 DirectSoundMgr::Create(void* hwnd, u32 level, u32 flags) {
    i32 created = DirectSoundCreate(0, &m_device, 0) != 0;
    if (created) {
        return 0;
    }
    i32 hr = m_device->vtbl->SetCooperativeLevel(m_device, hwnd, level) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x3b0, hr);
        m_device->vtbl->Release(m_device);
        return 0;
    }
    m_coopLevel = level;
    m_bufferFlags = flags;
    m_7c = 0;
    m_initialized = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ReacquireViaCallback (__thiscall). Tail-dispatch through the
// installed reacquire callback fn-ptr (m_reacquireMethod, a __thiscall on the manager); returns
// its result, or 0 when no callback is installed. (mov eax,[ecx+0x80]; jmp eax)
RVA(0x001365e0, 0xf)
i32 DirectSoundMgr::ReacquireViaCallback() {
    if (m_reacquireMethod != 0) {
        i32 (DirectSoundMgr::*cb)() = *(i32(DirectSoundMgr::**)()) & m_reacquireMethod;
        return (this->*cb)();
    }
    return 0;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetCooperativeLevel (__thiscall). Re-issues
// IDirectSound::SetCooperativeLevel on an already-created device; caches the new
// level in m_coopLevel. Gated on the m_initialized init flag.
RVA(0x001365f0, 0x57)
i32 DirectSoundMgr::SetCooperativeLevel(void* hwnd, u32 level) {
    if (m_initialized == 0) {
        return 0;
    }
    i32 hr = m_device->vtbl->SetCooperativeLevel(m_device, hwnd, level) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x3cf, hr);
        return 0;
    }
    m_coopLevel = level;
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::CreatePrimaryBuffer (__thiscall). Once the device is up
// (m_initialized) and the cooperative level is DSSCL_NORMAL (m_coopLevel == 1), lazily creates
// the primary sound buffer into m_primaryBuffer via IDirectSound::CreateSoundBuffer with a
// stack DSBUFFERDESC (dwSize = sizeof, dwFlags = m_bufferFlags | DSBCAPS_PRIMARYBUFFER).
RVA(0x00137260, 0x95)
i32 DirectSoundMgr::CreatePrimaryBuffer() {
    if (m_initialized == 0) {
        return 0;
    }
    if (m_coopLevel != DSSCL_NORMAL) {
        return 0;
    }
    if (m_primaryBuffer == 0) {
        DSBUFFERDESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(DSBUFFERDESC);
        desc.dwFlags = m_bufferFlags | DSBCAPS_PRIMARYBUFFER;
        i32 hr = m_device->vtbl->CreateSoundBuffer(m_device, &desc, &m_primaryBuffer, 0) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x6ab, hr);
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetErrorString
RVA(0x00138150, 0x33b)
void DirectSoundMgr::GetErrorString(char* file, i32 line, i32 hr) {
    char szCode[64];  // error-code name
    char szMsg[256];  // description
    char szLine[512]; // formatted output line

    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled) {
        return;
    }

    i32 code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case (i32)0x80004001:
            strcpy(szCode, "DSERR_UNSUPPORTED");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x80004005:
            strcpy(szCode, "DSERR_GENERIC");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x80040110:
            strcpy(szCode, "DSERR_NOAGGREGATION");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8007000e:
            strcpy(szCode, "DSERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x80070057:
            strcpy(szCode, "DSERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8878000a:
            strcpy(szCode, "DSERR_ALLOCATED");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8878001e:
            strcpy(szCode, "DSERR_CONTROLUNAVAIL");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88780032:
            strcpy(szCode, "DSERR_INVALIDCALL");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88780046:
            strcpy(szCode, "DSERR_PRIOLEVELNEEDED");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88780064:
            strcpy(szCode, "DSERR_BADFORMAT");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88780078:
            strcpy(szCode, "DSERR_NODRIVER");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88780096:
            strcpy(szCode, "DSERR_BUFFERLOST");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887800a0:
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

// -------------------------------------------------------------------------
// Engine-label backlog stubs (relocated from src/Stub/ - own this class here).
// -------------------------------------------------------------------------
// @confidence: low
// @source: winapi:timeGetTime
// @stub
RVA(0x00136e20, 0xa8)
i32 DirectSoundMgr::winapi_136e20_timeGetTime(i32) {
    return 0;
}

// @confidence: low
// @source: winapi:timeGetTime
// @stub
RVA(0x00137ac0, 0xa2)
i32 DirectSoundMgr::winapi_137ac0_timeGetTime(i32) {
    return 0;
}
