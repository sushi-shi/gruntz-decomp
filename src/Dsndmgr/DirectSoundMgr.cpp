// DirectSoundMgr.cpp - per-buffer DirectSound wrapper run (C:\Proj\Dsndmgr\DSNDMGR.CPP).
// DirectSoundMgr is the wrapper base (vtable 0x5ef6b8); two leaves derive it here:
// DSoundBaseSub (0x5ef6c0, 0x58B clone) and DSoundCloneInst (0x5ef6bc, 0x60B owns a
// clone list). SoundDevice bring-up methods in this RVA range live here too.
// GetErrorString maps a DSound HRESULT to a string + beeps/logs/MessageBox per three
// reporting globals (switch VALUES + DSERR_*/"DirectSoundMgr" strings are load-bearing).
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/DSoundVoice.h> // the 0x28-byte voice node CloneAndPlay news
#include <Dsndmgr/SoundDevice.h> // SoundDevice: the owning device (m_owner) + its methods
#include <Win32.h>               // windows.h base types (dsound.h needs them)
#include <mmsystem.h>            // WAVEFORMATEX (dsound.h needs it predefined)
#include <dsound.h> // real DirectSound SDK (IDirectSound/Buffer, DSBUFFERDESC, DSBCAPS)
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

// MessageBeep/MessageBoxA/OutputDebugStringA + Win32 types via <windows.h> (Win32.h).
#include <Win32.h>
#include <Globals.h>

// Reporting-mode globals (.data): g_logEnabled -> OutputDebugStringA, g_msgBoxEnabled
// -> MessageBox, g_beepEnabled -> startup beep, g_thirdEnabled -> "any output" gate.
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

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x002293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// __FILE__ every wrapper passes to GetErrorString (single $SG pooled constant).
#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

// DSOUND SDK magics (real values, defined locally like DirectInputMgr2.cpp).
#define DSB_RETAIL_LOOPBIT                                                                         \
    0x02 // retail IsLooping mask; DX6 dsound.h DSB_RETAIL_LOOPBIT is 0x04, this game used the DX5-era 0x02 (byte-proven, load-bearing)

// DSOUND.dll ordinal #1 device creator; no dllimport -> direct `e8 rel32` thunk (retail).

// DSound centi-dB [-10000..0] -> 0..100 percent; free __cdecl helper 0x135110.
extern "C" i32 ConvertVolumeToPercent(i32 vol);

// Volume->attenuation table SetVolumeByIndex indexes (0x653ab8, filled in SoundDevice.cpp).
extern i32 g_volumeTable[100];
// The pan table (0x653c48, Globals.h) sits right after: +arg reads the negated entry.

// The DirectSoundMgr clone hierarchy (CloneList / DSoundBaseSub / DSoundCloneInst) is
// now defined in DirectSoundMgr.h so the device + feeder can name the concrete leaf;
// the method bodies below are unchanged.

// ---------------------------------------------------------------------------
// ~DirectSoundMgr - empty base-subobject dtor (just the vptr reset to 0x5ef6b8).
RVA(0x00135300, 0x7)
DirectSoundMgr::~DirectSoundMgr() {}

// ---------------------------------------------------------------------------
// ctor: cache buffer + owner, zero the caps/state, then (if buf) read caps into m_caps
// and cache initial freq/pan/volume guarded by the capability bits (report on failure).
RVA(0x001351d0, 0x109)
DirectSoundMgr::DirectSoundMgr(IDirectSoundBuffer* buf, SoundDevice* owner) {
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
        i32 hr = buf->GetFrequency((LPDWORD)&m_freq) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x58, hr);
        }
    }
    m_setFreq = m_freq;

    if ((m_caps & DSBCAPS_CTRLPAN) == DSBCAPS_CTRLPAN) {
        i32 hr = buf->GetPan((LONG*)&m_pan) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x60, hr);
        }
    } else {
        m_pan = 0;
    }

    if ((m_caps & DSBCAPS_CTRLVOLUME) == DSBCAPS_CTRLVOLUME) {
        i32 hr = buf->GetVolume((LONG*)&m_volume) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x68, hr);
        }
    } else {
        m_volume = 0;
    }
}

// ---------------------------------------------------------------------------
// Restore: IDirectSoundBuffer::Restore; HRESULT normalized to 0/1 (neg/sbb/neg), that
// bool is both tested and forwarded to GetErrorString. 0 on failure, else 1.
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
// ReacquireBuffer: gated on owner init. If m_reacquireCb(this, m_reacquireCtx) is set,
// call it (1 on success); else tail into owner->ReacquireViaCallback (0x1365e0).
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
// StopAndRewind: gated on owner init; Stop() then SetCurrentPosition(0), reported.
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
// IsPlaying: GetStatus, report on failure, return the "playing" bit.
// @early-stop
// byte-AND-width wall (99%): `and eax,1` (ours) vs retail `and al,1` on a dword status
// load; a (u8) cast wrongly narrows the load too. /O2 partial-reg pick, not steerable.
RVA(0x001353f0, 0x4b)
i32 DirectSoundMgr::IsPlaying() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    u32 status;
    i32 hr = m_buffer->GetStatus((LPDWORD)&status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xac, hr);
        return 0;
    }
    return (status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING;
}

// ---------------------------------------------------------------------------
// IsLooping: GetStatus, report on failure, return the "looping" bit.
// @early-stop
// byte-AND-width wall (99%): same as IsPlaying (`and eax,2` vs retail `and al,2`).
RVA(0x00135440, 0x4d)
i32 DirectSoundMgr::IsLooping() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    u32 status;
    i32 hr = m_buffer->GetStatus((LPDWORD)&status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xbb, hr);
        return 0;
    }
    return (status & DSB_RETAIL_LOOPBIT) == DSB_RETAIL_LOOPBIT;
}

// ---------------------------------------------------------------------------
// SetField3: gated on init; set/clear bit 0 of m_playFlags (the Play looping flag).
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
// SetVolume: gated on init + volume-cap bit (m_caps & 0x80); SetVolume, report.
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
// SetVolumeByIndex: gated on init; SetVolume(g_volumeTable[idx]).
RVA(0x001355c0, 0x23)
i32 DirectSoundMgr::SetVolumeByIndex(i32 idx) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return SetVolume(g_volumeTable[idx]);
}

// ---------------------------------------------------------------------------
// GetVolume: GetVolume out-param; report + 0 on failure, else the queried volume.
RVA(0x001355f0, 0x42)
i32 DirectSoundMgr::GetVolume() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 vol;
    i32 hr = m_buffer->GetVolume((LONG*)&vol) != 0;
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
// SetPan: gated on init + pan-cap bit (m_caps & 0x40); SetPan, report.
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
// SetPanByIndex: gated on init; idx>=0 -> SetPan(-g_panTable[-idx]), else direct.
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
// GetPan: GetPan out-param; report + 0 on failure, else the queried pan.
RVA(0x001357f0, 0x42)
i32 DirectSoundMgr::GetPan() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 pan;
    i32 hr = m_buffer->GetPan((LONG*)&pan) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x15e, hr);
        return 0;
    }
    return pan;
}

// ---------------------------------------------------------------------------
// SetFrequency: gated on init + freq-cap bit (m_caps & 0x20); SetFrequency, report
// (HRESULT tested directly, not normalized), cache in m_setFreq on success.
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
// SetField2: gated on init; bump freq by pct% of m_freq (clamp [101,99999]) via
// SetFrequency, set m_sampleRate = pct% of m_rateBase, recompute duration; ret r.
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
// ComputeDuration: m_durationMs = m_sampleCount*1000/m_sampleRate (*1000 = lea/shl chain).
RVA(0x001359a0, 0x18)
void DirectSoundMgr::ComputeDuration() {
    m_durationMs = m_sampleCount * 1000 / m_sampleRate;
}

// ---------------------------------------------------------------------------
// Unlock: pass-through IDirectSoundBuffer::Unlock; report on failure.
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
    i32 hr = m_buffer->GetCurrentPosition((LPDWORD)play, (LPDWORD)write) != 0;
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
    i32 hr = m_buffer->GetFormat((LPWAVEFORMATEX)fmt, size, (LPDWORD)written) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1e2, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ~DSoundCloneInst: drain the clone list via RemoveClone (re-reads head each pass);
// cl stamps the clone vptr + chains ~DSoundBaseSub; /GX EH frame.
// @early-stop
// EH-dtor / vptr-reset schedule: clone-loop byte-exact; real derived dtor so cl emits
// the clone/base/buffer vptr resets + base dtor chain.
RVA(0x00135bb0, 0x63)
DSoundCloneInst::~DSoundCloneInst() {
    while (m_cloneList.m_head != 0) {
        RemoveClone(m_cloneList.m_head->m_inst);
    }
}

// ---------------------------------------------------------------------------
// Clone: gated on init; new a DSoundBaseSub clone (base ctor, original=this),
// DuplicateSoundBuffer into clone->m_buffer (report 0x217 on failure), link into
// m_cloneList, stamp m_playKey, return the clone.
RVA(0x00135c20, 0xf6)
DirectSoundMgr* DSoundCloneInst::Clone(i32 a) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DSoundBaseSub* clone = new DSoundBaseSub(m_buffer, m_owner, this);
    if (clone == 0) {
        return 0;
    }
    IDirectSound* dev = m_owner->m_device;
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
// RemoveClone: gated on init; for a real clone (!= this) Release+clear its buffer,
// unlink from m_cloneList, then delete it.
RVA(0x00135d20, 0x47)
void DSoundCloneInst::RemoveClone(DirectSoundMgr* clone) {
    if (m_owner->m_initialized == 0) {
        return;
    }
    if (clone != this) {
        IDirectSoundBuffer* buf = clone->m_buffer;
        buf->Release();
        clone->m_buffer = 0;
    }
    m_cloneList.Unlink(&clone->m_cloneNode);
    if (clone != this) {
        delete clone;
    }
}

// ---------------------------------------------------------------------------
// LockConvert: write src into the buffer across Lock's two wrap regions (p1/n1,p2/n2).
// `convert`==0 -> byte memcpy; !=0 -> 16->8 downconvert ((s+0x8000)>>8, high byte).
// Region 2 source continues at src+n1; gated on init, Lock/Unlock report on failure.
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
        m_buffer->Lock(0, lockBytes, &p1, (LPDWORD)&n1, &p2, (LPDWORD)&n2, DSBLOCK_ENTIREBUFFER)
        != 0;
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
// StopAllClones: gated on init; StopAndRewind each clone (list left intact).
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
// DSoundCloneInst ctor 0x135b10: chain DSoundBaseSub base ctor, init empty clone list,
// seed head with m_cloneNode, stamp m_playKey; cl stamps 0x5ef6bc.
// @early-stop
// EH-state-count wall: code bytes byte-identical (base-vs-target llvm-objdump -dr) except
// the /GX unwind state machine. Base ctor DEFINED AFTER -> out-of-line `call 0x136230`.
RVA(0x00135b10, 0x6b)
DSoundCloneInst::DSoundCloneInst(IDirectSoundBuffer* buf, SoundDevice* owner)
    : DSoundBaseSub(buf, owner) {
    m_cloneList.m_head = 0;
    m_cloneList.m_tail = 0;
    // cl auto-stamps ??_7DSoundCloneInst@@6B@ (0x5ef6bc) here.
    m_cloneList.InsertHead(&m_cloneNode);
    m_playKey = 1;
}

// ---------------------------------------------------------------------------
// DSoundBaseSub 2-arg ctor 0x136230: chain base ctor (cl stamps 0x5ef6c0); set
// m_cloneNode.m_inst=this, m_reacquireOwner=this, m_playKey=1.
RVA(0x00136230, 0x2d)
DSoundBaseSub::DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner)
    : DirectSoundMgr(buf, owner) {
    // cl auto-stamps ??_7DSoundBaseSub@@6B@ (0x5ef6c0) here.
    m_cloneNode.m_inst = this;
    m_reacquireOwner = this;
    m_playKey = 1;
}

// ---------------------------------------------------------------------------
// DSoundBaseSub clone/dup ctor 0x136180 (what Clone() news): chain base ctor, set
// back-ptr + m_reacquireOwner=original, copy its sample/reacquire/rate block, recompute
// duration. The trailing ComputeDuration over the destructible base -> /GX EH frame.
RVA(0x00136180, 0x86)
DSoundBaseSub::DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner, DirectSoundMgr* original)
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
// ~DSoundBaseSub: empty; cl emits the vptr reset to 0x5ef6c0 + chains ~DirectSoundMgr.
RVA(0x00136260, 0xb)
DSoundBaseSub::~DSoundBaseSub() {}

// ---------------------------------------------------------------------------
// Play: gated on init; Play(m_playFlags); on DSERR_BUFFERLOST reacquire + retry once
// (report on retry-fail or non-buffer-lost HRESULT, silent 0 on reacquire-fail).
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
// ApplyAndPlay: gated on init; SetVolumeByIndex/SetPanByIndex/SetField2/SetField3 then
// Play. SetField3's result is not folded in; every other failure clears the 0/1 accum.
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
// Lock: pass-through IDirectSoundBuffer::Lock; on DSERR_BUFFERLOST reacquire + retry once.
// @early-stop
// tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md, ~67.7%): the
// two GetErrorString(0x37c/0x386) report+ret-0 blocks tail-merge into one where retail
// duplicates them inline. Optimizer layout choice, not source-steerable.
RVA(0x00136370, 0xcc)
i32 DirectSoundMgr::Lock(u32 off, u32 bytes, void** p1, u32* n1, void** p2, u32* n2, u32 flags) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Lock(off, bytes, p1, (LPDWORD)n1, p2, (LPDWORD)n2, flags) != 0;
    if (!hr) {
        return 1;
    }
    if (hr == DSERR_BUFFERLOST) {
        if (m_reacquireOwner->ReacquireBuffer() == 0) {
            return 0;
        }
        hr = m_buffer->Lock(off, bytes, p1, (LPDWORD)n1, p2, (LPDWORD)n2, flags) != 0;
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
// SoundDevice::Create: DirectSoundCreate into m_device, then SetCooperativeLevel;
// on fail report + Release; on success cache coop params, clear m_createFlag, set init.
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
// ReacquireViaCallback: tail-dispatch through m_reacquireProc (ptr-to-member), or 0.
RVA(0x001365e0, 0xf)
i32 SoundDevice::ReacquireViaCallback() {
    if (m_reacquireProc != 0) {
        return (this->*m_reacquireProc)();
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SetCooperativeLevel: gated on init; re-issue SetCooperativeLevel, cache m_coopLevel.
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
// StartPrimary (0x137200, re-homed from BoundaryUpper2): gated on init, lazily
// (re)create the primary buffer, then start it looping (IDirectSoundBuffer::Play
// slot 12, DSBPLAY_LOOPING); report + fail on a non-zero HRESULT. (The
// BoundaryUpper2 view mislabeled this "SoundDevice::Restore" over a placeholder
// ISndBuf slot 0x30 - it is the real StartPrimary declared in SoundDevice.h.)
RVA(0x00137200, 0x53)
i32 SoundDevice::StartPrimary() {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_primaryBuffer->Play(0, 0, DSBPLAY_LOOPING) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x68b, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CreatePrimaryBuffer: gated on init + m_coopLevel==DSSCL_NORMAL; lazily
// CreateSoundBuffer(primary, m_bufferFlags|DSBCAPS_PRIMARYBUFFER) into m_primaryBuffer.
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
// PurgeVoiceList @0x136e20 - per-tick voice purge. Gated on init + m_createFlag time
// window; walk m_voiceList (DSoundVoice nodes) and for each whose Tick (slot 0) reports
// done (0), unlink + `delete (PureSoundElem*)e` (pure-base teardown). time==-1 -> clock.
// @early-stop
// select-zero-mask-dest-register wall (docs/patterns/select-zero-mask-dest-register.md,
// SAME as DSoundList::RemoveMatching @0x136f60): byte-exact except the `e ? &link : 0`
// mask (neg/sbb/and) lands in a different register than retail.
// g_pTimeGetTime = genuine game global fn-ptr (retail _g_pTimeGetTime @ 0x6c4650), NOT
// the WINMM import; call ds:[0x6c4650] indirection - do NOT swap for timeGetTime.
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
// TickSubManagers @0x137ac0 - per-frame sub-manager tick. Walk m_instanceHead; per
// sub: advance inner list (+0x6c), poll guard (m_guard); when idle(0) && m_active, stop
// inner list (if m_stopFlag) and retire (if m_retireFlag); record guard back to m_active.
// IDENTITY CONFIRMED (wave 3): SubNode IS the canonical StreamVoice (<Dsndmgr/
// StreamVoice.h>, DirectSoundMgr-derived, ctor 0x1375b0) - m_instanceHead@+0x94 is the
// SoundStream StreamVoice list - and SubInnerList IS its embedded StreamVoiceFeeder
// (StreamVoice::m_feeder @+0x6c). SubInnerList's Tick(0x137e30)/Stop(0x1380d0) are the
// feeder pump entries (0x1380d0 == StreamFeeder::TickPump; both COMDAT-fold with the
// ApiMisc Throttle_137e30/Timer_1380d0 placeholders, which is why they read as those).
// CLEAN FOLD DEFERRED: SoundDevice::TickSubManagers (0x137ac0, below) is 100%-EXACT and
// a byte-safe fold needs StreamVoice.h/StreamFeeder.h extended with the +0x60..+0x68
// flags, the +0x74 DirectSoundMgr guard, and the folded feeder-method names - a
// DirectSound-module task; kept a documented call-view so the 100% match holds.
struct SubInnerList { // == StreamVoiceFeeder (StreamVoice::m_feeder @+0x6c)
    void Tick(i32 t); // 0x137e30  advance the feeder (folds Throttle_137e30::Tick)
    void Stop(i32 x); // 0x1380d0  StreamFeeder::TickPump (folds Timer_1380d0::Tick)
};
SIZE(SubInnerList, 0x1); // call-view over StreamVoice::m_feeder @+0x6c
// == StreamVoice (see note above): link@+0x04, stop/retire/active flags @+0x60..+0x68,
// feeder @+0x6c, guard @+0x74. m_guard IS the canonical DirectSoundMgr (its Poll is
// DirectSoundMgr::IsPlaying @0x1353f0, folded wave 3 - StreamVoice IS DirectSoundMgr-
// derived, so the guard is the per-voice buffer manager owning the idle-check).
struct SubNode { // == StreamVoice (<Dsndmgr/StreamVoice.h>); fold deferred (see note)
    char m_pad0[0x4];
    DSoundLink* m_next; // +0x04  instance-list forward link (biased +4)
    char m_pad8[0x60 - 0x8];
    i32 m_stopFlag;           // +0x60  stop-inner-list-when-idle flag
    i32 m_retireFlag;         // +0x64  retire-when-idle flag
    i32 m_active;             // +0x68  active flag (updated with the guard result)
    SubInnerList m_innerList; // +0x6c  embedded inner voice list
    char m_pad6d[0x74 - 0x6d];
    DirectSoundMgr* m_guard; // +0x74  the stream's buffer manager (IsPlaying idle-check)
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
        i32 r = o->m_guard->IsPlaying();
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
