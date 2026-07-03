// CGruntzSoundZ.cpp - the Dsndmgr positional/zoned sound-bank manager
// (C:\Proj\Dsndmgr\, the 0x138xxx AIL/DirectSound region). CGruntzSoundZ derives
// from MFC CMapStringToOb: the map is the object's base (offset 0), keyed by each
// bank's inline name buffer; the manager tracks the currently-playing bank at
// +0x1c, the AIL digital/MIDI driver handles at +0x20/+0x24, and an "enabled"
// flag at +0x28 (see include/Dsndmgr/CGruntzSoundZ.h).
//
// CGruntzSoundZ adds no virtuals, so it reuses CMapStringToOb's vftable (no
// ??_7CGruntzSoundZ is emitted); the scalar destructor at 0x086040 runs Shutdown
// then the inherited ~CMapStringToOb (an out-of-line NAFXCW thunk, reloc-masked).
// Its local destructible CString in the StopAll teardown forces the /GX EH frame,
// so this unit is built with the "eh" flag profile.
//
// Externals reached here (all reloc-masked rel32/DIR32):
//   CMapStringToOb::operator[] / Lookup / GetNextAssoc / RemoveAll / ~ (NAFXCW),
//   CString ctor/dtor, operator new, the inner sound's retail vftable @ 0x5ef700,
//   and CGruntzSoundZ::Shutdown (defined in the sibling AIL TU at 0x1384f0).
//
// Field names are placeholders; the OFFSETS + emitted code bytes are load-bearing.
#include <Dsndmgr/CGruntzSoundZ.h>
#include <rva.h>

// CGruntzSoundInnerZ is a real polymorphic class; the create helpers use plain
// `new CGruntzSoundInnerZ()` (retail's operator-new + null-guarded inline ctor).

// Miles Sound System (AIL) sequence-status query, reached through the IAT
// (call ds:[__imp__AIL_sequence_status@4]); used by CGruntzSoundInnerZ::IsBusy.
extern "C" __declspec(dllimport) i32 __stdcall AIL_sequence_status(i32 seq);

// ---------------------------------------------------------------------------
// ~CGruntzSoundZ scalar destructor: stop/flush everything via Shutdown (a
// __thiscall method defined in the sibling AIL TU, so this call is a reloc-masked
// rel32), then the inherited ~CMapStringToOb fires from the destructor epilogue.
// The destructible base forces the /GX EH state machine (state 0 across Shutdown,
// -1 after).
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
// inner sound (same field layout), then run its alternate one-time setup via the
// inner vtable slot +0x18 (Load, 2 args); on failure destroy it (scalar dtor) and
// return 0, on success insert it into the map and return it.
RVA(0x001385e0, 0x85)
CGruntzSoundInnerZ* CGruntzSoundZ::CreateBank2(i32 path, i32 name) {
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
// CreateBank: if enabled, heap-allocate a 0x60-byte inner sound, stamp its vtable
// and seed its fields, run its one-time Init(+0x14); on failure destroy it and
// return 0, on success insert it into the map and return it.
RVA(0x00138670, 0x8a)
CGruntzSoundInnerZ* CGruntzSoundZ::CreateBank(i32 buf, i32 len, i32 name) {
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
i32 CGruntzSoundZ::PlayCreate2(i32 path, i32 playMode, i32 name) {
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
i32 CGruntzSoundZ::PlayCreate3(i32 buf, i32 len, i32 playMode, i32 name) {
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
// Play: look the bank up by name, stop whatever is current, start it on the
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
// Restart: re-launch the current bank - stop it (vtable +0x30) then replay it on
// the digital driver with the supplied parameter (vtable +0x24). Returns Play's
// result; 0 when no current bank.
RVA(0x001388c0, 0x2a)
i32 CGruntzSoundZ::Restart(i32 playMode) {
    if (m_pCurrent == 0) {
        return 0;
    }
    m_pCurrent->Stop();
    return m_pCurrent->Play(m_digHandle, playMode);
}

// ---------------------------------------------------------------------------
// IsPlaying: forward to the current bank's status query (vtable +0x30); 0 if none.
RVA(0x00138920, 0xf)
i32 CGruntzSoundZ::IsPlaying() {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->Stop();
}

// ---------------------------------------------------------------------------
// StopAll: forward to the current bank's "stop all" slot (vtable +0x28); 0 if
// none. Tail-call (mov eax,[m_pCurrent]; jmp [eax+0x28]).
RVA(0x001388f0, 0xf)
i32 CGruntzSoundZ::StopAll() {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->StopAll();
}

// ---------------------------------------------------------------------------
// StopBank: forward `bank` to the current bank's "stop bank" slot (vtable +0x2c); 0 if none.
RVA(0x00138900, 0x19)
i32 CGruntzSoundZ::StopBank(i32 bank) {
    if (m_pCurrent == 0) {
        return 0;
    }
    return m_pCurrent->StopBank(bank);
}

// ---------------------------------------------------------------------------
// CGruntzSoundInnerZ::IsBusy: if the bank's "is started" gate (vtable
// +0x20) is clear, not busy; otherwise query the AIL sequence status of the
// bank's MIDI handle (m_seqHandle) and report busy for PLAYING (4) / PLAYINGBUTRELEASED
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
