// GruntzSoundZ.cpp - the Dsndmgr positional/zoned sound-bank manager and its
// per-bank inner sound object (C:\Proj\Dsndmgr\, the 0x138xxx AIL/DirectSound
// region). This is the whole 0x138490-0x139030 method band: the outer manager
// CGruntzSoundZ (a CMapStringToOb keyed by each bank's inline name buffer) and the
// inner AIL/XMIDI sequence object CGruntzSoundInnerZ (the real polymorphic
// Wap::CObject-derived class whose 16-slot vtable @ 0x1ef700 cl auto-emits from the
// inline ctor here). It also carries the two __thiscall XMIDI master-volume helpers
// (0x138950/0x1389c0), reached as m_sound-> methods off CGruntzMgr +0x48.
//
// CGruntzSoundZ adds no virtuals, so it reuses CMapStringToOb's vftable (no
// ??_7CGruntzSoundZ is emitted); the scalar destructor at 0x086040 runs Shutdown
// then the inherited ~CMapStringToOb (an out-of-line NAFXCW thunk, reloc-masked).
// Its local destructible CString in StopAndFlush forces the /GX EH frame, so this
// unit is built with the "eh" flag profile; the plain AIL leaf methods carry no EH
// state (no destructible locals) even under /GX.
//
// Externals reached here (all reloc-masked rel32/DIR32): CMapStringToOb::operator[]/
// Lookup/GetNextAssoc/RemoveAll/~ (NAFXCW), CString ctor/dtor, operator new/delete,
// the Miles Sound System (AIL) imports through the IAT, and the inner sound's retail
// vftable @ 0x5ef700.
//
// Field names are placeholders; the OFFSETS + emitted code bytes are load-bearing.
#include <Dsndmgr/GruntzSoundZ.h>
#include <rva.h>

#include <mss.h>    // the genuine Miles Sound System (MSS32.DLL) AIL/XMIDI SDK header
#include <stdio.h>  // sprintf - the "MIDI%i" auto-name (0x11f890, _sprintf)
#include <string.h> // strcpy/memcpy (inline rep movs / repne scas under /O2)

// ---------------------------------------------------------------------------
// The AIL MIDI driver state globals (.data), shared with the RezSync game-init.
DATA(0x00253c5c)
extern HMDIDRIVER g_ailMidiDriver; // 0x653c5c  AIL/digital MIDI driver handle (0 = none open)
DATA(0x00253c60)
extern i32 g_midiSeqCounter; // 0x653c60  monotonic auto-name counter ("MIDI%i")
DATA(0x00253c64)
extern HMDIDRIVER g_ailDriver64; // 0x653c64  cached driver handle passed to Init

// ---------------------------------------------------------------------------
// ~CGruntzSoundZ scalar destructor: stop/flush everything via Shutdown, then the
// inherited ~CMapStringToOb fires from the destructor epilogue. The destructible
// base forces the /GX EH state machine (state 0 across Shutdown, -1 after).
// @early-stop
// vptr-restamp wall (~91.6%) - complete & correct: the /GX EH frame, the trylevel
// 0/-1 stamps, the Shutdown call, and the trailing base ~CMapStringToOb call are
// all byte-exact. The only divergence is one extra `mov [esi],&??_7CGruntzSoundZ`
// re-stamp our polymorphic model emits at dtor entry that retail elided (the body
// makes no virtual call on `this`, so cl's /GX EH machine dropped the dead store).
// The polymorphic CMapStringToOb base is REQUIRED (it supplies the EH frame +
// base-dtor call); no source spelling flips the re-stamp presence.
// See docs/patterns/eh-dtor-vptr-restamp-presence.md.
RVA(0x00086040, 0x49)
CGruntzSoundZ::~CGruntzSoundZ() {
    Shutdown();
}

// ---------------------------------------------------------------------------
// Init: cache the MIDI/digital driver handles, clear the current bank, mark the
// bank enabled, and (unless skipInit) start AIL + open the MIDI-out driver; if the
// open fails or yields no driver, mark the bank disabled. __thiscall, ret 0xc.
RVA(0x00138490, 0x5e)
i32 CGruntzSoundZ::Init(i32 mdiHandle, i32 digHandle, i32 skipInit) {
    m_mdiHandle = mdiHandle;
    m_digHandle = digHandle;
    m_pCurrent = 0;
    m_enabled = 1;
    g_ailDriver64 = (HMDIDRIVER)mdiHandle;
    if (skipInit == 0) {
        AIL_startup();
        if (AIL_midiOutOpen(&g_ailMidiDriver, 0, -1) != 0 || g_ailMidiDriver == 0) {
            m_enabled = 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Shutdown: flush every bank, tear the current bank down (Stop, +0x30), flush
// again, clear the digital handle / current bank / AIL MIDI driver, and shut the
// Miles Sound System down. Returns void (falls off the end; eax unset).
RVA(0x001384f0, 0x3b)
void CGruntzSoundZ::Shutdown() {
    StopAndFlush();
    if (m_pCurrent != 0) {
        m_pCurrent->Stop();
    }
    StopAndFlush();
    m_digHandle = 0;
    m_pCurrent = 0;
    g_ailMidiDriver = 0;
    AIL_shutdown();
}

// ---------------------------------------------------------------------------
// StopAndFlush: stop the currently-playing bank, iterate the map destroying every
// bank via its scalar-deleting destructor, RemoveAll, and clear m_pCurrent. The
// per-iteration CString key forces the /GX EH frame.
RVA(0x00138530, 0xa2)
void CGruntzSoundZ::StopAndFlush() {
    if (m_pCurrent != 0) {
        m_pCurrent->Stop();
    }
    POSITION pos = (POSITION)(GetCount() != 0 ? -1 : 0);
    if (pos != (POSITION)0) {
        do {
            CString key;
            CObject* val = 0;
            GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (CGruntzSoundInnerZ*)val;
            }
        } while (pos != (POSITION)0);
    }
    RemoveAll();
    m_pCurrent = 0;
}

// ---------------------------------------------------------------------------
// CreateBank2: the 2-arg twin of CreateBank. Heap-allocate + seed a 0x60-byte
// inner sound, then run its file-load setup (Load, slot 6); on failure destroy it
// (scalar dtor) and return 0, on success insert it into the map and return it.
RVA(0x001385e0, 0x85)
CGruntzSoundInnerZ* CGruntzSoundZ::CreateBank2(const char* path, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = new CGruntzSoundInnerZ();
    if (inner->Load(path, name) == 0) {
        if (inner != 0) {
            delete inner;
        }
        return 0;
    }
    Insert(inner);
    return inner;
}

// ---------------------------------------------------------------------------
// CreateBank: if enabled, heap-allocate a 0x60-byte inner sound and run its
// in-memory decode setup (DecodeBuf, slot 5); on failure destroy it and return 0,
// on success insert it into the map and return it.
RVA(0x00138670, 0x8a)
CGruntzSoundInnerZ* CGruntzSoundZ::CreateBank(const void* buf, u32 len, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = new CGruntzSoundInnerZ();
    if (inner->DecodeBuf(buf, len, name) == 0) {
        if (inner != 0) {
            delete inner;
        }
        return 0;
    }
    Insert(inner);
    return inner;
}

// ---------------------------------------------------------------------------
// Insert: key the bank into the map by its inline name buffer (+0x04); if there is
// no currently-playing bank yet, adopt this one as current.
RVA(0x00138700, 0x2d)
void CGruntzSoundZ::Insert(CGruntzSoundInnerZ* inner) {
    if (inner == 0) {
        return;
    }
    if (m_enabled == 0) {
        return;
    }
    (*this)[inner->m_name] = (CObject*)inner;
    if (m_pCurrent == 0) {
        m_pCurrent = inner;
    }
}

// ---------------------------------------------------------------------------
// Lookup a bank by name; gated on the digital driver handle and a non-empty key.
RVA(0x00138730, 0x41)
CGruntzSoundInnerZ* CGruntzSoundZ::FindBank(const char* key) {
    if (m_digHandle == 0) {
        return 0;
    }
    if (key == 0) {
        return 0;
    }
    if (*key == 0) {
        return 0;
    }
    CObject* result = 0;
    return Lookup(key, result) ? (CGruntzSoundInnerZ*)result : 0;
}

// ---------------------------------------------------------------------------
// PlayCreate2: create-or-fail a 2-arg bank, stop the current bank, and start the
// new one on the digital driver; on success adopt it as current and return 1.
RVA(0x00138780, 0x5b)
i32 CGruntzSoundZ::PlayCreate2(const char* path, i32 playMode, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = CreateBank2(path, name);
    if (inner == 0) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_digHandle, playMode) == 0) {
        return 0;
    }
    m_pCurrent = inner;
    return 1;
}

// ---------------------------------------------------------------------------
// PlayCreate3: the 3-arg twin of PlayCreate2 (creates a 3-arg bank then plays it).
RVA(0x001387e0, 0x60)
i32 CGruntzSoundZ::PlayCreate3(const void* buf, u32 len, i32 playMode, const char* name) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = CreateBank(buf, len, name);
    if (inner == 0) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_digHandle, playMode) == 0) {
        return 0;
    }
    m_pCurrent = inner;
    return 1;
}

// ---------------------------------------------------------------------------
// PlayByName: look the bank up by name, stop whatever is current, start it on the
// digital driver; on success adopt it as current and return 1.
RVA(0x00138840, 0x56)
i32 CGruntzSoundZ::PlayByName(const char* name, i32 playMode) {
    if (m_enabled == 0) {
        return 0;
    }
    CGruntzSoundInnerZ* inner = FindBank(name);
    if (inner == 0) {
        return 0;
    }
    StopCurrent();
    if (inner->Play(m_digHandle, playMode) == 0) {
        return 0;
    }
    m_pCurrent = inner;
    return 1;
}

// ---------------------------------------------------------------------------
// StopCurrent: stop the currently-playing bank and forget it.
RVA(0x001388a0, 0x18)
void CGruntzSoundZ::StopCurrent() {
    if (m_pCurrent != 0) {
        m_pCurrent->Stop();
        m_pCurrent = 0;
    }
}

// ---------------------------------------------------------------------------
// Restart: re-launch the current bank - stop it (Stop, +0x30) then replay it on
// the digital driver (Play, +0x24). Returns Play's result; 0 when no current bank.
RVA(0x001388c0, 0x2a)
i32 CGruntzSoundZ::Restart(i32 playMode) {
    if (m_pCurrent == 0) {
        return 0;
    }
    m_pCurrent->Stop();
    return m_pCurrent->Play(m_digHandle, playMode);
}

// ---------------------------------------------------------------------------
// StopAll: forward to the current bank's "stop all" slot (+0x28); 0 if none.
// Tail-call (mov eax,[m_pCurrent]; jmp [eax+0x28]).
RVA(0x001388f0, 0xf)
i32 CGruntzSoundZ::StopAll() {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->StopAll();
}

// ---------------------------------------------------------------------------
// StopBank: forward `bank` to the current bank's "stop bank" slot (+0x2c); 0 if none.
RVA(0x00138900, 0x19)
i32 CGruntzSoundZ::StopBank(i32 bank) {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->StopBank(bank);
}

// ---------------------------------------------------------------------------
// IsPlaying: forward to the current bank's status query (Stop, +0x30); 0 if none.
RVA(0x00138920, 0xf)
i32 CGruntzSoundZ::IsPlaying() {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->Stop();
}

// ---------------------------------------------------------------------------
// SetXMidiVolume: scale a 0..100 volume to 0..127 and push it to the AIL XMIDI
// driver. __thiscall (reached as m_sound->SetXMidiVolume off CGruntzMgr +0x48),
// but the body ignores `this` and drives the global AIL MIDI driver.
RVA(0x00138950, 0x70)
i32 CGruntzSoundZ::SetXMidiVolume(i32 volume) {
    if (g_ailMidiDriver == 0) {
        return 0;
    }
    i32 scaled;
    if (volume <= 0) {
        scaled = 0;
    } else if (volume >= 100) {
        scaled = 0x7f;
    } else {
        scaled = volume * 127 / 100;
    }
    AIL_set_XMIDI_master_volume(g_ailMidiDriver, scaled);
    return 1;
}

// ---------------------------------------------------------------------------
// GetXMidiVolume: read the AIL XMIDI master volume and rescale 0..127 -> 0..100.
// __thiscall (reached as m_sound->GetXMidiVolume), `this` unused.
// @early-stop
// divide-by-127 scheduling wall (~91%): logic byte-faithful (the g_ailMidiDriver
// gate, the AIL read, the 0/0x64 clamps, and the v*100/127 magic-multiply are all
// correct); the residual is MSVC's register/scheduling choice inside the reciprocal
// divide (the lea/lea/shl v*100 chain and the 0x81020409 imul). Re-homing to a
// __thiscall method is byte-neutral (`this` unused).
RVA(0x001389c0, 0x47)
i32 CGruntzSoundZ::GetXMidiVolume() {
    if (g_ailMidiDriver == 0) {
        return 0x64;
    }
    i32 v = AIL_XMIDI_master_volume(g_ailMidiDriver);
    if (v <= 0) {
        return 0;
    }
    if (v >= 0x7f) {
        return 0x64;
    }
    return v * 100 / 127;
}

// ===========================================================================
// CGruntzSoundInnerZ - the per-bank inner AIL/XMIDI sequence object. Its methods
// gate on IsStarted() (virtual slot 8) and drive the AIL sequence handle m_seqHandle.
// ===========================================================================

// ---------------------------------------------------------------------------
// ~CGruntzSoundInnerZ (0x138a50, base-object ??1 dtor, re-homed from
// BoundaryUpperEh): cl stamps ??_7CGruntzSoundInnerZ (0x5ef700), runs
// ReleaseHandle (slot 7, devirtualized inside the dtor -> direct call 0x138dd0),
// then chains the empty ~Wap::CObject base (restamp 0x5e8cb4). ReleaseHandle can
// throw (operator delete), so the base subobject carries the /GX unwind frame. The
// vtable-slot scalar-deleting dtor 0x138a30 is an (external) LIBCMT carve-out.
// ---------------------------------------------------------------------------
RVA(0x00138a50, 0x46)
CGruntzSoundInnerZ::~CGruntzSoundInnerZ() {
    ReleaseHandle();
}

// ---------------------------------------------------------------------------
// DecodeBuf (vtable slot 5): one-time in-memory setup - seed defaults, name the
// sequence (`name` or auto "MIDI%i"), copy `len` XMIDI bytes into the owned
// m_loadBuffer, then allocate + initialise an AIL sequence handle. __thiscall, ret 0xc.
RVA(0x00138c20, 0x122)
i32 CGruntzSoundInnerZ::DecodeBuf(const void* buf, u32 len, const char* name) {
    if (buf == 0) {
        return 0;
    }
    if (len < 4) {
        return 0;
    }
    if (g_ailMidiDriver == 0) {
        return 0;
    }
    ++g_midiSeqCounter;
    m_playMode = 0;
    m_tempoPct = 100;
    m_volumePct = 100;
    if (name != 0) {
        strcpy(m_name, name);
    } else {
        sprintf(m_name, "MIDI%i", g_midiSeqCounter);
    }
    if (m_loadBuffer == 0) {
        m_loadBuffer = (char*)operator new(len);
        if (m_loadBuffer == 0) {
            return 0;
        }
        memcpy(m_loadBuffer, buf, len);
    }
    m_seqHandle = AIL_allocate_sequence_handle(g_ailMidiDriver);
    if (m_seqHandle == 0) {
        return 0;
    }
    if (AIL_init_sequence(m_seqHandle, m_loadBuffer, 0) == 0) {
        AIL_release_sequence_handle(m_seqHandle);
        m_seqHandle = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ReleaseHandle (vtable slot 7): stop the sequence (Stop, +0x30), release the AIL
// sequence handle, and free the owned load buffer.
RVA(0x00138dd0, 0x36)
void CGruntzSoundInnerZ::ReleaseHandle() {
    Stop();
    if (m_seqHandle != 0) {
        AIL_release_sequence_handle(m_seqHandle);
        m_seqHandle = 0;
    }
    if (m_loadBuffer != 0) {
        operator delete(m_loadBuffer);
        m_loadBuffer = 0;
    }
}

// ---------------------------------------------------------------------------
// Play (vtable slot 9): if started, cache the digital driver/mode, start the AIL
// sequence, arm looping when requested, and clear the pause depth.
RVA(0x00138e10, 0x4a)
i32 CGruntzSoundInnerZ::Play(i32 hDriver, i32 mode) {
    if (IsStarted() == 0) {
        return 0;
    }
    m_playDriver = hDriver;
    m_playMode = mode;
    AIL_start_sequence(m_seqHandle);
    if (mode != 0) {
        AIL_set_sequence_loop_count(m_seqHandle, 0);
    }
    m_pauseDepth = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// Stop (vtable slot 12): if started, end the AIL sequence and clear the pause depth.
RVA(0x00138e60, 0x26)
i32 CGruntzSoundInnerZ::Stop() {
    if (IsStarted() == 0) {
        return 0;
    }
    AIL_end_sequence(m_seqHandle);
    m_pauseDepth = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// StopAll (vtable slot 10): if started and busy, stop the AIL sequence the first
// time and count nested pauses.
RVA(0x00138e90, 0x3a)
i32 CGruntzSoundInnerZ::StopAll() {
    if (IsStarted() == 0) {
        return 0;
    }
    if (IsBusy() == 0) {
        return 0;
    }
    if (m_pauseDepth == 0) {
        AIL_stop_sequence(m_seqHandle);
    }
    m_pauseDepth++;
    return 1;
}

// ---------------------------------------------------------------------------
// StopBank (vtable slot 11): unnest the pause depth and re-issue the AIL resume
// when it reaches zero; `bank` non-zero forces an immediate resume.
RVA(0x00138ed0, 0x4f)
i32 CGruntzSoundInnerZ::StopBank(i32 bank) {
    if (IsStarted() == 0) {
        return 0;
    }
    if (IsBusy() != 0) {
        return 1;
    }
    if (m_pauseDepth > 0) {
        m_pauseDepth--;
        if (bank != 0) {
            m_pauseDepth = 0;
        }
        if (m_pauseDepth <= 0) {
            AIL_resume_sequence(m_seqHandle);
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// IsBusy: if the bank's "is started" gate (slot 8) is clear, not busy; otherwise
// query the AIL sequence status and report busy for PLAYING (4) / PLAYINGBUTRELEASED
// (0x10).
RVA(0x00138f60, 0x2d)
i32 CGruntzSoundInnerZ::IsBusy() {
    if (IsStarted() == 0) {
        return 0;
    }
    i32 status = AIL_sequence_status(m_seqHandle);
    if (status == 4 || status == 0x10) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SetTempo: if started, set the AIL sequence tempo and cache the tempo percent.
RVA(0x00138f90, 0x32)
i32 CGruntzSoundInnerZ::SetTempo(i32 tempo, i32 ms) {
    if (IsStarted() == 0) {
        return 0;
    }
    AIL_set_sequence_tempo(m_seqHandle, tempo, ms);
    m_tempoPct = tempo;
    return 1;
}

// ---------------------------------------------------------------------------
// SetVolume: if started, scale a 0..100 volume to 0..127, set the AIL sequence
// volume, and cache the volume percent.
RVA(0x00138fd0, 0x5e)
i32 CGruntzSoundInnerZ::SetVolume(i32 volume, i32 ms) {
    if (IsStarted() == 0) {
        return 0;
    }
    i32 scaled;
    if (volume <= 0) {
        scaled = 0;
    } else if (volume >= 100) {
        scaled = 0x7f;
    } else {
        scaled = volume * 127 / 100;
    }
    AIL_set_sequence_volume(m_seqHandle, scaled, ms);
    m_volumePct = volume;
    return 1;
}

// ---------------------------------------------------------------------------
// SetLoop: if started and the cached loop flag changed, re-arm the AIL loop count
// (0 = loop forever, 1 = play once).
RVA(0x00139030, 0x4c)
i32 CGruntzSoundInnerZ::SetLoop(i32 loop) {
    if (IsStarted() == 0) {
        return 0;
    }
    if (m_playMode != loop) {
        m_playMode = loop;
        if (loop != 0) {
            AIL_set_sequence_loop_count(m_seqHandle, 0);
        } else {
            AIL_set_sequence_loop_count(m_seqHandle, 1);
        }
    }
    return 1;
}
