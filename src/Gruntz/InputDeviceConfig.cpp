#include <rva.h>
// InputDeviceConfig.cpp - CInputConfig::LoadInputDeviceConfig - maps the configured input
// device id (this->m_deviceId) to its display name (C:\Proj\Gruntz). Starts from
// the "None" default and, via a dense 5-way jump table on (m_deviceId - 1),
// overwrites it with "Keyboard" / "Joystick 1" .. "Joystick 4"; an out-of-range id
// keeps "None".
// Returns the name by value (copy-constructed into the caller's hidden return
// buffer); the local default-name CString is destroyed at the tail.
//
// Only offsets / code bytes are load-bearing; names are placeholders.
//
// FAITHFUL CARCASS (not strict-exact): the EH prologue, the "None" default-ctor,
// the dense 5-way jump table + all five device-name literal pushes, the converged
// operator=, the return copy-ctor into the hidden buffer, and the ~CString +
// EH-epilogue/ret-8 tail all match opcode-for-opcode. The residue is an extra
// EH-state-guarded conditional cleanup of a second CString temporary in the
// target's operator= return path (it pins the result in [esp+4] vs the default at
// [esp+8]); reproducing that two-temporary MFC shape is the remaining work. See
// config/units.toml.

// ---------------------------------------------------------------------------
// MFC CString (NAFXCW). The by-value return path needs the char* ctor, the
// assign-from-char* (operator=), the copy ctor (into the return buffer), and the
// dtor (drives the C++ EH unwind state). Their bodies are external/no-body so the
// call displacements reloc-mask against the matched NAFXCW routines.
// ---------------------------------------------------------------------------
#include <Gruntz/String.h>

// ---------------------------------------------------------------------------
// CInputConfig - the input-device option holder. Only the device-id discriminator
// at +0x14 is load-bearing.
// ---------------------------------------------------------------------------
class CInputConfig {
public:
    CString LoadInputDeviceConfig(i32 unused);

    char m_pad00[0x14];
    i32 m_deviceId; // +0x14  configured device id (1..5)
};

// ---------------------------------------------------------------------------
// CInputConfig::LoadInputDeviceConfig
// `unused` is the lone explicit stack arg (the ret 8 cleans the hidden CString
// return buffer + this 4-byte arg); the body does not read it.
RVA(0x000387c0, 0xc0)
CString CInputConfig::LoadInputDeviceConfig(i32 unused) {
    CString name("None");
    switch (m_deviceId) {
        case 1:
            name = "Keyboard";
            break;
        case 2:
            name = "Joystick 1";
            break;
        case 3:
            name = "Joystick 2";
            break;
        case 4:
            name = "Joystick 3";
            break;
        case 5:
            name = "Joystick 4";
            break;
    }
    return name;
}

SIZE_UNKNOWN(CInputConfig);
