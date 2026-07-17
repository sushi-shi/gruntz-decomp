// InputDevice.cpp - the DinMgr2 module's low-level device COM wrappers.
// original TU: C:\Proj\DinMgr2\InputDevice.cpp (__FILE__-anchored: 12 asserts
// across 0x134cb0-0x13508f push the 0x619ed8 InputDevice.cpp literal, at source
// lines 50..485 - the whole file is these thin wrappers).
//
// Split out of our former DirectInputMgr2.cpp (wave1-E; see DinMgr2.cpp's header
// for the boundary evidence). Each wrapper does iface->Method(args...) so the
// retail `call *off(reg)` COM dispatch falls out; on a nonzero HRESULT it reports
// via DirectInputMgr2::GetErrorString (reloc-masked cross-file call into
// DinMgr2.cpp) with this file's __FILE__ literal + line.
#include <DinMgr2/DirectInputMgr2.h>
#include <rva.h>
#include <Win32.h>

// The __FILE__ string the wrappers pass to GetErrorString ($SG pooled constant
// at 0x619ed8, referenced by every assert in this file).
#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

// IID_IDirectInputDevice2A - the dxguid GUID constant in .rdata (0x5ef458) passed to
// the device QueryInterface. <dinput.h> declares it (EXTERN_C const GUID); we redeclare
// it only to pin its retail RVA so objdiff names the reloc target. DirectInputCreateA
// itself is the real <dinput.h> import decl (a direct `e8 rel32` to its ILT thunk,
// reloc-masked, like DirectSoundCreate / DirectDrawCreate - not an `ff 15 [IAT]`).
DATA(0x001ef458)
extern "C" const GUID IID_IDirectInputDevice2A;

// CInputDevice::Create (__thiscall, ret 0xc => 3 args). Caches the
// cooperative-level hwnd (m_hwnd), creates the device via
// IDirectInput::CreateDevice into m_device, then QueryInterfaces it to the v2 device
// interface (m_device2). Each COM failure is reported; returns whether m_device2 is non-null.
RVA(0x00134cb0, 0x94)
i32 CInputDevRoot::Create(IDirectInputA* di, const void* deviceGuid, HWND hwnd) {
    if (di == 0) {
        return 0;
    }
    if (hwnd == 0) {
        return 0;
    }
    m_hwnd = hwnd;
    i32 hr = di->CreateDevice(*(const GUID*)deviceGuid, &m_device, 0);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x32, hr);
        return 0;
    }
    if (m_device == 0) {
        return 0;
    }
    hr = m_device->QueryInterface(IID_IDirectInputDevice2A, (void**)&m_device2);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x3e, hr);
        return 0;
    }
    return m_device2 != 0;
}

// CInputDevice::ReleaseDevices (__thiscall, no args). Unacquires + Releases the QI'd
// device (m_device2), Releases the created device (m_device), then clears the device handles +
// the cached hwnd / state buffer pointer.
RVA(0x00134d50, 0x3b)
void CInputDevRoot::ReleaseDevices() {
    if (m_device2 != 0) {
        Unacquire();
        m_device2->Release();
    }
    if (m_device != 0) {
        m_device->Release();
    }
    m_device = 0;
    m_device2 = 0;
    m_hwnd = 0;
    m_stateBuffer = 0;
}

// CInputDevice::ReadState (__thiscall, no args). Refreshes the +0x2a0 snapshot via
// IDirectInputDevice::GetDeviceState (slot +0x24). On DIERR_INPUTLOST/NOTACQUIRED it
// re-Acquires and, if that succeeds, keeps the (stale) buffer; any other failure is
// reported and yields 0. Returns the +0x2a0 buffer pointer on success. (Helper that
// Poll calls; reloc-masked direct call from 0x133d00.)
RVA(0x00134d90, 0x60)
DeviceState* CInputDevice::ReadState() {
    if (m_stateBuffer == 0) {
        return 0;
    }
    i32 hr = m_device2->GetDeviceState(m_stateBufferSize, m_stateBuffer);
    if (hr != 0) {
        if (hr != static_cast<i32>(DIERR_INPUTLOST) && hr != static_cast<i32>(DIERR_NOTACQUIRED)) {
            DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x84, hr);
            return 0;
        }
        if (Acquire() == 0) {
            return 0;
        }
    }
    return m_stateBuffer;
}

// CInputDevRoot::GetDeviceInfo (__thiscall, no args). Fills the embedded
// DIDEVICEINSTANCEA via IDirectInputDevice::GetDeviceInfo (slot +0x3c); report on
// failure. Returns the descriptor pointer, or 0 on failure.
RVA(0x00134df0, 0x33)
DIDEVICEINSTANCEA* CInputDevRoot::GetDeviceInfo() {
    m_deviceInfo.dwSize = 0x244;
    i32 hr = m_device2->GetDeviceInfo(&m_deviceInfo);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0xa6, hr);
        return 0;
    }
    return &m_deviceInfo;
}

// CInputDevRoot::GetCapabilities (__thiscall, no args). Fills the embedded DIDEVCAPS
// via IDirectInputDevice::GetCapabilities (slot +0x0c); report on failure. Returns
// the descriptor pointer, or 0 on failure.
RVA(0x00134e30, 0x36)
DIDEVCAPS* CInputDevRoot::GetCapabilities() {
    m_caps.dwSize = 0x244;
    i32 hr = m_device2->GetCapabilities(&m_caps);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0xc7, hr);
        return 0;
    }
    return &m_caps;
}

// CInputDevRoot::GetProperty (__thiscall, ret 4 => 1 arg). Fills the embedded
// DIPROPHEADER via IDirectInputDevice::GetProperty (slot +0x14) for the given
// property GUID; report on failure. Returns the header pointer, or 0 on failure.
RVA(0x00134e70, 0x3f)
DIPROPHEADER* CInputDevRoot::GetProperty(REFGUID rguid) {
    m_prop.dwSize = 0x244;
    i32 hr = m_device2->GetProperty(rguid, &m_prop);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0xe8, hr);
        return 0;
    }
    return &m_prop;
}

// CInputDevice::SetDataFormat (__thiscall, ret 4 => 1 arg). Pass-through to
// IDirectInputDevice::SetDataFormat; report on failure.
RVA(0x00134eb0, 0x3b)
i32 CInputDevRoot::SetDataFormat(const void* fmt) {
    if (fmt == 0) {
        return 0;
    }
    i32 hr = m_device2->SetDataFormat((LPCDIDATAFORMAT)fmt);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x108, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetCooperativeLevel (__thiscall, ret 4 => 1 arg). Re-issues
// IDirectInputDevice::SetCooperativeLevel with the cached hwnd (m_hwnd) and the
// given flags; report on failure.
RVA(0x00134ef0, 0x3c)
i32 CInputDevRoot::SetCooperativeLevel(u32 flags) {
    i32 hr = m_device2->SetCooperativeLevel(m_hwnd, flags);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x128, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetProperty (__thiscall, ret 8 => 2 args). Pass-through to
// IDirectInputDevice::SetProperty; report on failure.
RVA(0x00134f30, 0x40)
i32 CInputDevRoot::SetProperty(REFGUID rguid, void* prop) {
    if (prop == 0) {
        return 0;
    }
    i32 hr = m_device2->SetProperty(rguid, (LPCDIPROPHEADER)prop);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x148, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetPropertyDword (__thiscall, ret 0x10 => 4 args). Builds a
// DIPROPDWORD on the stack (the standard DI scalar-property descriptor:
// {diph{dwSize=0x14, dwHeaderSize=0x10, dwObj, dwHow}, dwData}) and forwards it
// to SetProperty(rguid, &prop). The data fields are filled from the args first,
// then the two fixed size words.
RVA(0x00134f70, 0x40)
i32 CInputDevRoot::SetPropertyDword(REFGUID rguid, u32 dwObj, u32 dwHow, u32 dwData) {
    DIPROPDWORD prop; // {diph{dwSize,dwHeaderSize,dwObj,dwHow}, dwData}
    prop.diph.dwObj = dwObj;
    prop.diph.dwHow = dwHow;
    prop.dwData = dwData;
    prop.diph.dwSize = 0x14;
    prop.diph.dwHeaderSize = 0x10;
    return SetProperty(rguid, &prop);
}

// CInputDevice::Acquire (__thiscall, ret 0 => no args). Pass-through to
// IDirectInputDevice::Acquire; report on failure.
RVA(0x00134fb0, 0x29)
i32 CInputDevice::Acquire() {
    i32 hr = m_device2->Acquire();
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x17a, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::Unacquire (__thiscall, no args). IDirectInputDevice::Unacquire
// (slot +0x20); returns whether the HRESULT was success (0).
RVA(0x00134fe0, 0x13)
i32 CInputDevRoot::Unacquire() {
    i32 hr = m_device2->Unacquire();
    return hr == 0;
}

// CInputDevRoot::Escape (__thiscall, ret 4 => 1 arg). Pass-through to
// IDirectInputDevice2::Escape (slot +0x60); report on failure.
RVA(0x00135000, 0x3b)
i32 CInputDevRoot::Escape(void* data) {
    if (data == 0) {
        return 0;
    }
    i32 hr = m_device2->Escape((LPDIEFFESCAPE)data);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x1b8, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::PollDevice (__thiscall, no args). The PollJoystick pre-step:
// IDirectInputDevice2::Poll (slot +0x64) refreshes the buffered device. On success
// returns 1; on DIERR_INPUTLOST/NOTACQUIRED it re-Acquires (return 0 if that fails)
// and re-Polls once. A remaining nonzero HRESULT is reported through GetErrorString
// (InputDevice.cpp:0x1e5); returns whether the final HRESULT was success (0).
RVA(0x00135040, 0x65)
i32 CInputDevice::PollDevice() {
    i32 hr = m_device2->Poll();
    if (hr == 0) {
        return 1;
    }
    if (hr == static_cast<i32>(DIERR_INPUTLOST) || hr == static_cast<i32>(DIERR_NOTACQUIRED)) {
        if (Acquire() == 0) {
            return 0;
        }
        hr = m_device2->Poll();
    }
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x1e5, hr);
    }
    return hr == 0;
}
