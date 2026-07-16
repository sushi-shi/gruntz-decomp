#include <rva.h>
#include <DinMgr2/DirectInputMgr2.h>
// StateMgrBZ.cpp - the engine input/control state singleton (*g_spawnConfig). Built
// from the DirectInputMgr2 (*g_inputMgr) with control mode 6 by the game's Init
// (0x83450); driven each frame by CGruntzMgr::TickStateMgrs (0x920b0 -> Flush).
//
// The five methods:
//   Init(src,mode)  0x382c0 - clear the latch, Build the source wiring, Setup the
//                             key tables, Reset, Flush. Returns success.
//   Build(src,mode) 0x383b0 - dense 9-way switch on the mode: wire 0..3 device
//                             sources from the manager's controller list.
//   Setup()         0x38340 - seed the keyboard device's +0x2b4.. scan-code table.
//   Flush()         0x385e0 - OR-fold the source devices' packed key flags.
//   Reset()         0x386b0 - per-device Reset dispatch, then clear the latch.
//
// Field names recovered from usage; offsets + code bytes are load-bearing.
#include <Gruntz/StateMgrBZ.h>

// ---------------------------------------------------------------------------
// StateMgrBZ::Init (0x382c0; __thiscall, ret 8). Clear the latched state, then
// run Build/Setup/Reset/Flush in order; bail out if the source is null or Build
// fails.
RVA(0x000382c0, 0x52)
i32 StateMgrBZ::Init(DirectInputMgr2* src, i32 mode) {
    if (src == 0) {
        return 0;
    }
    m_currentKeys = 0;
    m_edgeKeys = 0;
    m_latchedKeys = 0;
    m_suppress = 0;
    if (!Build(src, mode)) {
        return 0;
    }
    Setup();
    Reset();
    Flush();
    return 1;
}

// ---------------------------------------------------------------------------
// StateMgrBZ::Setup (0x38340; __thiscall, ret 0). Seed the keyboard device's
// scan-code table with the five mode-6 control slots (VK_ virtual-key codes).
// Re-reads m_keyboard per store.
RVA(0x00038340, 0x46)
void StateMgrBZ::Setup() {
    if (m_keyboard) {
        m_keyboard->m_keyTable[0] = 0x10; // VK_SHIFT
        m_keyboard->m_keyTable[1] = 0xd;  // VK_RETURN
        m_keyboard->m_keyTable[2] = 0x20; // VK_SPACE
        m_keyboard->m_keyTable[4] = 0x12; // VK_MENU
        m_keyboard->m_keyTable[5] = 0x11; // VK_CONTROL
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
i32 StateMgrBZ::Build(DirectInputMgr2* src, i32 mode) {
    if (src == 0) {
        return 0;
    }
    m_keyboard = 0;
    m_joystick = 0;
    m_deviceList = 0;
    m_device = 0;
    m_mode = 0;
    switch (static_cast<u32>(mode)) {
        case 1: {
            CInputDevice* d = (CInputDevice*)src->m_deviceA;
            m_keyboard = d;
            m_device = d;
            break;
        }
        case 2: {
            CInputDevice* d =
                (src->m_devices.GetSize() > 0) ? (CInputDevice*)src->m_devices.GetAt(0) : 0;
            m_joystick = d;
            m_device = d;
            break;
        }
        case 3: {
            CInputDevice* d =
                (src->m_devices.GetSize() > 1) ? (CInputDevice*)src->m_devices.GetAt(1) : 0;
            m_joystick = d;
            m_device = d;
            break;
        }
        case 4: {
            CInputDevice* d =
                (src->m_devices.GetSize() > 2) ? (CInputDevice*)src->m_devices.GetAt(2) : 0;
            m_joystick = d;
            m_device = d;
            break;
        }
        case 5: {
            CInputDevice* d =
                (src->m_devices.GetSize() > 3) ? (CInputDevice*)src->m_devices.GetAt(3) : 0;
            m_joystick = d;
            m_device = d;
            break;
        }
        case 6: {
            m_keyboard = (CInputDevice*)src->m_deviceA;
            CInputDevice* d =
                (src->m_devices.GetSize() > 0) ? (CInputDevice*)src->m_devices.GetAt(0) : 0;
            m_joystick = d;
            m_deviceList =
                (SbzDeviceList*)src->AddControllerArr((i32)m_keyboard, (i32)d, 0, 0, 0, 0, 0);
            break;
        }
        case 8: {
            m_keyboard = (CInputDevice*)src->m_deviceA;
            CInputDevice* d =
                (src->m_devices.GetSize() > 0) ? (CInputDevice*)src->m_devices.GetAt(0) : 0;
            m_joystick = d;
            m_joystick2 = (CInputDevice*)src->m_deviceB;
            m_deviceList =
                (SbzDeviceList*)
                    src->AddControllerArr((i32)m_keyboard, (i32)d, (i32)m_joystick2, 0, 0, 0, 0);
            break;
        }
        case 7:
            m_keyboard = (CInputDevice*)src->m_deviceA;
            m_joystick2 = (CInputDevice*)src->m_deviceB;
            m_deviceList =
                (SbzDeviceList*)
                    src->AddControllerArr((i32)m_keyboard, (i32)m_joystick2, 0, 0, 0, 0, 0);
            break;
        case 0:
            m_keyboard = 0;
            m_joystick = 0;
            m_deviceList = 0;
            m_joystick2 = 0;
            m_device = 0;
            break;
    }
    m_mode = mode;
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
    if (m_device) {
        m_edgeKeys = m_device->m_edgeKeys;
        m_currentKeys = m_device->m_currentKeys;
    } else if (m_deviceList) {
        m_edgeKeys = m_keyboard->m_edgeKeys;
        m_currentKeys = m_keyboard->m_currentKeys;
        if (m_joystick) {
            m_edgeKeys |= m_joystick->m_edgeKeys;
            m_currentKeys |= m_joystick->m_currentKeys;
        }
        if (m_joystick2) {
            m_edgeKeys |= m_joystick2->m_edgeKeys;
            m_currentKeys |= m_joystick2->m_currentKeys;
        }
    }
    if (m_suppress) {
        m_edgeKeys = 0;
        m_currentKeys = 0;
    }
    m_latchedKeys = m_edgeKeys;
    return 1;
}

// ---------------------------------------------------------------------------
// StateMgrBZ::Reset (0x386b0; __thiscall, ret 0). Dispatch the per-device Reset
// (slot 5) over m_0 or, when none, the m_10 array, then clear the latched flags.
RVA(0x000386b0, 0x5d)
i32 StateMgrBZ::Reset() {
    CInputDevice* d = m_device;
    if (d) {
        d->ResetState();
    } else {
        SbzDeviceList* arr = m_deviceList;
        if (arr && arr->m_count > 0) {
            CInputDevice** p = &arr->m_elems[0];
            i32 i = 0;
            do {
                (*p)->ResetState();
                ++i;
                ++p;
            } while (i < arr->m_count);
        }
    }
    m_currentKeys = 0;
    m_edgeKeys = 0;
    m_latchedKeys = 0;
    return 1;
}

// StateMgrBZ::GetDirBits (0x38730; __thiscall, ret 0). Pack the four top-nibble
// direction bits of m_edgeKeys (0x10000000..0x80000000) into a 4-bit code.
RVA(0x00038730, 0x2e)
u8 StateMgrBZ::GetDirBits() {
    u32 k = m_edgeKeys;
    u8 r = 0;
    if (k & 0x10000000) {
        r = 1;
    }
    if (k & 0x20000000) {
        r |= 2;
    }
    if (k & 0x40000000) {
        r |= 4;
    }
    if (k & 0x80000000) {
        r |= 8;
    }
    return r;
}

// StateMgrBZ::SetDirBits (0x38770; __thiscall, ret 4). Unpack a 4-bit direction
// code into the top nibble of m_edgeKeys; clear m_currentKeys. Returns 1.
RVA(0x00038770, 0x40)
i32 StateMgrBZ::SetDirBits(i32 flags) {
    m_edgeKeys = 0;
    m_currentKeys = 0;
    if (flags & 1) {
        m_edgeKeys = 0x10000000;
    }
    if (flags & 2) {
        m_edgeKeys |= 0x20000000;
    }
    if (flags & 4) {
        m_edgeKeys |= 0x40000000;
    }
    if (flags & 8) {
        m_edgeKeys |= 0x80000000;
    }
    return 1;
}
