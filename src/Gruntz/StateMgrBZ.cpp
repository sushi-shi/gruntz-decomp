#include <rva.h>
// StateMgrBZ.cpp - the engine input/control state singleton (*g_645578). Built
// from the DirectInputMgr2 (*g_645570) with control mode 6 by the game's Init
// (0x83450); driven each frame by CGruntzMgr::TickStateMgrs (0x920b0 -> Flush).
//
// The five methods:
//   Init(src,mode)  0x382c0 - clear the latch, Build the source wiring, Setup the
//                             key tables, Reset, Flush. Returns success.
//   Build(src,mode) 0x383b0 - dense 9-way switch on the mode: wire 0..3 device
//                             sources from the manager's controller list.
//   Setup()         0x38340 - seed the m_4 device's +0x2b4.. scan-code table.
//   Flush()         0x385e0 - OR-fold the source devices' packed key flags.
//   Reset()         0x386b0 - per-device Reset dispatch, then clear the latch.
//
// Only offsets + code bytes are load-bearing; names are placeholders.
#include <Gruntz/StateMgrBZ.h>

// ---------------------------------------------------------------------------
// StateMgrBZ::Init (0x382c0; __thiscall, ret 8). Clear the latched state, then
// run Build/Setup/Reset/Flush in order; bail out if the source is null or Build
// fails.
RVA(0x000382c0, 0x52)
i32 StateMgrBZ::Init(SbzInputManager* src, i32 mode) {
    if (src == 0) {
        return 0;
    }
    m_1c = 0;
    m_18 = 0;
    m_20 = 0;
    m_24 = 0;
    if (!Build(src, mode)) {
        return 0;
    }
    Setup();
    Reset();
    Flush();
    return 1;
}

// ---------------------------------------------------------------------------
// StateMgrBZ::Setup (0x38340; __thiscall, ret 0). Seed the m_4 device's scan-code
// table with the five mode-6 control slots. Re-reads m_4 per store.
RVA(0x00038340, 0x46)
void StateMgrBZ::Setup() {
    if (m_4) {
        m_4->m_key0 = 0x10;
        m_4->m_key1 = 0xd;
        m_4->m_key2 = 0x20;
        m_4->m_key4 = 0x12;
        m_4->m_key5 = 0x11;
    }
}

// ---------------------------------------------------------------------------
// StateMgrBZ::Build (0x383b0; __thiscall, ret 8). Dense 9-way switch on the
// control mode: wire up 0..3 device sources (and, for modes 6..8, register a
// composite controller node) from the manager's device list. Returns 1 unless the
// source is null.
// @early-stop
// jumptable-data-overlap scoring artifact: every code instruction matches; the
// residual is the inline .rdata jump-table data block + the reloc-masked switch-
// base / AddControllerArr symbol names. See docs/patterns/jumptable-data-overlap.md.
RVA(0x000383b0, 0x19c)
i32 StateMgrBZ::Build(SbzInputManager* src, i32 mode) {
    if (src == 0) {
        return 0;
    }
    m_4 = 0;
    m_8 = 0;
    m_10 = 0;
    m_0 = 0;
    m_14 = 0;
    switch ((u32)mode) {
        case 1: {
            SbzInputDevice* d = src->m_devA;
            m_4 = d;
            m_0 = d;
            break;
        }
        case 2: {
            SbzInputDevice* d = (src->m_count > 0) ? src->m_data[0] : 0;
            m_8 = d;
            m_0 = d;
            break;
        }
        case 3: {
            SbzInputDevice* d = (src->m_count > 1) ? src->m_data[1] : 0;
            m_8 = d;
            m_0 = d;
            break;
        }
        case 4: {
            SbzInputDevice* d = (src->m_count > 2) ? src->m_data[2] : 0;
            m_8 = d;
            m_0 = d;
            break;
        }
        case 5: {
            SbzInputDevice* d = (src->m_count > 3) ? src->m_data[3] : 0;
            m_8 = d;
            m_0 = d;
            break;
        }
        case 6: {
            m_4 = src->m_devA;
            SbzInputDevice* d = (src->m_count > 0) ? src->m_data[0] : 0;
            m_8 = d;
            m_10 = (SbzDeviceList*)src->AddControllerArr((i32)m_4, (i32)d, 0, 0, 0, 0, 0);
            break;
        }
        case 8: {
            m_4 = src->m_devA;
            SbzInputDevice* d = (src->m_count > 0) ? src->m_data[0] : 0;
            m_8 = d;
            m_c = src->m_devB;
            m_10 = (SbzDeviceList*)src->AddControllerArr((i32)m_4, (i32)d, (i32)m_c, 0, 0, 0, 0);
            break;
        }
        case 7:
            m_4 = src->m_devA;
            m_c = src->m_devB;
            m_10 = (SbzDeviceList*)src->AddControllerArr((i32)m_4, (i32)m_c, 0, 0, 0, 0, 0);
            break;
        case 0:
            m_4 = 0;
            m_8 = 0;
            m_10 = 0;
            m_c = 0;
            m_0 = 0;
            break;
    }
    m_14 = mode;
    return 1;
}

// ---------------------------------------------------------------------------
// StateMgrBZ::Flush (0x385e0; __thiscall, ret 0). OR-fold the source devices'
// packed current/edge key flags into m_18/m_1c, with a "suppress input" clear, and
// snapshot into m_20.
// @early-stop
// inverse zero-register-pinning regalloc wall: body byte-structurally identical
// (same offsets/sequence), but our cl pins 0 in edx (extra push esi + cmp eax,edx
// null tests + reg-zero clears) where retail uses test eax,eax + immediate mov
// [],0 and edx as the OR temp. No /O2 source lever flips the pin (coin-flip).
// See docs/patterns/zero-register-pinning.md (INVERSE case).
RVA(0x000385e0, 0x9f)
i32 StateMgrBZ::Flush() {
    if (m_0) {
        m_18 = m_0->m_edgeKeys;
        m_1c = m_0->m_currentKeys;
    } else if (m_10) {
        m_18 = m_4->m_edgeKeys;
        m_1c = m_4->m_currentKeys;
        if (m_8) {
            m_18 |= m_8->m_edgeKeys;
            m_1c |= m_8->m_currentKeys;
        }
        if (m_c) {
            m_18 |= m_c->m_edgeKeys;
            m_1c |= m_c->m_currentKeys;
        }
    }
    if (m_24) {
        m_18 = 0;
        m_1c = 0;
    }
    m_20 = m_18;
    return 1;
}

// ---------------------------------------------------------------------------
// StateMgrBZ::Reset (0x386b0; __thiscall, ret 0). Dispatch the per-device Reset
// (slot 5) over m_0 or, when none, the m_10 array, then clear the latched flags.
RVA(0x000386b0, 0x5d)
i32 StateMgrBZ::Reset() {
    SbzInputDevice* d = m_0;
    if (d) {
        d->Reset();
    } else {
        SbzDeviceList* arr = m_10;
        if (arr && arr->m_count > 0) {
            SbzInputDevice** p = &arr->m_elems[0];
            i32 i = 0;
            do {
                (*p)->Reset();
                ++i;
                ++p;
            } while (i < arr->m_count);
        }
    }
    m_1c = 0;
    m_18 = 0;
    m_20 = 0;
    return 1;
}
