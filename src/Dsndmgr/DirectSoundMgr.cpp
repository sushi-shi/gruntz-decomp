// DirectSoundMgr.cpp
// original TU: C:\Proj\Dsndmgr\DSNDMGR.CPP
//
// The WHOLE DSNDMGR.CPP object, consolidated per docs/exe-map/interval-dossiers.md
// (interval 0x1350b0-0x13848b splits at 0x137330 into DSNDMGR.CPP + DSndMgSR.cpp;
// __FILE__-anchored). One retail obj = one TU, functions in retail RVA order:
//   * SoundDevice - the DirectSound *device* manager (vftable 0x5ef6c4): bring-up,
//     buffer/voice lists, RIFF acquire/reload, primary buffer (was SoundDevice.cpp).
//   * DirectSoundMgr - the per-buffer wrapper base (vftable 0x5ef6b8) + its two
//     leaves DSoundBaseSub (0x5ef6c0, 0x58B clone) and DSoundCloneInst (0x5ef6bc,
//     0x60B owns a clone list).
//   * DSoundVoice - the 0x28-byte volume-ramp voice node (vftable 0x5ef6d0),
//     Tick/Stop (was DSoundVoice.cpp). Its ctor 0x136fe0 is NOT yet reconstructed
//     (no source body anywhere; the dossier's "SoundTick_Ctor" gap) - CloneAndPlay's
//     `new DSoundVoice(...)` reloc-masks to it.
//   * DSoundList::RemoveMatching (0x136f60) - the reaping list helper (the other
//     five DSoundList primitives live at 0x1390e0+, OUTSIDE this obj's span, and
//     stay in SoundVoiceList.cpp).
//   * CSoundCueMgr::GetItem/ConfigureItem (0x135d70/0x1360d0) - the Dsndmgr UI
//     sound-cue manager (dossier seam re-homes from statusbarmgrgetitem /
//     spriteresource; both walk DirectSoundMgr voices).
//   * ConvertVolumeToPercent (0x135110) - the centi-dB -> percent transfer curve
//     (dossier seam re-home; was the mis-homed "GruntCmdPercent" /Odi singleton,
//     now compiled under this TU's real /O2 /GX profile).
//   * SoundDevice::AcquireResource/ReloadResource (0x136a30/0x136ce0) - the WAVE
//     Win32-resource siblings of AcquireFile/ReloadFile (dossier seam re-homes;
//     were the ResLoaders::WaveHost_136a30/WaveHost2_136ce0 views).
//
// GetErrorString + SetDSoundReportModes (0x138120/0x138150) moved OUT to the
// DSndMgSR.cpp side (SoundStream.cpp): they sit AFTER the StreamFeeder block in
// retail, past the 0x137330 file boundary. Same for ??1PureSoundElem (0x137330)
// and SoundDevice::TickSubManagers (0x137ac0).
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundVoiceList.h>
#include <Dsndmgr/DSoundVoice.h> // the 0x28-byte voice node CloneAndPlay news
#include <Dsndmgr/SoundDevice.h> // SoundDevice: the owning device (m_owner) + its methods
#include <Rez/RezMgr.h>          // RezAlloc/RezFree/RezFRead/RezFClose - the engine heap/file layer
                                 // (pulls <Mfc.h>; MUST precede <Win32.h> - the C1189 rule)
#include <Win32.h>               // windows.h base types (dsound.h needs them)
#include <mmsystem.h>            // WAVEFORMATEX (dsound.h needs it predefined)
#include <dsound.h> // real DirectSound SDK (IDirectSound/Buffer, DSBUFFERDESC, DSBCAPS)
#include <rva.h>
#include <math.h>   // acos / pow (intrinsic __CIacos / __CIpow) in the volume curves
#include <stdio.h>  // engine sprintf (reloc-masked); FILE - the CRT stream Eng_fopen returns
#include <string.h> // inline strcpy/memcpy (rep movs / repne scasb)

#include <Globals.h> // c_volScale/c_volNum/c_acosNorm/c_powExp + g_panTable

#include <Gruntz/SoundCueMgr.h> // CSoundCueMgr (GetItem/ConfigureItem live in this obj)

// __FILE__ every wrapper passes to GetErrorString (single $SG pooled constant).
#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

// DSOUND SDK magics (real values, defined locally like DirectInputMgr2.cpp).
#define DSB_RETAIL_LOOPBIT                                                                         \
    0x02 // retail IsLooping mask; DX6 dsound.h DSB_RETAIL_LOOPBIT is 0x04, this game used the DX5-era 0x02 (byte-proven, load-bearing)

// DSBUFFERDESC.dwSize (the 0x14-byte sound-buffer descriptor).
#define DSBUFFERDESC_SIZE 0x14

// A little-endian RIFF FourCC as a u32 (compile-time constant -> the same immediate
// the retail chunk-tag compares use).
#define WAVE_FOURCC(a, b, c, d)                                                                    \
    ((u32)(u8)(a) | ((u32)(u8)(b) << 8) | ((u32)(u8)(c) << 16) | ((u32)(u8)(d) << 24))

// DSOUND.dll ordinal #1 device creator; no dllimport -> direct `e8 rel32` thunk (retail).

// RIFF/WAVE chunk parser 0x137110 (__cdecl): scan a RIFF blob for `fmt `/`data`,
// write fmt (*out), PCM ptr (*dataOut) + len (*sizeOut); nonzero if `fmt ` found.
extern "C" i32 ParseWaveChunks(void* riff, ParseFmt* out, void** dataOut, u32* sizeOut);

// Volume->attenuation table SetVolumeByIndex indexes (rva 0x253ab8), filled by
// BuildVolumeTable(i=0..100); INCLUSIVE loop -> the 101st store lands on
// g_panTable[0] (retail). The pan table (rva 0x253c48, Globals.h) sits right after:
// SetPanByIndex's +arg reads the negated entry.
DATA(0x00253ab8)
i32 g_volumeTable[100];

// Engine fopen 0x11f870 (CRT FILE*) + file-size query 0x18c480 (reads FILE._file fd).
extern "C" FILE* Eng_fopen(const char* path, const char* mode); // 0x11f870
extern "C" u32 Eng_filelength(i32 fd);                          // 0x18c480

// "rb" open-mode string the loader passes fopen (.data, rva 0x20b668). Bound via
// @data-symbol, not DATA: clang mangles the const-char[] extern with a `Q` storage
// class while cl 5.0 emits `P` (?s_rb@@3PBDB), so a DATA() label's clang mangledName
// would miss the base obj's undefined external. The @data-symbol name is the exact
// cl mangling and is authority-checked against the base obj.
// @data-symbol: ?s_rb@@3PBDB 0x0020b668
extern const char s_rb[];

// The app resource-module accessor AcquireResource/ReloadResource read the
// HINSTANCE from (global accessor 0x1d3631, module handle @+0x08; the same local
// view ResourceLoaders.cpp keeps for its RT_BITMAP/PALETTE loaders).
struct AppModule_136a30 {
    char m_pad0[8];
    HINSTANCE m_8; // +0x08 = the resource module handle
};
SIZE_UNKNOWN(AppModule_136a30);
AppModule_136a30* AppModule_1d3631(); // RVA 0x1d3631 (global accessor)

// The retail game-global timeGetTime fn-ptr (_g_pTimeGetTime @ 0x6c4650), NOT the
// WINMM import; PurgeVoiceList calls ds:[0x6c4650] - do NOT swap for timeGetTime.
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650

// The DirectSoundMgr clone hierarchy (CloneList / DSoundBaseSub / DSoundCloneInst) is
// defined in DirectSoundMgr.h so the device + feeder can name the concrete leaf.

// ---------------------------------------------------------------------------
// VolumeToAttenuation (static __cdecl, x87): 0..100 volume -> centi-dB attenuation.
// 100->0, 0->-10000, else -acos(pow(1/(v/100),10))/acos(2)*100, floored via __ftol.
// @early-stop
// x87-fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md, 58%): retail spills
// `ratio` to [ebp-8] (push ebp frame + shared jmp epilogue); MSVC5 keeps it in st0
// (frameless) + an extra fxch. Not source-steerable; logic complete.
RVA(0x001350b0, 0x5d)
i32 SoundDevice::VolumeToAttenuation(i32 value) {
    if (value == 100) {
        return 0;
    }
    if (value == 0) {
        return -10000;
    }
    double t = (double)value / c_volScale;
    double ratio = acos(pow(c_volNum / t, c_powExp)) / acos(c_acosNorm);
    return (i32)(-(ratio * c_volScale));
}

// ---------------------------------------------------------------------------
// ConvertVolumeToPercent (0x135110, free __cdecl): DSound centi-dB [-10000..0] ->
// 0..100 linear percent. Returns 100 for a zero input, else r = c_volScale -
// (c_volNum - pow(c_acosNorm, -(|v|/100)/c_powExp)) * c_volScale
// (== 100*2^(-(|v|/100)/10)), floored via __ftol and sign-flipped for non-negative
// inputs. Shares the volume-curve constant pool with VolumeToAttenuation above.
// (Dossier seam re-home: was the mis-homed "ComputeCmdPercent" singleton unit
// gruntcmdpercent, compiled /Odi.)
// @early-stop
// compile-profile wall (33.96%, was 100% under the /Odi singleton crutch): the body
// needs /O1-style codegen (ebp frame retained, idiv-by-100 with no magic-number
// strength reduction, x87 spill schedule) that this TU's real /O2 /GX profile does
// not emit. Logic complete + previously byte-proven; placement per the dossier is
// authoritative (never revert placement) - a per-fn #pragma optimize probe is the
// final-sweep lead.
RVA(0x00135110, 0x8e)
SYMBOL(_ConvertVolumeToPercent)
extern "C" i32 ConvertVolumeToPercent(i32 v) {
    if (v == 0) {
        return 100;
    }
    double d;
    if (v < 0) {
        d = (double)(-v / 100);
    } else {
        d = (double)(v / 100);
    }
    double r = c_volScale - (c_volNum - pow(c_acosNorm, -d / c_powExp)) * c_volScale;
    if (v < 0) {
        return (i32)r;
    }
    return (i32)(-r);
}

// ---------------------------------------------------------------------------
// BuildVolumeTable: g_volumeTable[i] = VolumeToAttenuation(i), i=0..100 (101 stores).
RVA(0x001351a0, 0x23)
void SoundDevice::BuildVolumeTable() {
    for (i32 i = 0; i <= 100; i++) {
        g_volumeTable[i] = VolumeToAttenuation(i);
    }
}

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
// ~DirectSoundMgr - empty base-subobject dtor (just the vptr reset to 0x5ef6b8).
RVA(0x00135300, 0x7)
DirectSoundMgr::~DirectSoundMgr() {}

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
// IsInHardware: gated on init; GetCaps into a zeroed DSBCAPS, report on failure,
// return the DSBCAPS_LOCHARDWARE bit. Same normalize/forward shape as IsPlaying.
// @early-stop
// byte-AND-width wall (99.88%): retail `and al,4` (24 04) vs cl `and eax,4` (83 e0 04)
// on the returned (dwFlags & 4)==4 - identical wall to IsLooping/IsPlaying; permuter
// no-change. Body is byte-exact otherwise (memset-{0} keeps the redundant dwSize zero).
RVA(0x00135490, 0x73)
i32 DirectSoundMgr::IsInHardware() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DSBCAPS caps;
    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(DSBCAPS);
    i32 hr = m_buffer->GetCaps(&caps) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xcc, hr);
        return 0;
    }
    return (caps.dwFlags & DSBCAPS_LOCHARDWARE) == DSBCAPS_LOCHARDWARE;
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
// device is down or the voice allocation/ctor failed. (The DSoundVoice ctor
// 0x136fe0 has no reconstructed body yet - the call reloc-masks.)
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
// GetPanPercent (0x135840): gated on init; read the raw DSound pan via GetPan and
// map it to a signed -100..100 percent through ConvertVolumeToPercent (0x135110):
// right pan (>0) -> 100 - convert(-pan), left pan (<0) -> convert(pan) - 100.
RVA(0x00135840, 0x3b)
i32 DirectSoundMgr::GetPanPercent() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 pan = GetPan();
    if (pan == 0) {
        return 0;
    }
    if (pan > 0) {
        return 100 - ConvertVolumeToPercent(-pan);
    }
    return ConvertVolumeToPercent(pan) - 100;
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
    ((DSoundList*)&m_cloneList)->InsertHead((DSoundLink*)&m_cloneNode);
    m_playKey = 1;
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
    ((DSoundList*)&m_cloneList)->InsertHead((DSoundLink*)&clone->m_cloneNode);
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
    ((DSoundList*)&m_cloneList)->Unlink((DSoundLink*)&clone->m_cloneNode);
    if (clone != this) {
        delete clone;
    }
}

// ---------------------------------------------------------------------------
// CSoundCueMgr::GetItem (0x135d70) - the Dsndmgr sound-cue manager's pooled-buffer
// resolver: walk the +0x58 item list for a live/finished DirectSound buffer,
// reconfigure it (pan/pitch/volume from m_18/m_1c/m_20) and, when none is free,
// Create() a fresh one, then unlink + re-append it to the +0x58 list. (Dossier
// seam re-home from the statusbarmgrgetitem singleton unit; its owner
// CSoundCueMgr was rtti-mislabeled "CStatusBarMgr" - see <Gruntz/SoundCueMgr.h>.)
// @confidence: med
// @source: reloc-correlation (1 caller)
// @early-stop
// shrink-wrapped callee-save push wall (~90%): logic + offsets + externs byte-exact.
// Retail saves only edi at entry and defers `push esi`/`push ebx` past the m_78 null
// guard (the early-out restores just edi); cl pushes all three upfront. Not source-
// steerable; docs/patterns/shrink-wrapped-callee-save-push.md. Final sweep.
RVA(0x00135d70, 0x92)
CStatusBarItem2* CSoundCueMgr::GetItem() {
    if (!m_10->m_78) {
        return 0;
    }
    SBNode* node = m_58.m_head;
    if (node) {
        while (1) {
            if (node->m_8->m_50 && ((DirectSoundMgr*)node->m_8)->IsPlaying() == 0) {
                break;
            }
            node = node->m_0;
            if (!node) {
                break;
            }
        }
    }
    CStatusBarItem2* found;
    if (!node) {
        found = 0;
    } else {
        found = node->m_8;
    }
    if (found) {
        ((DirectSoundMgr*)found)->SetVolume(m_20);
        ((DirectSoundMgr*)found)->SetPan(m_1c);
        ((DirectSoundMgr*)found)->SetFrequency(m_18);
    }
    if (!found) {
        found = Create(1);
        if (!found) {
            return found;
        }
    }
    ((DSoundList*)&m_58)->Unlink((DSoundLink*)&found->m_link44);
    ((DSoundList*)&m_58)->InsertTail((DSoundLink*)&found->m_link44);
    return found;
}

// ---------------------------------------------------------------------------
// LoadFromFile: (optionally fseek to `offset`), Lock the buffer FROMWRITECURSOR for
// `bytes`, fread each wrap region straight from `fp`, then Unlock. Gated on init;
// fseek/fread failures and Lock/Unlock HRESULTs each bail to 0.
RVA(0x00135e10, 0x124)
i32 DirectSoundMgr::LoadFromFile(FILE* fp, u32 bytes, i32 offset) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if (offset != -1) {
        if (fseek(fp, offset, SEEK_SET) != 0) {
            return 0;
        }
    }

    void* p1;
    u32 n1;
    void* p2;
    u32 n2;
    i32 hr =
        m_buffer->Lock(0, bytes, &p1, (LPDWORD)&n1, &p2, (LPDWORD)&n2, DSBLOCK_FROMWRITECURSOR);
    if (hr != 0) {
        GetErrorString(DSNDMGR_FILE, 0x27c, hr);
        return 0;
    }

    if (n1 > 0) {
        if (fread(p1, n1, 1, fp) != 1) {
            return 0;
        }
    }
    if (n2 > 0) {
        if (fread(p2, n2, 1, fp) != 1) {
            return 0;
        }
    }

    hr = m_buffer->Unlock(p1, n1, p2, n2);
    if (hr != 0) {
        GetErrorString(DSNDMGR_FILE, 0x295, hr);
        return 0;
    }
    return 1;
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
// CSoundCueMgr::ConfigureItem (0x1360d0) - the shared cue-configuration helper the
// per-widget status-bar loaders funnel through. Guard on the surface being live
// (m_10->m_78), resolve the pooled buffer via GetItem (0x135d70), then push the
// four cue values through the DirectSoundMgr setters + Play, ANDing the checked
// results (the 4th setter's result is ignored). __thiscall, ret 0x10. (Dossier
// seam re-home from SpriteResource.cpp.)
RVA(0x001360d0, 0x7c)
i32 CSoundCueMgr::ConfigureItem(i32 a0, i32 a1, i32 a2, i32 a3) {
    if (!m_10->m_78) {
        return 0;
    }
    CStatusBarItem2* item = GetItem();
    if (!item) {
        return 0;
    }
    i32 ok = 1;
    if (!((DirectSoundMgr*)item)->SetVolumeByIndex(a0)) {
        ok = 0;
    }
    if (!((DirectSoundMgr*)item)->SetPanByIndex(a1)) {
        ok = 0;
    }
    if (!((DirectSoundMgr*)item)->SetField2(a2)) {
        ok = 0;
    }
    ((DirectSoundMgr*)item)->SetField3(a3);
    if (!((DirectSoundMgr*)item)->Play()) {
        ok = 0;
    }
    return ok;
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
// ctor (/GX EH frame): zero the two list members, stamp vptr, clear init flag,
// BuildVolumeTable, zero device/primary state. SoundStream derives -> base call here.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// retail's /GX frame comes from the fully-constructed object registering ~SoundDevice
// for unwind; MSVC5 emits a frameless body. Body faithful; same family as ~SoundDevice.
RVA(0x00136440, 0x74)
SoundDevice::SoundDevice() {
    // cl auto-stamps ??_7SoundDevice@@6B@ (0x5ef6c4).
    m_bufferList.m_head = 0;
    m_bufferList.m_tail = 0;
    m_voiceList.m_head = 0;
    m_voiceList.m_tail = 0;
    m_initialized = 0;
    BuildVolumeTable();
    m_reacquireProc = 0;
    m_primaryBuffer = 0;
    m_coopLevel = 0;
    m_bufferFlags = 0;
    m_force8Bit = 0;
}

// Slot-0 scalar-deleting dtor (??_G) MSVC synthesizes from the virtual dtor; no source
// body -> pin by mangled name.
// @rva-symbol: ??_GSoundDevice@@UAEPAXI@Z 0x001364c0 0x1e

// ---------------------------------------------------------------------------
// ~SoundDevice (/GX EH frame): cl resets vptr, then if init runs the teardown.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// body byte-exact, but retail's /GX frame comes from the fully-constructed base
// subobject; MSVC5's dtor is frameless. Same wall as ~DirectSoundMgr.
RVA(0x00136500, 0x43)
SoundDevice::~SoundDevice() {
    // cl auto-resets the vptr to ??_7SoundDevice@@6B@ (0x5ef6c4).
    if (m_initialized) {
        Shutdown();
    }
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
    i32 hr = m_device->SetCooperativeLevel((HWND)hwnd, level) != 0;
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
    i32 hr = m_device->SetCooperativeLevel((HWND)hwnd, level) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3cf, hr);
        return 0;
    }
    m_coopLevel = level;
    return 1;
}

// ---------------------------------------------------------------------------
// Compact (0x136650): gated on init; IDirectSound::Compact (slot +0x1c) the device,
// report + fail on a non-zero HRESULT.
RVA(0x00136650, 0x37)
i32 SoundDevice::Compact() {
    if (m_initialized == 0) {
        return 0;
    }
    i32 hr = m_device->Compact() != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3dc, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Shutdown: RemoveBuffer each owned buffer, then Release the primary + device, clear
// the flag. The `head ? node-4 : 0` biased-pointer recovery is language-forced.
RVA(0x00136690, 0x58)
void SoundDevice::Shutdown() {
    if (m_initialized) {
        DSoundCloneInst* node = elemOf<DSoundCloneInst>(m_bufferList.m_head);
        while (node) {
            RemoveBuffer(node);
            node = elemOf<DSoundCloneInst>(m_bufferList.m_head);
        }
        if (m_primaryBuffer) {
            m_primaryBuffer->Release();
        }
        m_device->Release();
    }
    m_initialized = 0;
}

// ---------------------------------------------------------------------------
// CreateBuffer (/GX EH frame): validate PCM fmt, CreateSoundBuffer, RezAlloc+BaseInit a
// DSoundCloneInst leaf, thread on the +0x04 list, seed fmt/avg-bytes/byte-count + duration.
// @early-stop
// RezAlloc+placement-new EH-frame wall (docs/patterns/rezalloc-placement-new-no-eh-frame.md):
// body byte-exact, but retail's `new`-with-RezAlloc-operator-new emits a /GX
// ctor-in-flight frame the RezAlloc+BaseInit path can't reproduce. Same wall as
// SoundStream::CreateStreamBuffer.
RVA(0x001366f0, 0x168)
DirectSoundMgr* SoundDevice::CreateBuffer(WaveFormatX* fmt, u32 bytes, u32 flags) {
    if (m_initialized == 0) {
        return 0;
    }
    if (bytes == 0) {
        return 0;
    }
    if (fmt == 0) {
        return 0;
    }
    if (fmt->wFormatTag != 1) {
        return 0;
    }

    // The 16-byte WAVEFORMATEX copy: retail moves it as dword@0, dword@4, dword@8,
    // dword@0xc, word@0x10 (verified). The two u16-pair fields (wFormatTag|nChannels
    // and nBlockAlign|wBitsPerSample) must be punned to a single dword store -
    // field-by-field would emit two 16-bit moves. Language-forced (verified by disasm).
    WaveFormatX wf;
    *(u32*)&wf.wFormatTag = *(u32*)&fmt->wFormatTag;
    wf.nSamplesPerSec = fmt->nSamplesPerSec;
    wf.nAvgBytesPerSec = fmt->nAvgBytesPerSec;
    *(u32*)&wf.nBlockAlign = *(u32*)&fmt->nBlockAlign;
    wf.cbSize = fmt->cbSize;

    IDirectSoundBuffer* out = 0;
    DSBUFFERDESC desc;
    desc.dwSize = DSBUFFERDESC_SIZE;
    desc.dwFlags = flags;
    desc.dwBufferBytes = bytes;
    desc.dwReserved = 0;
    desc.lpwfxFormat = (LPWAVEFORMATEX)&wf;

    i32 hr = m_device->CreateSoundBuffer(&desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x422, hr);
        return 0;
    }
    if (out == 0) {
        return 0;
    }

    // RezAlloc the 0x60B leaf, then BaseInit (the ctor 0x135b10, reached as a method)
    // stamps its vptr. The buffer's cached format/rate/sample fields (base offsets
    // +0x18/+0x38/+0x3c/+0x2c) are seeded from the wave header here.
    DSoundCloneInst* voice = (DSoundCloneInst*)RezAlloc(0x60);
    if (voice) {
        voice->BaseInit(out, this);
    }
    voice->m_freq = *(u32*)&wf.wFormatTag; // +0x18  format word (wFormatTag|nChannels)
    m_bufferList.InsertHead(voice ? &voice->m_link : 0);
    voice->m_rateBase = fmt->nAvgBytesPerSec;   // +0x38  avg bytes/sec
    voice->m_sampleRate = fmt->nAvgBytesPerSec; // +0x3c  duration divisor
    voice->m_sampleCount = bytes;               // +0x2c  byte count
    voice->ComputeDuration();
    return voice; // DSoundCloneInst* -> DirectSoundMgr* base view (CreateBuffer's return)
}

// ---------------------------------------------------------------------------
// AcquireFile: gated on init; fopen "rb", slurp whole file into a new'd buffer, Acquire
// the RIFF blob, free + close. Returns the wrapper (0 on any I/O failure).
RVA(0x00136860, 0xa9)
DirectSoundMgr* SoundDevice::AcquireFile(char* path, u32 flags, u32 reserved) {
    if (m_initialized == 0) {
        return 0;
    }
    FILE* fp = Eng_fopen(path, s_rb);
    if (fp == 0) {
        return 0;
    }
    u32 size = Eng_filelength(fp->_file);
    void* buf = operator new(size);
    if (RezFRead(buf, size, 1, fp) != 1) {
        RezFClose(fp);
        operator delete(buf);
        return 0;
    }
    RezFClose(fp);
    DirectSoundMgr* wrapper = Acquire(buf, flags, reserved);
    operator delete(buf);
    return wrapper;
}

// ---------------------------------------------------------------------------
// Acquire: parse RIFF/WAVE fmt+data, optionally 16->8 downconvert (m_force8Bit or parse
// flag), CreateBuffer, LockConvert the PCM in; RemoveBuffer on load failure.
// @early-stop
// frame-homing-area-reuse wall (docs/patterns/stack-buffer-size-drives-frame.md, 99.8%):
// every code byte IDENTICAL; retail overlays the ParseFmt out-struct onto dead arg-home
// slots (sub esp,8) while MSVC5 gives fresh stack (sub esp,0x18). Not source-steerable.
RVA(0x00136910, 0x119)
DirectSoundMgr* SoundDevice::Acquire(void* riff, u32, u32) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == 0) {
        return 0;
    }

    ParseFmt po;
    void* data;
    u32 size;
    po.m_reservedC = 0;
    po.m_flags = 0;
    po.m_reservedA = 0;
    if (ParseWaveChunks(riff, &po, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (po.m_flags & 1) == 1) {
        cvt = 1;
    }
    if (po.m_fmt->wBitsPerSample != 0x10 || po.m_fmt->wFormatTag != 1) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        po.m_fmt->wBitsPerSample = 8;
        po.m_fmt->nAvgBytesPerSec >>= 1;
        po.m_fmt->nBlockAlign >>= 1;
    }

    DirectSoundMgr* wrapper = CreateBuffer(po.m_fmt, size, po.m_flags);
    if (wrapper == 0) {
        return 0;
    }
    if (wrapper->LockConvert(data, size, cvt) == 0) {
        RemoveBuffer(wrapper);
        return 0;
    }
    return wrapper;
}

// ---------------------------------------------------------------------------
// AcquireResource (0x136a30): the WAVE Win32-resource sibling of AcquireFile -
// find/load/lock the named "WAVE" resource in the app module, then Acquire the
// RIFF blob. (Dossier seam re-home: was ResLoaders::WaveHost_136a30::LoadWave;
// the +0x78 gate IS m_initialized and the 0x136910 callee IS Acquire.)
RVA(0x00136a30, 0x76)
DirectSoundMgr* SoundDevice::AcquireResource(const char* name, u32 flags, u32 reserved) {
    if (m_initialized == 0) {
        return 0;
    }
    HINSTANCE mod1 = AppModule_1d3631()->m_8;
    HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
    if (!hRsrc) {
        return 0;
    }
    HINSTANCE mod2 = AppModule_1d3631()->m_8;
    HGLOBAL hRes = LoadResource(mod2, hRsrc);
    if (!hRes) {
        return 0;
    }
    void* data = LockResource(hRes);
    if (!data) {
        return 0;
    }
    return Acquire(data, flags, reserved);
}

// ---------------------------------------------------------------------------
// ValidateRestore: gated on init; require size + fmt (non-null) + wFormatTag==1, then
// Restore the buffer and return its 0/1 success.
RVA(0x00136ab0, 0x41)
i32 SoundDevice::ValidateRestore(DirectSoundMgr* buf, WaveFormatX* fmt, u32 size) {
    if (m_initialized == 0) {
        return 0;
    }
    if (size == 0) {
        return 0;
    }
    if (fmt == 0) {
        return 0;
    }
    if (fmt->wFormatTag != 1) {
        return 0;
    }
    return buf->Restore() != 0;
}

// ---------------------------------------------------------------------------
// ReloadFile: fopen a file and re-load it into an EXISTING looping buffer (the
// AcquireFile sibling that funnels through ReloadRiff). Skip (return 1) unless the
// buffer is looping; on any I/O failure return 0.
RVA(0x00136b00, 0xc2)
i32 SoundDevice::ReloadFile(DirectSoundMgr* buf, char* path, u32 reserved) {
    if (m_initialized == 0) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
        return 1;
    }
    FILE* fp = Eng_fopen(path, s_rb);
    if (fp == 0) {
        return 0;
    }
    u32 size = Eng_filelength(fp->_file);
    void* data = operator new(size);
    if (RezFRead(data, size, 1, fp) != 1) {
        RezFClose(fp);
        operator delete(data);
        return 0;
    }
    RezFClose(fp);
    i32 r = ReloadRiff(buf, data, reserved);
    operator delete(data);
    return r;
}

// ---------------------------------------------------------------------------
// ReloadRiff: re-load a RIFF into an EXISTING buffer (Acquire sibling). Gate on init +
// non-null RIFF + buffer looping; parse, optional 16->8 downconvert, Restore, LockConvert.
// @early-stop
// frame-homing-area-reuse wall (docs/patterns/stack-buffer-size-drives-frame.md): same
// family as Acquire (fresh stack vs retail's arg-home overlay). Not source-steerable.
RVA(0x00136bd0, 0x110)
i32 SoundDevice::ReloadRiff(DirectSoundMgr* buf, void* riff, u32 /*reserved*/) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == 0) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
        return 1;
    }

    ParseFmt po;
    void* data;
    u32 size;
    po.m_reservedC = 0;
    po.m_flags = 0;
    po.m_reservedA = 0;
    if (ParseWaveChunks(riff, &po, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (po.m_flags & 1) == 1) {
        cvt = 1;
    }
    if (po.m_fmt->wBitsPerSample != 0x10 || po.m_fmt->wFormatTag != 1) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        po.m_fmt->wBitsPerSample = 8;
        po.m_fmt->nAvgBytesPerSec >>= 1;
        po.m_fmt->nBlockAlign >>= 1;
    }

    if (ValidateRestore(buf, po.m_fmt, size) == 0) {
        return 0;
    }
    return buf->LockConvert(data, size, cvt) != 0;
}

// ---------------------------------------------------------------------------
// ReloadResource (0x136ce0): the WAVE Win32-resource sibling of ReloadFile - if the
// probe buffer is looping, find/load/lock the named "WAVE" resource and ReloadRiff
// it into the buffer. (Dossier seam re-home: was ResLoaders::WaveHost2_136ce0::
// LoadWave; the +0x78 gate IS m_initialized and the 0x136bd0 callee IS ReloadRiff.)
RVA(0x00136ce0, 0x92)
i32 SoundDevice::ReloadResource(DirectSoundMgr* probe, const char* name, u32 reserved) {
    if (m_initialized == 0) {
        return 0;
    }
    if (probe->IsLooping() == 0) {
        return 1;
    }
    HINSTANCE mod1 = AppModule_1d3631()->m_8;
    HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
    if (!hRsrc) {
        return 0;
    }
    HINSTANCE mod2 = AppModule_1d3631()->m_8;
    HGLOBAL hRes = LoadResource(mod2, hRsrc);
    if (!hRes) {
        return 0;
    }
    void* data = LockResource(hRes);
    if (!data) {
        return 0;
    }
    return ReloadRiff(probe, data, reserved);
}

// ---------------------------------------------------------------------------
// RemoveBuffer: reap the buffer's voices (keyed by its address), Release its COM
// buffer, unlink from the owned-buffer list, then scalar-delete it.
RVA(0x00136d80, 0x56)
void SoundDevice::RemoveBuffer(DirectSoundMgr* node) {
    if (m_initialized) {
        // The voices carry the owning buffer's address as their reap key.
        m_voiceList.RemoveMatching(node, 0xffff);
        if (node->m_buffer) {
            node->m_buffer->Release();
            node->m_buffer = 0;
        }
        m_bufferList.Unlink(node ? &node->m_link : 0);
        if (node) {
            delete node;
        }
    }
}

// ---------------------------------------------------------------------------
// StopAll: StopAndRewind + StopAllClones each owned buffer.
RVA(0x00136de0, 0x3c)
void SoundDevice::StopAll() {
    if (m_initialized) {
        DSoundCloneInst* node = elemOf<DSoundCloneInst>(m_bufferList.m_head);
        while (node) {
            node->StopAndRewind();
            node->StopAllClones();
            node = elemOf<DSoundCloneInst>(node->m_link.m_next);
        }
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

// ---------------------------------------------------------------------------
// FreeSamples: walk the voice list; per node run its slot-1 stop, unlink, then
// `delete (PureSoundElem*)node` (pure-base teardown + RezFree). Returns 1.
// @early-stop
// regalloc/early-out scheduling wall (77%): retail reserves all 4 callee-saved regs and
// runs the early-out in-frame; MSVC emits a leaner frameless early-out. Loop byte-exact.
RVA(0x00136ed0, 0x72)
i32 SoundDevice::FreeSamples() {
    if (m_initialized == 0) {
        return 0;
    }
    DSoundElem* node = elemOf<DSoundElem>(m_voiceList.m_head);
    while (node) {
        DSoundLink* n = node->m_link.m_next;
        DSoundElem* next = elemOf<DSoundElem>(n);
        node->Stop(); // slot 1: stop the element before freeing it
        m_voiceList.Unlink(node ? &node->m_link : 0);
        if (node) {
            // pure-base teardown: reset vptr to ??_7PureSoundElem (0x5ef6c8) + RezFree.
            PureSoundElem* pure = node;
            delete pure;
        }
        node = next;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DSoundList::RemoveMatching (0x136f60, __thiscall, 2 stack args) - the reaping
// list helper (the only DSoundList primitive that falls INSIDE this obj; the other
// five live at 0x1390e0+ in SoundVoiceList.cpp). Walk the chain; unlink + free
// every element whose key (@+0x10) equals `key` and whose tag (@+0xc) equals `tag`
// (0xffff is a wildcard). The free is `delete (PureSoundElem*)e`: the base-
// subobject teardown resets the element vptr to the pure base (??_7PureSoundElem =
// 0x5ef6c8) and PureSoundElem::operator delete RezFree's it. The tag-mismatch arm
// does not advance (it re-tests the current element) - retail's structure; the
// elements that reach here never trip it, but the source must reproduce the
// codegen, so spell it as a no-advance `continue`.
// @early-stop
// select-zero-mask-dest-register wall (docs/patterns/select-zero-mask-dest-register.md):
// byte-exact except the `e ? node : 0` mask (neg/sbb/and) lands in edx (ours) vs eax
// (retail) - a free-list pick the four obvious source spellings don't move. 99.3%,
// logic complete; deferred to the final sweep.
RVA(0x00136f60, 0x74)
void DSoundList::RemoveMatching(void* key, u32 tag) {
    DSoundElem* e = elemOf<DSoundElem>(m_head);
    while (e) {
        DSoundLink* node = &e->m_link;
        DSoundLink* n = e->m_link.m_next;
        DSoundElem* next = elemOf<DSoundElem>(n);
        if (tag != 0xffff && e->m_tag != tag) {
            continue;
        }
        if (e->m_key == key) {
            Unlink(e ? node : 0);
            if (e) {
                PureSoundElem* pure = e; // up-cast: teardown resets to the pure base
                delete pure;
            }
        }
        e = next;
    }
}

// ---------------------------------------------------------------------------
// 0x136fe0 - the DSoundVoice 6-arg ctor ("SoundTick_Ctor", 123 B): stamps the
// 0x5ef6d0 vtable + the play params. NOT yet reconstructed (the dossier's one
// remaining in-obj gap); CloneAndPlay's `new DSoundVoice(...)` reloc-masks to it.

// ---------------------------------------------------------------------------
// DSoundVoice::Tick (0x137060, vtbl slot 0, __thiscall, 1 arg = the current
// clock). Clamp the elapsed time to the ramp duration (flagging completion when
// it overruns); if the buffer stopped playing on its own, also flag done.
// Otherwise interpolate the ramp volume index across the elapsed fraction and
// push it through SetVolumeByIndex. On completion, when the stop flag is set,
// stop+rewind the buffer. Returns whether the voice is still live (!done).
// @early-stop
// 95.7% -- regalloc-pinning wall (docs/patterns/zero-register-pinning.md): every
// instruction matches (the unsigned clamp `jb`, the signed idiv interpolation, the
// three buffer calls, the done/!done epilogue). The only residual is the esi<->edi
// coin-flip: retail pins this->esi + elapsed->edi, MSVC5 here pins elapsed->esi +
// this->edi (same values, mirrored register file). Logic complete.
RVA(0x00137060, 0x6b)
i32 DSoundVoice::Tick(i32 now) {
    i32 done = 0;
    i32 elapsed = now - m_rampStartTime;
    if ((u32)elapsed >= (u32)m_rampDurationMs) {
        elapsed = m_rampDurationMs;
        done = 1;
    }
    if (m_buffer->IsPlaying() == 0) {
        done = 1;
    } else {
        i32 vol =
            (m_rampEndVolume - m_rampStartVolume) * elapsed / m_rampDurationMs + m_rampStartVolume;
        m_buffer->SetVolumeByIndex(vol);
    }
    if (done && m_stopAndRewind != 0) {
        m_buffer->StopAndRewind();
    }
    return done == 0;
}

// ---------------------------------------------------------------------------
// DSoundVoice::Stop (0x1370d0, vtbl slot 1, __thiscall, no args). If the buffer
// is still playing: when the stop flag is set, stop+rewind it; otherwise snap its
// volume to the ramp end. Always returns 1.
RVA(0x001370d0, 0x38)
i32 DSoundVoice::Stop() {
    if (m_buffer->IsPlaying() != 0) {
        if (m_stopAndRewind != 0) {
            m_buffer->StopAndRewind();
            return 1;
        }
        m_buffer->SetVolumeByIndex(m_rampEndVolume);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ParseWaveChunks (__cdecl): verify 'RIFF'/'WAVE', walk even-aligned chunks, record the
// 'fmt ' payload into out->m_fmt and 'data' ptr/len into *dataOut/*sizeOut; nonzero when
// 'fmt ' seen before 'data'.
// @early-stop
// add-fold scheduling wall (98.2%): byte-identical except the per-chunk cursor advance -
// source `p += 2` -> two `add $4` (retail) vs one `add $8` (MSVC5 /O2 fold). Not steerable.
RVA(0x00137110, 0x8d)
SYMBOL(_ParseWaveChunks)
extern "C" i32 ParseWaveChunks(void* riff, ParseFmt* out, void** dataOut, u32* sizeOut) {
    u32* p = (u32*)((char*)riff + 4);
    u32 riffSize = *p;
    p++;
    u32 waveTag = *p;
    p++;
    char* end = (char*)p + riffSize - 4;
    if (*(u32*)riff != WAVE_FOURCC('R', 'I', 'F', 'F')) {
        return 0;
    }
    if (waveTag != WAVE_FOURCC('W', 'A', 'V', 'E')) {
        return 0;
    }
    out->m_fmt = 0;
    *dataOut = 0;
    while ((char*)p < end) {
        u32 id = p[0];
        u32 size = p[1];
        p += 2;
        if (id == WAVE_FOURCC('f', 'm', 't', ' ')) {
            out->m_fmt = (WaveFormatX*)p;
        } else if (id == WAVE_FOURCC('d', 'a', 't', 'a')) {
            *dataOut = p;
            *sizeOut = size;
            return out->m_fmt != 0;
        }
        p = (u32*)((char*)p + ((size + 1) & ~1));
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SetPrimaryFormat: ensure the primary buffer exists, then SetFormat; report + bail.
RVA(0x001371a0, 0x5a)
i32 SoundDevice::SetPrimaryFormat(void* fmt) {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_primaryBuffer->SetFormat((LPWAVEFORMATEX)fmt) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x678, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// StartPrimary (0x137200): gated on init, lazily (re)create the primary buffer,
// then start it looping (IDirectSoundBuffer::Play slot 12, DSBPLAY_LOOPING);
// report + fail on a non-zero HRESULT.
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
// GetPrimary (0x137300): gated on init, lazily (re)create the primary buffer, then
// return it. The device-getter sibling of StartPrimary/CreatePrimaryBuffer. LAST
// function of the DSNDMGR.CPP obj (the 0x137330 file boundary follows; DSndMgSR.cpp
// = src/Dsndmgr/SoundStream.cpp).
RVA(0x00137300, 0x23)
IDirectSoundBuffer* SoundDevice::GetPrimary() {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    return m_primaryBuffer;
}
