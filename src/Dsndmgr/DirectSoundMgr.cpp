#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundVoiceList.h>
#include <Dsndmgr/DSoundVoice.h> // the 0x28-byte voice node CloneAndPlay news
#include <Dsndmgr/SoundDevice.h> // SoundDevice: the owning device (m_owner) + its methods
#include <Rez/RezMgr.h>          // RezAlloc/RezFree/RezFRead/RezFClose - the engine heap/file layer
#include <Win32.h>               // windows.h base types (dsound.h needs them)
#include <mmsystem.h>            // WAVEFORMATEX (dsound.h needs it predefined)
#include <dsound.h> // real DirectSound SDK (IDirectSound/Buffer, DSBUFFERDESC, DSBCAPS)
#include <Dsndmgr/WaveFormatPtr.h> // the engine record / SDK LPWAVEFORMATEX pair
#include <rva.h>
#include <Pix16.h>  // the byte-cursor unions (RecordBytes / Pix16Ptr)
#include <math.h>   // acos / pow (intrinsic __CIacos / __CIpow) in the volume curves
#include <stdio.h>  // engine sprintf (reloc-masked); FILE + fopen/fread/fclose (RIFF loaders)
#include <io.h>     // _filelength (0x18c480) - the RIFF file-size query
#include <string.h> // inline strcpy/memcpy (rep movs / repne scasb)

#include <Wap32/Wap32.h> // ex Globals.h

DATA(0x001ef6b0)
const double c_acosNorm = 2.0; // 0x5ef6b0  acos() normalizer arg

DATA(0x001ef6a8)
const double c_powExp = 10.0; // 0x5ef6a8  pow() exponent

DATA(0x001ef6a0)
const double c_volNum = 1.0; // 0x5ef6a0  numerator of the reciprocal

DATA(0x001ef698)
const double c_volScale = 100.0; // 0x5ef698  v / c_volScale, and the final * c_volScale

#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

typedef enum DSoundDx5Magic {
    DSB_RETAIL_LOOPBIT = 0x02, // retail IsLooping status mask (DX6 DSBSTATUS_LOOPING is 0x04)
    DSBUFFERDESC_SIZE = 0x14,  // retail sizeof(DSBUFFERDESC) (DX6 grew the struct)
} DSoundDx5Magic;

VTBL(DirectSoundMgr, 0x001ef6b8);  // cl-emitted ??_7DirectSoundMgr@@6B@ (base subobject dtor)
VTBL(DSoundCloneInst, 0x001ef6bc); // cl-emitted ??_7DSoundCloneInst@@6B@
VTBL(DSoundBaseSub, 0x001ef6c0);   // cl-emitted ??_7DSoundBaseSub@@6B@
VTBL(SoundDevice, 0x001ef6c4);     // cl-emitted ??_7SoundDevice@@6B@ (virtual dtor)
VTBL(PureSoundElem, 0x001ef6c8);   // 2 __purecall slots (Tick/Stop); the reap-teardown
VTBL(DSoundVoice, 0x001ef6d0);     // cl-emitted ??_7DSoundVoice@@6B@ (Tick/Stop/+137630)
DATA(0x00253ab8)
i32 g_volumeTable[100];
DATA(0x00253c48)
i32 g_panTable[8]; // 0x653c48 (0x20 B; up to g_activeGameWnd@0x653c68)

// "rb" open-mode string the loader passes fopen (.data, rva 0x20b668). clang mangles the
// const-char[] extern's storage class `Q` where cl 5.0 emits `P` (?s_rb@@3PBDB);
// labels.py applies that rewrite itself, authority-checked against the base obj.
DATA(0x0020b668)
const char s_rb[] = "rb";

// ---------------------------------------------------------------------------
// The two volume-curve functions are compiled UNOPTIMIZED in retail while the rest
// of this TU (starting with BuildVolumeTable right after them) is /O2 - so the
// original source wraps exactly this pair in a pragma optimize off/on region.
// Proof, from the retail bytes: both keep an ebp frame with a shared `jmp` epilogue,
// both reload the incoming parameter off [ebp+8] at every use instead of caching it,
// ConvertVolumeToPercent divides by 100 with `mov ecx,0x64 / idiv ecx` (no magic-
// number strength reduction, which /O1 and /O2 both always apply), and every x87
// intermediate is spilled to [ebp-N]. BuildVolumeTable at 0x1351a0 is frameless with
// a strength-reduced esi/edi induction pair, i.e. fully optimized.
#pragma optimize("", off)

// ---------------------------------------------------------------------------
// VolumeToAttenuation (static __cdecl, x87): 0..100 volume -> centi-dB attenuation.
// 100->0, 0->-10000, else -acos(pow(1/(v/100),10))/acos(2)*100, floored via __ftol.
RVA(0x001350b0, 0x5d)
i32 SoundDevice::VolumeToAttenuation(i32 value) {
    if (value == 100) {
        return 0;
    }
    if (value == 0) {
        return -10000;
    }
    // One expression, no `t` local: retail divides in place (`fild / fdiv c_volScale
    // / fdivr c_volNum`) and only `ratio` gets a stack slot ([ebp-8]).
    double ratio = acos(pow(c_volNum / (value / c_volScale), c_powExp)) / acos(c_acosNorm);
    return static_cast<i32>((-(ratio * c_volScale)));
}

// ---------------------------------------------------------------------------
// ConvertVolumeToPercent (0x135110, free __cdecl): DSound centi-dB [-10000..0] ->
// 0..100 linear percent. Returns 100 for a zero input, else r = c_volScale -
// (c_volNum - pow(c_acosNorm, -(|v|/100)/c_powExp)) * c_volScale
// (== 100*2^(-(|v|/100)/10)), floored via __ftol and sign-flipped for non-negative
// inputs. Shares the volume-curve constant pool with VolumeToAttenuation above.
// (Dossier seam re-home: was the mis-homed "ComputeCmdPercent" singleton unit
// gruntcmdpercent, compiled /Odi.)
RVA(0x00135110, 0x8e)
i32 ConvertVolumeToPercent(i32 v) {
    if (v == 0) {
        return 100;
    }
    double d;
    if (v < 0) {
        d = static_cast<double>((-v / 100));
    } else {
        d = static_cast<double>((v / 100));
    }
    double r = c_volScale - (c_volNum - pow(c_acosNorm, -d / c_powExp)) * c_volScale;
    if (v < 0) {
        return static_cast<i32>(r);
    }
    return static_cast<i32>((-r));
}

#pragma optimize("", on)

RVA(0x001351a0, 0x23)
void SoundDevice::BuildVolumeTable() {
    for (i32 i = 0; i <= 100; i++) {
        g_volumeTable[i] = VolumeToAttenuation(i);
    }
}

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

// 0x1352e0 - ??_GDirectSoundMgr: the auto-emitted scalar-deleting dtor (slot 0 of
// the 1-slot ??_7DirectSoundMgr @0x5ef6b8). Was a FID ??_G__non_rtti_object false
// positive in config/library_labels.csv (its inner call targets ~DirectSoundMgr
// @0x135300, not the CRT).
RVA_COMPGEN(0x001352e0, 0x1e, ??_GDirectSoundMgr@@UAEPAXI@Z)

RVA(0x00135300, 0x7)
DirectSoundMgr::~DirectSoundMgr() {}

RVA(0x00135310, 0x2a)
i32 DirectSoundMgr::Restore() {
    i32 hr = m_buffer->Restore() != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x7b, hr);
        return 0;
    }
    return 1;
}

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
// The bit test is an `if`-statement, not a `return <expr>` - only the statement
// form makes cl narrow the mask to `and al,1` over the dword load
// (docs/patterns/dword-load-byte-and-mix.md).
RVA(0x001353f0, 0x4b)
i32 DirectSoundMgr::IsPlaying() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DWORD status;
    i32 hr = m_buffer->GetStatus(&status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xac, hr);
        return 0;
    }
    if ((status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// IsLooping: GetStatus, report on failure, return the "looping" bit.
RVA(0x00135440, 0x4d)
i32 DirectSoundMgr::IsLooping() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DWORD status;
    i32 hr = m_buffer->GetStatus(&status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xbb, hr);
        return 0;
    }
    if ((status & DSB_RETAIL_LOOPBIT) == DSB_RETAIL_LOOPBIT) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// IsInHardware: gated on init; GetCaps into a zeroed DSBCAPS, report on failure,
// return the DSBCAPS_LOCHARDWARE bit. Same normalize/forward shape as IsPlaying.
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
    if ((caps.dwFlags & DSBCAPS_LOCHARDWARE) == DSBCAPS_LOCHARDWARE) {
        return 1;
    }
    return 0;
}

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

RVA(0x001355c0, 0x23)
i32 DirectSoundMgr::SetVolumeByIndex(i32 idx) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return SetVolume(g_volumeTable[idx]);
}

RVA(0x001355f0, 0x42)
i32 DirectSoundMgr::GetVolume() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    long vol;
    i32 hr = m_buffer->GetVolume(&vol) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x10e, hr);
        return 0;
    }
    return vol;
}

RVA(0x00135640, 0x1c)
i32 DirectSoundMgr::GetVolumePercent() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return ConvertVolumeToPercent(GetVolume());
}

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

RVA(0x001357f0, 0x42)
i32 DirectSoundMgr::GetPan() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    long pan;
    i32 hr = m_buffer->GetPan(&pan) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x15e, hr);
        return 0;
    }
    return pan;
}

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

RVA(0x00135920, 0x80)
i32 DirectSoundMgr::SetField2(i32 pct) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 v = pct * static_cast<i32>(m_freq) / 100 + static_cast<i32>(m_freq);
    if (static_cast<u32>(v) >= 0x186a0) {
        v = 0x1869f;
    }
    if (static_cast<u32>(v) <= 0x64) {
        v = 0x65;
    }
    i32 r = SetFrequency(v);
    m_sampleRate = pct * m_rateBase / 100 + m_rateBase;
    ComputeDuration();
    return r;
}

RVA(0x001359a0, 0x18)
void DirectSoundMgr::ComputeDuration() {
    m_durationMs = m_sampleCount * 1000 / m_sampleRate;
}

RVA(0x001359c0, 0x54)
i32 DirectSoundMgr::Unlock(void* audioPtr1, u32 audioBytes1, void* audioPtr2, u32 audioBytes2) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1bb, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135a20, 0x4a)
i32 DirectSoundMgr::GetCurrentPosition(DWORD* play, DWORD* write) {
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

RVA(0x00135ac0, 0x4f)
i32 DirectSoundMgr::GetFormat(void* fmt, u32 size, DWORD* written) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->GetFormat(static_cast<LPWAVEFORMATEX>(fmt), size, written) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1e2, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DSoundCloneInst ctor 0x135b10: chain DSoundBaseSub base ctor, init empty clone list,
// seed head with m_cloneNode, stamp m_playKey; cl stamps 0x5ef6bc.
// The empty clone list is DSoundList's own default ctor (m_cloneList runs between the
// base ctor and the vptr stamp, /GX state 0 -> 1); spelling it in the body would put
// both stores AFTER the stamp.
RVA(0x00135b10, 0x6b)
DSoundCloneInst::DSoundCloneInst(IDirectSoundBuffer* buf, SoundDevice* owner)
    : DSoundBaseSub(buf, owner) {
    // cl auto-stamps ??_7DSoundCloneInst@@6B@ (0x5ef6bc) here.
    ((&m_cloneList))->InsertHead(&m_cloneNode);
    m_playKey = 1;
}

// 0x135b80 - ??_GDSoundCloneInst: the auto-emitted scalar-deleting dtor (slot 0
// of ??_7DSoundCloneInst @0x5ef6bc; inner call -> ~DSoundCloneInst @0x135bb0).
// Was a FID ??_G__non_rtti_object false positive.
RVA_COMPGEN(0x00135b80, 0x1e, ??_GDSoundCloneInst@@UAEPAXI@Z)

// ---------------------------------------------------------------------------
// ~DSoundCloneInst: drain the clone list via RemoveClone (re-reads head each pass);
// cl stamps the clone vptr + chains ~DSoundBaseSub; /GX EH frame.
RVA(0x00135bb0, 0x63)
DSoundCloneInst::~DSoundCloneInst() {
    while (m_cloneList.m_head != 0) {
        RemoveClone(static_cast<CloneNode*>(m_cloneList.m_head)->m_inst);
    }
}

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
    ((&m_cloneList))->InsertHead(&clone->m_cloneNode);
    clone->m_playKey = a;
    return clone;
}

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
    ((&m_cloneList))->Unlink(&clone->m_cloneNode);
    if (clone != this) {
        delete clone;
    }
}

// ---------------------------------------------------------------------------
// DSoundCloneInst::GetItem (0x135d70) - the pooled-clone cue-play path (the ex
// resolver: walk the +0x58 item list for a live/finished DirectSound buffer,
// reconfigure it (pan/pitch/volume from m_18/m_1c/m_20) and, when none is free,
// Create() a fresh one, then unlink + re-append it to the +0x58 list. (Dossier
// seam re-home from the statusbarmgrgetitem singleton unit; its owner
// "DSoundCloneInst" view of this class; it was also rtti-mislabeled "CStatusBarMgr".)
// @confidence: med
// @source: reloc-correlation (1 caller)
// @early-stop
// shrink-wrapped callee-save push wall (90.31), in the OPPOSITE direction to its
// sibling SoundDevice::FreeSamples: here RETAIL shrink-wraps (only edi at entry,
// `push esi`/`push ebx` deferred past the m_78 null guard, the early-out restoring
// just edi) and cl pushes all three up front. Logic + offsets + externs byte-exact.
// A 4-cell matrix over the guard (config/axes/getitem.json: `!x`, `x == 0`, an owner
// local, the node declared above the guard) scored ALL FOUR identical at 90.31 - the
// site is not the lever. That the two functions need OPPOSITE shrink-wrap decisions
// from the same guard shape is why this is the allocator, not the source.
RVA(0x00135d70, 0x92)
DirectSoundMgr* DSoundCloneInst::GetItem() {
    if (!m_owner->m_initialized) {
        return 0;
    }
    CloneNode* node = static_cast<CloneNode*>(m_cloneList.m_head);
    if (node) {
        while (1) {
            if (node->m_inst->m_playKey && node->m_inst->IsPlaying() == 0) {
                break;
            }
            node = static_cast<CloneNode*>(node->m_next);
            if (!node) {
                break;
            }
        }
    }
    DirectSoundMgr* found;
    if (!node) {
        found = 0;
    } else {
        found = node->m_inst;
    }
    if (found) {
        found->SetVolume(m_volume);
        found->SetPan(m_pan);
        found->SetFrequency(m_freq);
    }
    if (!found) {
        found = Clone(1);
        if (!found) {
            return found;
        }
    }
    ((&m_cloneList))->Unlink(&found->m_cloneNode);
    ((&m_cloneList))->InsertTail(&found->m_cloneNode);
    return found;
}

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

    void* audioPtr1;
    DWORD audioBytes1;
    void* audioPtr2;
    DWORD audioBytes2;
    i32 hr = m_buffer->Lock(
        0,
        bytes,
        &audioPtr1,
        &audioBytes1,
        &audioPtr2,
        &audioBytes2,
        DSBLOCK_FROMWRITECURSOR
    );
    if (hr != 0) {
        GetErrorString(DSNDMGR_FILE, 0x27c, hr);
        return 0;
    }

    if (audioBytes1 > 0) {
        if (fread(audioPtr1, audioBytes1, 1, fp) != 1) {
            return 0;
        }
    }
    if (audioBytes2 > 0) {
        if (fread(audioPtr2, audioBytes2, 1, fp) != 1) {
            return 0;
        }
    }

    hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);
    if (hr != 0) {
        GetErrorString(DSNDMGR_FILE, 0x295, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135f40, 0x169)
i32 DirectSoundMgr::LockConvert(void* src, u32 lockBytes, u32 convert) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }

    void* audioPtr1;
    void* audioPtr2;
    DWORD audioBytes1;
    DWORD audioBytes2;
    i32 hr = m_buffer->Lock(
                 0,
                 lockBytes,
                 &audioPtr1,
                 &audioBytes1,
                 &audioPtr2,
                 &audioBytes2,
                 DSBLOCK_ENTIREBUFFER
             )
             != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x2bd, hr);
        return 0;
    }

    if (convert == 0) {
        // Plain byte copy of each region.
        if (audioBytes1 > 0) {
            memcpy(audioPtr1, src, audioBytes1);
        }
        if (audioBytes2 > 0) {
            memcpy(audioPtr2, static_cast<char*>(src) + audioBytes1, audioBytes2);
        }
    } else {
        // 16-bit signed -> 8-bit unsigned downconversion, per region.
        if (audioBytes1 > 0) {
            char* d = static_cast<char*>(audioPtr1);
            i16* s = static_cast<i16*>(src);
            char* end = static_cast<char*>(audioPtr1) + audioBytes1;
            while (d < end) {
                *d = static_cast<char>((static_cast<u32>((*s + 0x8000)) >> 8));
                ++s;
                ++d;
            }
        }
        if (audioBytes2 > 0) {
            char* d = static_cast<char*>(audioPtr2);
            // second half of a wrapped ring copy: audioBytes1 is a RUNTIME byte length from the
            // DirectSound lock, so the 16-bit source is stepped by a BYTE count - both
            // readings of the cursor are named (<Pix16.h>)
            Pix16Ptr half2;
            half2.m_chars = (static_cast<char*>(src) + audioBytes1);
            i16* s = half2.m_swords;
            char* end = static_cast<char*>(audioPtr2) + audioBytes2;
            while (d < end) {
                *d = static_cast<char>((static_cast<u32>((*s + 0x8000)) >> 8));
                ++s;
                ++d;
            }
        }
    }

    hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x2e1, hr);
        return 0;
    }
    return 1;
}

RVA(0x001360d0, 0x7e)
i32 DSoundCloneInst::ConfigureItem(i32 vol, i32 pan, i32 freqPct, i32 loop) {
    if (!m_owner->m_initialized) {
        return 0;
    }
    DirectSoundMgr* item = GetItem();
    if (!item) {
        return 0;
    }
    i32 ok = 1;
    if (!item->SetVolumeByIndex(vol)) {
        ok = 0;
    }
    if (!item->SetPanByIndex(pan)) {
        ok = 0;
    }
    if (!item->SetField2(freqPct)) {
        ok = 0;
    }
    item->SetField3(loop);
    if (!item->Play()) {
        ok = 0;
    }
    return ok;
}

RVA(0x00136150, 0x22)
void DSoundCloneInst::StopAllClones() {
    if (m_owner->m_initialized == 0) {
        return;
    }
    for (CloneNode* node = static_cast<CloneNode*>(m_cloneList.m_head); node != 0;
         node = static_cast<CloneNode*>(node->m_next)) {
        node->m_inst->StopAndRewind();
    }
}

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

// 0x136210 - ??_GDSoundBaseSub: the auto-emitted scalar-deleting dtor (slot 0 of
// ??_7DSoundBaseSub @0x5ef6c0; inner call -> ~DSoundBaseSub @0x136260). Was a FID
// ??_G__non_rtti_object false positive.
RVA_COMPGEN(0x00136210, 0x1e, ??_GDSoundBaseSub@@UAEPAXI@Z)

RVA(0x00136230, 0x2d)
DSoundBaseSub::DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner)
    : DirectSoundMgr(buf, owner) {
    // cl auto-stamps ??_7DSoundBaseSub@@6B@ (0x5ef6c0) here.
    m_cloneNode.m_inst = this;
    m_reacquireOwner = this;
    m_playKey = 1;
}

RVA(0x00136260, 0xb)
DSoundBaseSub::~DSoundBaseSub() {}

// The reacquire-failure path FALLS INTO the shared `return 0` that also ends the
// non-BUFFERLOST error arm (retail's `je 0x1362f0` lands on the `xor eax,eax` the
// GetErrorString(0x356) arm falls through to). Spelling the reacquire gate
// positively is what merges those two epilogues instead of inlining one each.
RVA(0x00136270, 0x8b)
i32 DirectSoundMgr::Play() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Play(0, 0, m_playFlags) != 0;
    if (hr != 0) {
        if (hr == DSERR_BUFFERLOST) {
            if (m_reacquireOwner->ReacquireBuffer() != 0) {
                i32 hr2 = m_buffer->Play(0, 0, m_playFlags) != 0;
                if (hr2 == 0) {
                    return 1;
                }
                GetErrorString(DSNDMGR_FILE, 0x34c, hr2);
                return 0;
            }
        } else {
            GetErrorString(DSNDMGR_FILE, 0x356, hr);
        }
        return 0;
    }
    return 1;
}

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
// The `return 1` is the function's LAST statement and both success exits fall into
// it (retail's 0x136430 block is the final one). The reacquire gate is the POSITIVE
// form with a bare `else { return 0; }` so that exit is a plain `xor eax,eax` block
// cl can cross-jump onto the 0x386 report's tail (0x136427) - the `if (... == 0)
// return 0;` early-out instead let cl notice eax was already 0 and emit its own
// ret, which is the DUP-EXIT `sema disasm --branches --diff` reports.
RVA(0x00136370, 0xcc)
i32 DirectSoundMgr::Lock(
    u32 off,
    u32 bytes,
    void** audioPtr1,
    DWORD* audioBytes1,
    void** audioPtr2,
    DWORD* audioBytes2,
    u32 flags
) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Lock(off, bytes, audioPtr1, audioBytes1, audioPtr2, audioBytes2, flags) != 0;
    if (hr != 0) {
        // `hr` was normalized to 0/1 above, so this compare can never be true - the
        // retail bytes carry the same dead test (0x1363bd `cmp eax,0x88780096` sits
        // AFTER the neg/sbb/neg), so it is the original code's bug, not ours.
        if (hr == DSERR_BUFFERLOST) {
            if (m_reacquireOwner->ReacquireBuffer() != 0) {
                hr = m_buffer
                         ->Lock(off, bytes, audioPtr1, audioBytes1, audioPtr2, audioBytes2, flags)
                     != 0;
                if (hr != 0) {
                    GetErrorString(DSNDMGR_FILE, 0x37c, hr);
                    return 0;
                }
            } else {
                return 0;
            }
        } else {
            GetErrorString(DSNDMGR_FILE, 0x386, hr);
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ctor (/GX EH frame): the two DSoundList members construct themselves empty (their
// own default ctor, /GX state -1 -> 0 -> 1), THEN cl stamps the vptr, then the body
// clears the init flag, builds the volume table and zeroes the device/primary state.
// SoundStream derives -> base call here.
RVA(0x00136440, 0x74)
SoundDevice::SoundDevice() {
    // cl auto-stamps ??_7SoundDevice@@6B@ (0x5ef6c4) after the two member ctors.
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
RVA_COMPGEN(0x001364c0, 0x1e, ??_GSoundDevice@@UAEPAXI@Z)

// ---------------------------------------------------------------------------
// ~SoundDevice (/GX EH frame): cl resets vptr, then if init runs the teardown.
RVA(0x00136500, 0x43)
SoundDevice::~SoundDevice() {
    // cl auto-resets the vptr to ??_7SoundDevice@@6B@ (0x5ef6c4).
    if (m_initialized) {
        Shutdown();
    }
}

RVA(0x00136550, 0x8c)
i32 SoundDevice::Create(void* hwnd, u32 level, u32 flags) {
    i32 created = DirectSoundCreate(0, &m_device, 0) != 0;
    if (created) {
        return 0;
    }
    i32 hr = m_device->SetCooperativeLevel(static_cast<HWND>(hwnd), level) != 0;
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

RVA(0x001365e0, 0xf)
i32 SoundDevice::ReacquireViaCallback() {
    if (m_reacquireProc != 0) {
        return (this->*m_reacquireProc)();
    }
    return 0;
}

RVA(0x001365f0, 0x57)
i32 SoundDevice::SetCooperativeLevel(void* hwnd, u32 level) {
    if (m_initialized == 0) {
        return 0;
    }
    i32 hr = m_device->SetCooperativeLevel(static_cast<HWND>(hwnd), level) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3cf, hr);
        return 0;
    }
    m_coopLevel = level;
    return 1;
}

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
// CreateBuffer (/GX EH frame): validate PCM fmt, CreateSoundBuffer, construct a
// DSoundCloneInst leaf, thread it on the +0x04 list, and seed its format,
// avg-bytes, byte-count, and duration.
// @early-stop
// ONE exit, and it must stay a `goto done`: retail's six gates each emit
// `xor eax,eax / jmp 0x136843` into a single fs:0-restoring epilogue (1 ret). Six
// `return 0;` instead MEASURED 61.35 -> 35.61 - under /GX cl5 duplicates the WHOLE
// epilogue (fs:0 restore + pops + add esp) at every return, giving 7 rets. The
// per-gate `xor eax,eax` is cl's own return-value materialization into the shared
// exit, NOT evidence of a per-gate return (re-confirmed 2026-08-01; the earlier
// 35.6 -> 53.5 note recorded the same experiment). docs/patterns/
// positive-gate-enables-shrink-wrap.md (shared-exit half). Residual is the
// callee-saved count (same family as SoundStream::CreateStreamBuffer).
RVA(0x001366f0, 0x168)
DSoundCloneInst* SoundDevice::CreateBuffer(WaveFormatX* fmt, u32 bytes, u32 flags) {
    WaveFormatX wf;
    IDirectSoundBuffer* out;
    DSBUFFERDESC desc;
    i32 hr;
    // ONE exit: retail's six gates each emit `xor eax,eax / jmp 0x136843` into a
    // single fs:0-restoring epilogue, i.e. the result is a variable returned once,
    // not a second `return 0` with its own inlined unwind.
    DSoundCloneInst* voice = 0;

    if (m_initialized == 0) {
        goto done;
    }
    if (bytes == 0) {
        goto done;
    }
    if (fmt == 0) {
        goto done;
    }
    if (fmt->wFormatTag != 1) {
        goto done;
    }

    // The 16-byte header copy: retail moves it as dword@0, dword@4, dword@8, dword@0xc,
    // word@0x10 (verified). The two u16 PAIRS move as single dwords - which WaveFormatX
    // now names (m_formatWord / m_blockWord), so this needs no pun.
    wf.m_formatWord = fmt->m_formatWord;
    wf.nSamplesPerSec = fmt->nSamplesPerSec;
    wf.nAvgBytesPerSec = fmt->nAvgBytesPerSec;
    wf.m_blockWord = fmt->m_blockWord;
    wf.cbSize = fmt->cbSize;

    out = 0;
    desc.dwSize = DSBUFFERDESC_SIZE;
    desc.dwFlags = flags;
    desc.dwBufferBytes = bytes;
    desc.dwReserved = 0;
    WaveFormatPtr fmtPtr;
    fmtPtr.m_rec = &wf;
    desc.lpwfxFormat = fmtPtr.m_sdk;

    hr = m_device->CreateSoundBuffer(&desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x422, hr);
        goto done;
    }
    if (out == 0) {
        goto done;
    }

    // Global operator new is RezAlloc; the constructor call gives MSVC the
    // retail ctor-in-flight /GX state and stamps the leaf vptr.
    voice = new DSoundCloneInst(out, this);
    voice->m_freq = wf.m_formatWord; // +0x18 (retail 0x136808 `mov edx,[esp+0x14]`)
    m_bufferList.InsertHead(voice ? &voice->m_link : 0);
    voice->m_rateBase = fmt->nAvgBytesPerSec;   // +0x38  avg bytes/sec
    voice->m_sampleRate = fmt->nAvgBytesPerSec; // +0x3c  duration divisor
    voice->m_sampleCount = bytes;               // +0x2c  byte count
    voice->ComputeDuration();
done:
    return voice; // DSoundCloneInst* -> DirectSoundMgr* base view (CreateBuffer's return)
}

RVA(0x00136860, 0xa9)
DSoundCloneInst* SoundDevice::AcquireFile(char* path, u32 flags, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    FILE* fp = fopen(path, s_rb);
    if (fp == 0) {
        return 0;
    }
    u32 size = _filelength(fp->_file);
    void* buf = operator new(size);
    if (fread(buf, size, 1, fp) != 1) {
        fclose(fp);
        operator delete(buf);
        return 0;
    }
    fclose(fp);
    DSoundCloneInst* wrapper = Acquire(buf, flags, loadOpts);
    operator delete(buf);
    return wrapper;
}

// ---------------------------------------------------------------------------
// Acquire: parse RIFF/WAVE fmt+data, optionally 16->8 downconvert (m_force8Bit or the
// caller's loadOpts bit 0), CreateBuffer, LockConvert the PCM in; RemoveBuffer on load
// failure. The parser's three out-params are three separate locals (the ex ParseFmt
// 3-dword "struct" was a view): retail passes &fmt / &data / &size, two of which MSVC5
// overlays on the dead `riff`/arg homes, and the flag word CreateBuffer gets is
// Acquire's OWN `flags` parameter ([esp+0x1c]), while the downconvert request is
// `loadOpts & 1` ([esp+0x20]) - neither ever came out of the parser.
RVA(0x00136910, 0x119)
DSoundCloneInst* SoundDevice::Acquire(void* riff, u32 flags, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == 0) {
        return 0;
    }

    // decl order sets the frame (fresh slots top-down, then the dead `riff` home);
    // the zero stores follow the ASSIGNMENT order, which retail emits fmt/data/size.
    void* data;
    u32 size;
    WaveFormatX* fmt;
    fmt = 0;
    data = 0;
    size = 0;
    if (ParseWaveChunks(riff, &fmt, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (loadOpts & 1) == 1) {
        cvt = 1;
    }
    if (fmt->wBitsPerSample != 0x10 || fmt->wFormatTag != 1) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        fmt->wBitsPerSample = 8;
        fmt->nAvgBytesPerSec >>= 1;
        fmt->nBlockAlign >>= 1;
    }

    DSoundCloneInst* wrapper = CreateBuffer(fmt, size, flags);
    if (wrapper == 0) {
        return 0;
    }
    if (wrapper->LockConvert(data, size, cvt) == 0) {
        RemoveBuffer(wrapper);
        return 0;
    }
    return wrapper;
}

RVA(0x00136a30, 0x76)
DSoundCloneInst* SoundDevice::AcquireResource(const char* name, u32 flags, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    // afxCurrentInstanceHandle - MFC's own documented spelling for this (AFXWIN.H:3880).
    // The inline AfxGetInstanceHandle() needs <afxwin.h>, whose inlines clang rejects in
    // this TU (the _AFX_ENABLE_INLINES guard); a NAMED member of MFC's own struct, not a
    // positional reach.
    HINSTANCE mod1 = AfxGetModuleState()->m_hCurrentInstanceHandle;
    HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
    if (!hRsrc) {
        return 0;
    }
    HINSTANCE mod2 = AfxGetModuleState()->m_hCurrentInstanceHandle;
    HGLOBAL hRes = LoadResource(mod2, hRsrc);
    if (!hRes) {
        return 0;
    }
    void* data = LockResource(hRes);
    if (!data) {
        return 0;
    }
    return Acquire(data, flags, loadOpts);
}

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

RVA(0x00136b00, 0xc2)
i32 SoundDevice::ReloadFile(DirectSoundMgr* buf, char* path, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
        return 1;
    }
    FILE* fp = fopen(path, s_rb);
    if (fp == 0) {
        return 0;
    }
    u32 size = _filelength(fp->_file);
    void* data = operator new(size);
    if (fread(data, size, 1, fp) != 1) {
        fclose(fp);
        operator delete(data);
        return 0;
    }
    fclose(fp);
    i32 r = ReloadRiff(buf, data, loadOpts);
    operator delete(data);
    return r;
}

// ---------------------------------------------------------------------------
// ReloadRiff: re-load a RIFF into an EXISTING buffer (Acquire sibling). Gate on init +
// non-null RIFF + buffer looping; parse, optional 16->8 downconvert, Restore, LockConvert.
// Same three-out-param parse as Acquire, and the downconvert request is this function's
// OWN 3rd parameter (`loadOpts & 1`, read from [esp+0x1c]) - never a parser output.
// MSVC5 overlays `fmt` and `size` on the dead `riff`/`buf` argument homes here, so only
// `data` needs fresh stack (retail's one-dword `push ecx` frame).
RVA(0x00136bd0, 0x110)
i32 SoundDevice::ReloadRiff(DirectSoundMgr* buf, void* riff, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == 0) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
        return 1;
    }

    void* data;
    u32 size;
    WaveFormatX* fmt;
    fmt = 0;
    data = 0;
    size = 0;
    if (ParseWaveChunks(riff, &fmt, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (loadOpts & 1) == 1) {
        cvt = 1;
    }
    if (fmt->wBitsPerSample != 0x10 || fmt->wFormatTag != 1) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        fmt->wBitsPerSample = 8;
        fmt->nAvgBytesPerSec >>= 1;
        fmt->nBlockAlign >>= 1;
    }

    if (ValidateRestore(buf, fmt, size) == 0) {
        return 0;
    }
    return buf->LockConvert(data, size, cvt) != 0;
}

RVA(0x00136ce0, 0x92)
i32 SoundDevice::ReloadResource(DirectSoundMgr* probe, const char* name, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (probe->IsLooping() == 0) {
        return 1;
    }
    // afxCurrentInstanceHandle - MFC's own documented spelling for this (AFXWIN.H:3880).
    // The inline AfxGetInstanceHandle() needs <afxwin.h>, whose inlines clang rejects in
    // this TU (the _AFX_ENABLE_INLINES guard); a NAMED member of MFC's own struct, not a
    // positional reach.
    HINSTANCE mod1 = AfxGetModuleState()->m_hCurrentInstanceHandle;
    HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
    if (!hRsrc) {
        return 0;
    }
    HINSTANCE mod2 = AfxGetModuleState()->m_hCurrentInstanceHandle;
    HGLOBAL hRes = LoadResource(mod2, hRsrc);
    if (!hRes) {
        return 0;
    }
    void* data = LockResource(hRes);
    if (!data) {
        return 0;
    }
    return ReloadRiff(probe, data, loadOpts);
}

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
        time = static_cast<i32>(::timeGetTime());
    }
    if (static_cast<u32>(time) <= static_cast<u32>(m_createFlag)) {
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
// shrink-wrap wall (77.31), and the guard form is NOT the lever. The whole loop body
// is BYTE-IDENTICAL (including the neg/sbb/and null-mask in eax); the only difference
// is that retail saves all four callee-saved regs in the prologue and pops them on
// the early exit, while cl distributes the pushes past the gate. A 4-cell matrix
// (config/axes/freesamples.json) refutes docs/patterns/positive-gate-enables-shrink-
// wrap.md here: the early-return baseline is the BEST at 77.31, the positive form
// `if (m_initialized) { loop; return 1; } return 0;` drops to 72.13 and a result
// variable to 71.39. Retail's own polarity is the early return too (`jne body`,
// return-0 as the fallthrough), so both sides already agree on the gate.
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
            // The link address must be re-formed AT the call, not reused from `node`:
            // through the `node` temp cl lands the neg/sbb/and null-mask in edx,
            // through the inline `&e->m_link` in eax as retail does (99.30 -> 99.90,
            // 6-cell matrix in config/axes/removematching.json).
            Unlink(e ? &e->m_link : 0);
            if (e) {
                PureSoundElem* pure = e; // up-cast: teardown resets to the pure base
                delete pure;
            }
        }
        e = next;
    }
}

// ---------------------------------------------------------------------------
// DSoundVoice 6-arg ctor (0x136fe0, "SoundTick_Ctor", 123 B, __thiscall): stamps
// the 0x5ef6d0 vtable (cl-auto, PureSoundElem-derived concrete voice) and the
// volume-ramp play params. The stamp arg == -1 means "start now" -> latch the
// global-clock reading (_g_pTimeGetTime @ 0x6c4650); otherwise use it verbatim.
// CloneAndPlay's `new DSoundVoice(...)` binds here.
// @early-stop
// store-SCHEDULING wall, NOT the vptr-stamp-splits-meminit divider. Retail stores
// +0xc/+0x10/+0x14/+0x1c before `mov [esi],??_7DSoundVoice` and +0x18/+0x20/+0x24
// after; we emit +0x10/+0x14/+0x18 before and +0xc/+0x1c after. Spelling retail's
// pre-stamp set as a member-init list (`: m_live(1), m_buffer(owner),
// m_stopAndRewind(slot), m_rampStartVolume(pct)`) MEASURED 87.94 -> 87.76 and moved
// NEITHER a list member up nor the body member down - the stamp is not a barrier
// here. PureSoundElem is an abstract base with no ctor, so there is no base-ctor
// call to anchor the stamp and cl schedules it freely among the stores.
RVA(0x00136fe0, 0x7b)
DSoundVoice::DSoundVoice(i32 key, i32 pct, i32 mode, DirectSoundMgr* owner, i32 slot, i32 stamp) {
    m_live = 1;
    m_buffer = owner;
    m_stopAndRewind = slot;
    m_rampStartVolume = pct;
    m_rampEndVolume = key;
    m_rampDurationMs = mode;
    m_rampStartTime = (stamp == -1) ? ::timeGetTime() : stamp;
}

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
// @early-stop
// this-vs-arg callee-saved pick (95.70): retail loads `mov esi,ecx` (this) first and
// puts `now` in edi; we load `now` into esi first and this into edi. Every
// instruction is otherwise identical. A 6-cell matrix over the elapsed computation
// (config/axes/dsoundvoicetick.json - dur/start hoisted to locals, both hoisted,
// negated-member LHS, `done` split from its declaration) scored ALL SIX identical to
// baseline, so the statement site is not the lever; the pick is made by the
// allocator, not by first-use order in the source.
RVA(0x00137060, 0x6b)
i32 DSoundVoice::Tick(i32 now) {
    i32 done = 0;
    i32 elapsed = now - m_rampStartTime;
    if (static_cast<u32>(elapsed) >= static_cast<u32>(m_rampDurationMs)) {
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
// 'fmt ' payload into *fmtOut and 'data' ptr/len into *dataOut/*sizeOut; nonzero when
// 'fmt ' seen before 'data'.
// (100%: the per-chunk cursor advance is TWO `*p++` reads - a use between the increments
// blocks the +8 fold - and the ex "add-fold wall" was the p+=2 spelling.)
RVA(0x00137110, 0x8d)
i32 ParseWaveChunks(void* riff, WaveFormatX** fmtOut, void** dataOut, u32* sizeOut) {
    // The RIFF walk: dword reads, byte-granular bounds, and the 'fmt ' body typed at
    // the cursor - all three readings are named on RiffCursor.
    RiffCursor p;
    p.m_b = static_cast<char*>(riff) + 4;
    u32 riffSize = *p.m_w;
    p.m_w++;
    u32 waveTag = *p.m_w;
    p.m_w++;
    char* end = p.m_b + riffSize - 4;
    if (*static_cast<u32*>(riff) != mmioFOURCC('R', 'I', 'F', 'F')) {
        return 0;
    }
    if (waveTag != mmioFOURCC('W', 'A', 'V', 'E')) {
        return 0;
    }
    *fmtOut = 0;
    *dataOut = 0;
    while (p.m_b < end) {
        u32 id = *p.m_w++;
        u32 size = *p.m_w++;
        if (id == mmioFOURCC('f', 'm', 't', ' ')) {
            *fmtOut = p.m_fmt;
        } else if (id == mmioFOURCC('d', 'a', 't', 'a')) {
            *dataOut = p.m_w;
            *sizeOut = size;
            return *fmtOut != 0;
        }
        // the RIFF chunk walk: advance by the chunk's own size, word-aligned up
        p.m_b += ((size + 1) & ~1);
    }
    return 0;
}

RVA(0x001371a0, 0x5a)
i32 SoundDevice::SetPrimaryFormat(void* fmt) {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_primaryBuffer->SetFormat(static_cast<LPWAVEFORMATEX>(fmt)) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x678, hr);
        return 0;
    }
    return 1;
}

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

RVA(0x00137260, 0x95)
i32 SoundDevice::CreatePrimaryBuffer() {
    if (m_initialized == 0) {
        return 0;
    }
    // a primary buffer needs a priority coop level - bail when still NORMAL
    if (m_coopLevel == DSSCL_NORMAL) {
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
