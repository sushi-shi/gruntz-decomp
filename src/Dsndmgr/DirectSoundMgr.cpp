// DirectSoundMgr.cpp - the Dsndmgr module's per-buffer DirectSound wrapper run
// (C:\Proj\Dsndmgr\DSNDMGR.CPP). DirectSoundMgr is the buffer-wrapper base
// (vtable 0x5ef6b8); two concrete leaves derive it here - DSoundBaseSub (0x5ef6c0,
// the 0x58-byte clone Clone() news) and DSoundCloneInst (0x5ef6bc, the 0x60-byte
// leaf that owns a clone list). The device-level bring-up methods (Create /
// SetCooperativeLevel / CreatePrimaryBuffer / ReacquireViaCallback) run on the
// owning SoundDevice and are defined here too (they fall in this RVA range).
//
// GetErrorString maps a DirectSound error code to a "<DSERR_NAME> (<code>) -
// <description>" string and, per three reporting-mode globals, beeps / logs it via
// OutputDebugStringA / pops a MessageBox. It self-identifies its module via the
// strings it references (every DSERR_* name + "DirectSoundMgr"); the switch case
// VALUES and string contents are load-bearing.
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DSoundVoice.h> // the 0x28-byte voice node CloneAndPlay news
#include <Dsndmgr/SoundDevice.h> // SoundDevice: the owning device (m_owner) + its methods
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA / OutputDebugStringA + BOOL/HWND/LPCSTR/UINT come
// from the real <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's
// uType is MB_ICONEXCLAMATION (0x30).
#include <Win32.h>
#include <Globals.h>

// Reporting-mode globals (live in .data). g_logEnabled drives the
// OutputDebugStringA path, g_msgBoxEnabled the MessageBox path; g_beepEnabled
// gates the startup beep, g_thirdEnabled is a third "any output wanted" gate.
DATA(0x00253c54)
extern "C" i32 g_beepEnabled; // 0x653c54
DATA(0x00253c4c)
extern "C" i32 g_logEnabled; // 0x653c4c
DATA(0x00253c50)
extern "C" i32 g_msgBoxEnabled; // 0x653c50
DATA(0x00253c58)
extern "C" i32 g_thirdEnabled; // 0x653c58

// DSERR_BUFFERLOST (MAKE_DSHRESULT(150)) - the DirectSound error a lost hardware
// buffer returns; every buffer op that hits it drops into the reacquire-retry path.
static const i32 DSERR_BUFFERLOST = (i32)0x88780096;

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

// Volume (DSound hundredths-of-dB, [-10000..0]) -> 0..100 linear percent. A free
// __cdecl helper (0x135110): caller pops the one arg (`add esp,4`).
extern "C" i32 ConvertVolumeToPercent(i32 vol);

// The volume->attenuation lookup table SetVolumeByIndex indexes (0x653ab8), filled
// at startup in SoundDevice.cpp; only referenced here, so no DATA().
extern i32 g_volumeTable[100];
// The pan lookup table SetPanByIndex indexes (0x653c48, in Globals.h) sits right
// after the volume table: a positive arg reads the negated entry (walking toward
// lower addresses), a negative arg the entry direct.

// ---------------------------------------------------------------------------
// The DirectSoundMgr-derived clone hierarchy (real 3-level polymorphic). All the
// wrapper fields + methods live on the DirectSoundMgr base (DirectSoundMgr.h);
// the two derived leaves add only their own vtable/dtor (and, for the concrete
// leaf, the clone list).

// The clone list head {head,tail}. InsertHead/Unlink are the shared engine list
// helpers (0x1390e0 / 0x1391e0) typed on CloneNode so the call sites stay cast-free
// (a CloneNode's {next,prev} lead word is what the helpers thread).
struct CloneList {
    CloneNode* m_head;                // +0x00
    CloneNode* m_tail;                // +0x04
    void InsertHead(CloneNode* node); // 0x1390e0
    void Unlink(CloneNode* node);     // 0x1391e0
};
SIZE(CloneList, 0x8); // {head, tail}

// DSoundBaseSub - the clone/duplicate buffer wrapper Clone() news (vtable 0x5ef6c0,
// size 0x58 - adds no fields over the base). Chains the DirectSoundMgr base ctor
// and cl-stamps the base-subobject vftable; its dtor (0x136260) resets the vptr and
// chains ~DirectSoundMgr.
class DSoundBaseSub : public DirectSoundMgr {
public:
    DSoundBaseSub(IDirectSoundBufferZ* buf, SoundDevice* owner); // 0x136230
    // Clone/duplicate ctor (0x136180): as the 2-arg ctor, plus records the source
    // manager (m_reacquireOwner=original), copies its sample/reacquire/rate block,
    // and recomputes the duration - the object Clone() news for a duplicated buffer.
    DSoundBaseSub(
        IDirectSoundBufferZ* buf,
        SoundDevice* owner,
        DirectSoundMgr* original
    );                        // 0x136180
    virtual ~DSoundBaseSub(); // 0x136260  base-subobject dtor (vptr reset + chain)
};
SIZE(DSoundBaseSub, 0x58);       // clone alloc: Clone() news 0x58 (RezAlloc(0x58))
VTBL(DSoundBaseSub, 0x001ef6c0); // cl-emitted ??_7DSoundBaseSub@@6B@

// DSoundCloneInst - the concrete per-buffer leaf that owns a clone list (vtable
// 0x5ef6bc, size 0x60). Its dtor (0x135bb0) drains the clone list.
class DSoundCloneInst : public DSoundBaseSub {
public:
    DSoundCloneInst(IDirectSoundBufferZ* buf, SoundDevice* owner); // 0x135b10
    virtual ~DSoundCloneInst();                                    // 0x135bb0  clone-drain dtor

    DirectSoundMgr* Clone(i32 a);            // 0x135c20  new a clone, dup the buffer, link it
    void RemoveClone(DirectSoundMgr* clone); // 0x135d20  release + unlink + delete one clone
    void StopAllClones();                    // 0x136150  StopAndRewind each clone

    CloneList m_cloneList; // +0x58  clone/child list {head@+0x58, tail@+0x5c}
};
SIZE(DSoundCloneInst, 0x60);       // buffer leaf: CreateBuffer RezAlloc(0x60)
VTBL(DSoundCloneInst, 0x001ef6bc); // cl-emitted ??_7DSoundCloneInst@@6B@

// ---------------------------------------------------------------------------
// DirectSoundMgr::~DirectSoundMgr (__thiscall) - the base-subobject dtor. Body is
// empty; cl emits just the implicit vptr reset to ??_7DirectSoundMgr@@6B@ (0x5ef6b8)
// (mov [ecx],vtbl; ret).
RVA(0x00135300, 0x7)
DirectSoundMgr::~DirectSoundMgr() {}

// ---------------------------------------------------------------------------
// DirectSoundMgr ctor (__thiscall). Wraps a held IDirectSoundBuffer: stamps the
// vptr, caches the buffer (m_buffer) + owning device (m_owner), zero-inits the cached
// caps/state, then (if a buffer was given) reads its caps into m_caps and caches
// the initial frequency/pan/volume guarded by the matching capability bits, each
// query reported through GetErrorString on failure.
RVA(0x001351d0, 0x109)
DirectSoundMgr::DirectSoundMgr(IDirectSoundBufferZ* buf, SoundDevice* owner) {
    // cl auto-stamps ??_7DirectSoundMgr@@6B@ (0x5ef6b8) here.
    m_buffer = buf;
    m_owner = owner;
    m_playFlags = 0;
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
    if (buf->GetCaps(&caps) == 0) {
        m_caps = caps.dwFlags;
    } else {
        m_caps = 0;
    }

    if ((m_caps & DSBCAPS_CTRLFREQUENCY) == DSBCAPS_CTRLFREQUENCY) {
        i32 hr = buf->GetFrequency(&m_freq) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x58, hr);
        }
    }
    m_setFreq = m_freq;

    if ((m_caps & DSBCAPS_CTRLPAN) == DSBCAPS_CTRLPAN) {
        i32 hr = buf->GetPan(&m_pan) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x60, hr);
        }
    } else {
        m_pan = 0;
    }

    if ((m_caps & DSBCAPS_CTRLVOLUME) == DSBCAPS_CTRLVOLUME) {
        i32 hr = buf->GetVolume(&m_volume) != 0;
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
    i32 hr = m_buffer->Restore() != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x7b, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::ReacquireBuffer (__thiscall). Gated on the owning device's init
// flag. If an instance-specific reacquire callback is installed (m_reacquireCb, a
// __cdecl fn-ptr taking (this, m_reacquireCtx)), invoke it and return 1 on success;
// otherwise tail into the owner's ReacquireViaCallback (0x1365e0).
RVA(0x00135340, 0x37)
i32 DirectSoundMgr::ReacquireBuffer() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if (m_reacquireCb != 0) {
        if (m_reacquireCb(this, m_reacquireCtx) != 0) {
            return 1;
        }
    }
    return m_owner->ReacquireViaCallback();
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::StopAndRewind (__thiscall). Gated on the owning device's init
// flag; Stop() then rewind via SetCurrentPosition(0), each reported on failure.
RVA(0x00135380, 0x66)
i32 DirectSoundMgr::StopAndRewind() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Stop() != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x99, hr);
        return 0;
    }
    hr = m_buffer->SetCurrentPosition(0) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x9e, hr);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::IsPlaying (__thiscall). GetStatus, report on failure, return
// the "playing" status bit.
// @early-stop
// byte-AND-width wall (99%): body byte-exact except `and eax,1` (ours) vs retail
// `and al,1` - retail narrows the mask to the low byte while keeping the dword status
// load (`mov eax,[esp]`); a (u8) cast flips the AND but wrongly narrows the load too
// (mov al). Neither pure form is retail's dword-load+byte-and mix; a /O2 partial-
// register codegen pick, not source-steerable.
RVA(0x001353f0, 0x4b)
i32 DirectSoundMgr::IsPlaying() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    u32 status;
    i32 hr = m_buffer->GetStatus(&status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xac, hr);
        return 0;
    }
    return (status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::IsLooping (__thiscall). GetStatus, report on failure, return
// the "looping" status bit.
// @early-stop
// byte-AND-width wall (99%): same as IsPlaying - `and eax,2` (ours) vs retail
// `and al,2` while both keep the dword status load; /O2 partial-register codegen
// pick, not source-steerable.
RVA(0x00135440, 0x4d)
i32 DirectSoundMgr::IsLooping() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    u32 status;
    i32 hr = m_buffer->GetStatus(&status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xbb, hr);
        return 0;
    }
    return (status & DSBSTATUS_LOOPING) == DSBSTATUS_LOOPING;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetField3 (__thiscall, 1 arg). Gated on init. Sets/clears bit 0
// of the +0x14 play-flags word (the IDirectSoundBuffer::Play looping flag).
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
    i32 hr = m_buffer->SetVolume(vol) != 0;
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
    i32 hr = m_buffer->GetVolume(&vol) != 0;
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
// DirectSoundMgr::CloneAndPlay (__thiscall, 3 args = key, mode, slot). Gated on
// init. First reaps any matching finished voices from the owner's voice list;
// when mode==0 it just re-applies the volume via SetVolumeByIndex; otherwise new's
// a 0x28-byte DSoundVoice for the requested play and links its anchor into the
// owner's voice list (new/ctor in a /GX EH frame). Returns 1 on success, 0 if the
// device is down or the voice allocation/ctor failed.
RVA(0x00135660, 0xe0)
i32 DirectSoundMgr::CloneAndPlay(i32 key, i32 mode, i32 slot) {
    SoundDevice* owner = m_owner;
    if (owner->m_initialized == 0) {
        return 0;
    }
    owner->m_voiceList.RemoveMatching(this, 1);

    if (mode == 0) {
        SetVolumeByIndex(key);
        return 1;
    }

    DSoundVoice* voice = new DSoundVoice(key, GetVolumePercent(), mode, this, slot, -1);
    if (voice == 0) {
        return 0;
    }
    m_owner->m_voiceList.InsertHead(&voice->m_link);
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
    i32 hr = m_buffer->SetPan(pan) != 0;
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
    i32 hr = m_buffer->GetPan(&pan) != 0;
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
    i32 hr = m_buffer->SetFrequency(freq);
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
// SetFrequency, records the matching m_sampleRate rate (pct% of m_rateBase), then
// recomputes the duration. The SetFrequency result is returned. Signed mul/div 100.
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
// DirectSoundMgr::ComputeDuration (__thiscall, no args). m_durationMs =
// m_sampleCount*1000/m_sampleRate (unsigned), the playback duration in ms from the
// sample count and the (freshly set) sample rate. The *1000 is the strength-reduced
// lea*5/*5/*5/shl3 chain.
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
    i32 hr = m_buffer->Unlock(p1, n1, p2, n2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1bb, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetCurrentPosition (__thiscall). Pass-through; report on failure.
RVA(0x00135a20, 0x4a)
i32 DirectSoundMgr::GetCurrentPosition(u32* play, u32* write) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->GetCurrentPosition(play, write) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1c8, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::SetCurrentPosition (__thiscall). Pass-through; report on failure.
RVA(0x00135a70, 0x45)
i32 DirectSoundMgr::SetCurrentPosition(u32 pos) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->SetCurrentPosition(pos) != 0;
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
    i32 hr = m_buffer->GetFormat(fmt, size, written) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1e2, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DSoundCloneInst::~DSoundCloneInst (__thiscall). cl auto-stamps the clone vptr
// (??_7DSoundCloneInst@@6B@ = 0x5ef6bc) at entry, then the body drains the clone
// list (m_cloneList): for each clone the back-pointer (node->m_inst) is handed to
// RemoveClone, which Releases its buffer, unlinks it, and scalar-deletes it -
// re-reading the head each pass since RemoveClone shrinks the list. Then cl chains
// ~DSoundBaseSub (0x136260) automatically. A /GX EH frame wraps the loop.
// @early-stop
// EH-dtor / vptr-reset schedule: the clone-loop is byte-exact; realized as a real
// derived dtor so the clone/base/buffer vptr resets + base dtor chain are cl-emitted.
RVA(0x00135bb0, 0x63)
DSoundCloneInst::~DSoundCloneInst() {
    while (m_cloneList.m_head != 0) {
        RemoveClone(m_cloneList.m_head->m_inst);
    }
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Clone -> DSoundCloneInst::Clone (__thiscall, 1 arg). Gated on
// owner init. Constructs a 0x58-byte DSoundBaseSub clone (chaining the base ctor
// with this->m_buffer / this->m_owner, recording original=this), then asks the
// owner's IDirectSound device to DuplicateSoundBuffer this->m_buffer into the
// clone's m_buffer; on failure reports via GetErrorString(0x217) and returns 0. On
// success links the clone's node into this->m_cloneList, stamps the play key and
// returns the clone.
RVA(0x00135c20, 0xf6)
DirectSoundMgr* DSoundCloneInst::Clone(i32 a) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DSoundBaseSub* clone = new DSoundBaseSub(m_buffer, m_owner, this);
    if (clone == 0) {
        return 0;
    }
    IDirectSoundZ* dev = m_owner->m_device;
    i32 hr = dev->DuplicateSoundBuffer(m_buffer, &clone->m_buffer) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x217, hr);
        return 0;
    }
    m_cloneList.InsertHead(&clone->m_cloneNode);
    clone->m_playKey = a;
    return clone;
}

// ---------------------------------------------------------------------------
// DSoundCloneInst::RemoveClone (__thiscall, 1 arg). Gated on init. For a real
// clone (clone != this), Releases its held buffer and clears m_buffer; then
// unlinks the clone's node from this->m_cloneList; finally, when the clone is a
// distinct non-null object, scalar-deletes it through its own vtable.
RVA(0x00135d20, 0x47)
void DSoundCloneInst::RemoveClone(DirectSoundMgr* clone) {
    if (m_owner->m_initialized == 0) {
        return;
    }
    if (clone != this) {
        IDirectSoundBufferZ* buf = clone->m_buffer;
        buf->Release();
        clone->m_buffer = 0;
    }
    m_cloneList.Unlink(&clone->m_cloneNode);
    if (clone != this) {
        delete clone;
    }
}

// ---------------------------------------------------------------------------
// DirectSoundMgr::LockConvert (__thiscall). Writes a source buffer (src) into the
// held sound buffer, handling the circular wraparound via Lock's two output
// regions (p1/n1 then p2/n2). The third arg is a "convert 16->8" flag, not a size:
// when zero, each region is a plain byte memcpy of src; when nonzero, each source
// 16-bit sample is downconverted to one 8-bit byte ((s + 0x8000) >> 8 -
// signed->unsigned offset, take the high byte). Region 2's source continues at
// src + n1. Lock/Unlock failures route through GetErrorString; gated on init.
RVA(0x00135f40, 0x169)
i32 DirectSoundMgr::LockConvert(void* src, u32 lockBytes, u32 convert) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }

    void* p1;
    void* p2;
    u32 n1;
    u32 n2;
    i32 hr = m_buffer->Lock(0, lockBytes, &p1, &n1, &p2, &n2, DSBLOCK_ENTIREBUFFER) != 0;
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

    hr = m_buffer->Unlock(p1, n1, p2, n2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x2e1, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DSoundCloneInst::StopAllClones (__thiscall, no args). Gated on init. Walks the
// clone list, calling StopAndRewind on each clone (node->m_inst) so every playing
// voice halts and rewinds; the list itself is left intact.
RVA(0x00136150, 0x22)
void DSoundCloneInst::StopAllClones() {
    if (m_owner->m_initialized == 0) {
        return;
    }
    for (CloneNode* node = m_cloneList.m_head; node != 0; node = node->m_next) {
        node->m_inst->StopAndRewind();
    }
}

// ---------------------------------------------------------------------------
// DSoundCloneInst ctor (0x135b10, __thiscall). The buffer leaf: chain the
// DSoundBaseSub base ctor (its base call is out-of-line - the base ctor is defined
// below), initialize the empty clone list, then insert THIS node's own anchor
// (m_cloneNode) as the list's seed head, and stamp the play key. cl auto-stamps
// ??_7DSoundCloneInst@@6B@ (0x5ef6bc) here.
// @early-stop
// EH-state-count wall: code bytes are byte-identical (verified base-vs-target
// llvm-objdump -dr) except the /GX unwind state machine. Realized as a real derived
// ctor for the ALL-VTABLES phase; the DSoundBaseSub ctor is DEFINED AFTER this so cl
// only sees its declaration at the call site => an out-of-line `call 0x136230`.
RVA(0x00135b10, 0x6b)
DSoundCloneInst::DSoundCloneInst(IDirectSoundBufferZ* buf, SoundDevice* owner)
    : DSoundBaseSub(buf, owner) {
    m_cloneList.m_head = 0;
    m_cloneList.m_tail = 0;
    // cl auto-stamps ??_7DSoundCloneInst@@6B@ (0x5ef6bc) here.
    m_cloneList.InsertHead(&m_cloneNode);
    m_playKey = 1;
}

// ---------------------------------------------------------------------------
// DSoundBaseSub 2-arg ctor (0x136230, __thiscall). Chains the DirectSoundMgr base
// ctor; cl auto-stamps ??_7DSoundBaseSub@@6B@ (0x5ef6c0). Records this clone's
// back-pointer (m_cloneNode.m_inst) and self as the reacquire owner, seeds the play key.
RVA(0x00136230, 0x2d)
DSoundBaseSub::DSoundBaseSub(IDirectSoundBufferZ* buf, SoundDevice* owner)
    : DirectSoundMgr(buf, owner) {
    // cl auto-stamps ??_7DSoundBaseSub@@6B@ (0x5ef6c0) here.
    m_cloneNode.m_inst = this;
    m_reacquireOwner = this;
    m_playKey = 1;
}

// ---------------------------------------------------------------------------
// DSoundBaseSub clone/duplicate ctor (0x136180, __thiscall). The object Clone()
// news for a duplicated sound buffer. Chains the DirectSoundMgr base ctor; cl
// auto-stamps ??_7DSoundBaseSub@@6B@ (0x5ef6c0). Records this clone's back-pointer
// (m_cloneNode.m_inst) and the source manager (m_reacquireOwner=original), copies the
// original's sample/reacquire/rate parameter block, then recomputes the duration.
// The trailing ComputeDuration() call over the destructible DirectSoundMgr base is
// what gives this ctor a /GX ctor-in-flight EH frame (the 2-arg ctor has none).
RVA(0x00136180, 0x86)
DSoundBaseSub::DSoundBaseSub(IDirectSoundBufferZ* buf, SoundDevice* owner, DirectSoundMgr* original)
    : DirectSoundMgr(buf, owner) {
    m_cloneNode.m_inst = this;
    m_reacquireOwner = original;
    m_playKey = 1;
    m_sampleCount = original->m_sampleCount;
    m_reacquireCb = original->m_reacquireCb;
    m_reacquireCtx = original->m_reacquireCtx;
    m_sampleRate = original->m_sampleRate;
    m_rateBase = original->m_rateBase;
    ComputeDuration();
}

// ---------------------------------------------------------------------------
// DSoundBaseSub::~DSoundBaseSub (__thiscall). Empty body: cl emits the implicit
// base-subobject vptr reset to ??_7DSoundBaseSub (0x5ef6c0), then chains
// ~DirectSoundMgr (0x135300).
RVA(0x00136260, 0xb)
DSoundBaseSub::~DSoundBaseSub() {}

// ---------------------------------------------------------------------------
// DirectSoundMgr::Play (__thiscall, no args). Gated on init. Plays the buffer with
// the +0x14 looping flag; on DSERR_BUFFERLOST asks m_reacquireOwner to reacquire and
// retries once. A retry failure or a non-buffer-lost HRESULT is reported via
// GetErrorString; a failed reacquire returns 0 silently. Same shape as Lock.
RVA(0x00136270, 0x8b)
i32 DirectSoundMgr::Play() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Play(0, 0, m_playFlags) != 0;
    if (hr != 0) {
        if (hr == DSERR_BUFFERLOST) {
            if (m_reacquireOwner->ReacquireBuffer() == 0) {
                return 0;
            }
            i32 hr2 = m_buffer->Play(0, 0, m_playFlags) != 0;
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
// DirectSoundMgr::ApplyAndPlay (__thiscall, 4 args). Gated on owner init. Apply the
// four play params - SetVolumeByIndex, SetPanByIndex, SetField2, SetField3 - then
// start playback (Play, with its own DSERR_BUFFERLOST reacquire-retry). The third
// setter's result is not folded into the success flag; every other failure clears
// it. Returns the accumulated 0/1.
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
// on DSERR_BUFFERLOST (0x88780096) it asks m_reacquireOwner to reacquire the buffer
// and, if that succeeds, retries the Lock once. A retry failure or a non-buffer-lost
// HRESULT is reported via GetErrorString; a failed reacquire returns 0 silently.
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
    i32 hr = m_buffer->Lock(off, bytes, p1, n1, p2, n2, flags) != 0;
    if (!hr) {
        return 1;
    }
    if (hr == DSERR_BUFFERLOST) {
        if (m_reacquireOwner->ReacquireBuffer() == 0) {
            return 0;
        }
        hr = m_buffer->Lock(off, bytes, p1, n1, p2, n2, flags) != 0;
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
// SoundDevice::Create (__thiscall). Brings up the DirectSound device:
// DirectSoundCreate into m_device (its HRESULT normalized into a stored bool), then
// SetCooperativeLevel(hwnd, level). A failed coop call is reported and the device
// released; on success the coop params are cached (m_coopLevel/m_bufferFlags),
// m_createFlag cleared and the m_initialized flag set.
RVA(0x00136550, 0x8c)
i32 SoundDevice::Create(void* hwnd, u32 level, u32 flags) {
    i32 created = DirectSoundCreate(0, &m_device, 0) != 0;
    if (created) {
        return 0;
    }
    i32 hr = m_device->SetCooperativeLevel(hwnd, level) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3b0, hr);
        m_device->Release();
        return 0;
    }
    m_coopLevel = level;
    m_bufferFlags = flags;
    m_createFlag = 0;
    m_initialized = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// SoundDevice::ReacquireViaCallback (__thiscall). Tail-dispatch through the
// installed reacquire callback (m_reacquireProc, a __thiscall pointer-to-member on
// the device); returns its result, or 0 when no callback is installed.
// (mov eax,[ecx+0x80]; jmp eax)
RVA(0x001365e0, 0xf)
i32 SoundDevice::ReacquireViaCallback() {
    if (m_reacquireProc != 0) {
        return (this->*m_reacquireProc)();
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SoundDevice::SetCooperativeLevel (__thiscall). Re-issues
// IDirectSound::SetCooperativeLevel on an already-created device; caches the new
// level in m_coopLevel. Gated on the m_initialized flag.
RVA(0x001365f0, 0x57)
i32 SoundDevice::SetCooperativeLevel(void* hwnd, u32 level) {
    if (m_initialized == 0) {
        return 0;
    }
    i32 hr = m_device->SetCooperativeLevel(hwnd, level) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3cf, hr);
        return 0;
    }
    m_coopLevel = level;
    return 1;
}

// ---------------------------------------------------------------------------
// SoundDevice::CreatePrimaryBuffer (__thiscall). Once the device is up
// (m_initialized) and the cooperative level is DSSCL_NORMAL (m_coopLevel == 1),
// lazily creates the primary sound buffer into m_primaryBuffer via
// IDirectSound::CreateSoundBuffer with a stack DSBUFFERDESC (dwSize = sizeof,
// dwFlags = m_bufferFlags | DSBCAPS_PRIMARYBUFFER).
RVA(0x00137260, 0x95)
i32 SoundDevice::CreatePrimaryBuffer() {
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
        i32 hr = m_device->CreateSoundBuffer(&desc, &m_primaryBuffer, 0) != 0;
        if (hr) {
            DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x6ab, hr);
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
        case DSERR_BUFFERLOST:
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
// SoundDevice::PurgeVoiceList @0x136e20 - the per-tick voice-list purge. The voice
// list (m_voiceList @ +0xc, same list InsertHead/RemoveMatching use) holds
// DSoundVoice nodes (link at +0x04, so element == link - 4). Once per tick window
// (gated by m_initialized + the m_createFlag timestamp), walk the list and for
// every voice whose per-frame update (Tick, vtbl slot 0) reports "done" (0), unlink
// it and free it via the pure-base teardown (`delete (PureSoundElem*)e` = reset
// vptr to ??_7PureSoundElem + RezFree). The `time == -1` arm resolves the current
// time via the timeGetTime pointer.
// @early-stop
// select-zero-mask-dest-register wall (docs/patterns/select-zero-mask-dest-register.md,
// SAME as DSoundList::RemoveMatching @0x136f60): byte-exact except the `e ? &link : 0`
// mask (neg/sbb/and) lands in a different free-list register than retail.
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650
RVA(0x00136e20, 0xa8)
i32 SoundDevice::PurgeVoiceList(i32 time) {
    if (m_initialized == 0) {
        return 0;
    }
    DSoundLink* head = m_voiceList.m_head;
    DSoundVoice* e = elemOf<DSoundVoice>(head);
    if (e == 0) {
        return 0;
    }
    if (time == -1) {
        time = (i32)g_pTimeGetTime();
    }
    if ((u32)time <= (u32)m_createFlag) {
        return 1;
    }
    m_createFlag = time;
    do {
        DSoundLink* n = e->m_link.m_next;
        DSoundVoice* next = elemOf<DSoundVoice>(n);
        if (e->Tick(time) == 0) {
            m_voiceList.Unlink(e ? &e->m_link : 0);
            if (e) {
                PureSoundElem* pure = e; // up-cast: teardown resets to the pure base + RezFree
                delete pure;
            }
        }
        e = next;
    } while (e);
    return 1;
}

// -------------------------------------------------------------------------
// SoundDevice::TickSubManagers @0x137ac0 - the per-frame sub-manager tick. Walk the
// derived instance list (m_instanceHead @ +0x94, link-at-+4 chain), and for each
// sub-object: advance its inner voice list (sub+0x6c) with the current time
// (0x137e30), poll its guard (sub->m_guard, 0x1353f0); when the guard reports idle
// (0) and the sub is active (m_active), stop the inner list (0x1380d0) if flagged
// (m_stopFlag) and retire the sub (0x1379d0) if flagged (m_retireFlag). Records the
// guard result back into m_active.
// The inner voice list embedded in each sub-object at +0x6c - a call-view over the
// two __thiscall helpers the tick drives (advance / stop).
struct SubInnerList {
    void Tick(i32 t); // 0x137e30  advance the inner voice list
    void Stop(i32 x); // 0x1380d0  stop the inner voice list
};
SIZE(SubInnerList, 0x1); // call-view sub-object embedded at SubNode+0x6c
// The idle guard object a sub-object hangs at +0x74 - a call-view over its poll
// (which shares the IsPlaying RVA 0x1353f0).
struct SubGuard {
    i32 Poll(); // 0x1353f0  idle-check
};
SIZE(SubGuard, 0x1); // call-view over the guard object (SubNode::m_guard)
// A sub-object (stream instance) in the device's instance list: an intrusive link
// at +0x04 (element == link - 4), stop/retire flags + an active flag at
// +0x60..+0x68, the inner voice list embedded at +0x6c and the idle guard at +0x74.
// A partial view of the real stream instance (its trailing fields are not modeled).
struct SubNode {
    char m_pad0[0x4];
    DSoundLink* m_next; // +0x04  instance-list forward link (biased +4)
    char m_pad8[0x60 - 0x8];
    i32 m_stopFlag;           // +0x60  stop-inner-list-when-idle flag
    i32 m_retireFlag;         // +0x64  retire-when-idle flag
    i32 m_active;             // +0x68  active flag (updated with the guard result)
    SubInnerList m_innerList; // +0x6c  embedded inner voice list
    char m_pad6d[0x74 - 0x6d];
    SubGuard* m_guard; // +0x74  idle guard object
};
SIZE(SubNode, 0x78); // instance-list node view (fields end at +0x78)
RVA(0x00137ac0, 0xa2)
i32 SoundDevice::TickSubManagers(i32 time) {
    if (time == -1) {
        time = (i32)g_pTimeGetTime();
    }
    DSoundLink* head = m_instanceHead;
    SubNode* o = elemOf<SubNode>(head);
    while (o) {
        SubNode* next = elemOf<SubNode>(o->m_next);
        o->m_innerList.Tick(time);
        i32 r = o->m_guard->Poll();
        if (r == 0 && o->m_active != 0) {
            if (o->m_stopFlag != 0) {
                o->m_innerList.Stop(-1);
            }
            if (o->m_retireFlag != 0) {
                RemoveSub(o);
                o = 0;
            }
        }
        if (o) {
            o->m_active = r;
        }
        o = next;
    }
    return 1;
}
