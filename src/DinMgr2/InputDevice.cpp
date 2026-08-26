#include <rva.h>

#include <ComOutRef.h>
#include <DinMgr2/DirectInputMgr2.h>

#include <stddef.h>

#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

RVA(0x00134ec0, 0x94)
i32 CInputDevRoot::Create(IDirectInputA* di, const GUID* deviceGuid, HWND hwnd) {
    if (di == NULL) {
        return 0;
    }
    if (hwnd == NULL) {
        return 0;
    }
    m_hwnd = hwnd;
    i32 hr = di->CreateDevice(*deviceGuid, &m_device, NULL);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x32, hr);
        return 0;
    }
    if (m_device == NULL) {
        return 0;
    }
    ComOutRef<IDirectInputDevice2A> devOut;
    devOut.m_asTyped = &m_device2;
    hr = m_device->QueryInterface(IID_IDirectInputDevice2A, devOut.m_asVoid);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x3e, hr);
        return 0;
    }
    return m_device2 != NULL;
}

RVA(0x00134f60, 0x3b)
void CInputDevRoot::ReleaseDevices() {
    if (m_device2 != NULL) {
        Unacquire();
        m_device2->Release();
    }
    if (m_device != NULL) {
        m_device->Release();
    }
    m_device = NULL;
    m_device2 = NULL;
    m_hwnd = NULL;
    m_stateBuffer = NULL;
}

RVA(0x00134fa0, 0x60)
DeviceState* CInputDevRoot::ReadState() {
    if (m_stateBuffer == NULL) {
        return NULL;
    }
    i32 hr = m_device2->GetDeviceState(m_stateBufferSize, m_stateBuffer);
    if (hr != 0) {
        if (hr != static_cast<i32>(DIERR_INPUTLOST) && hr != static_cast<i32>(DIERR_NOTACQUIRED)) {
            DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x84, hr);
            return NULL;
        }
        if (Acquire() == 0) {
            return NULL;
        }
    }
    return m_stateBuffer;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135000, 0x33)
DIDEVICEINSTANCEA* CInputDevRoot::GetDeviceInfo() {
    m_deviceInfo.dwSize = sizeof(m_deviceInfo);
    i32 hr = m_device2->GetDeviceInfo(&m_deviceInfo);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0xa6, hr);
        return NULL;
    }
    return &m_deviceInfo;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135040, 0x36)
DIDEVCAPS* CInputDevRoot::GetCapabilities() {
    // Preserved bug: the DIDEVICEINSTANCEA size makes this DIDEVCAPS query fail.
    m_caps.dwSize = 0x244;
    i32 hr = m_device2->GetCapabilities(&m_caps);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0xc7, hr);
        return NULL;
    }
    return &m_caps;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135080, 0x3f)
DIPROPHEADER* CInputDevRoot::GetProperty(REFGUID rguid) {
    m_prop.dwSize = 0x244;
    i32 hr = m_device2->GetProperty(rguid, &m_prop);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0xe8, hr);
        return NULL;
    }
    return &m_prop;
}

RVA(0x001350c0, 0x3b)
i32 CInputDevRoot::SetDataFormat(LPCDIDATAFORMAT fmt) {
    if (fmt == NULL) {
        return 0;
    }
    i32 hr = m_device2->SetDataFormat(fmt);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x108, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135100, 0x3c)
i32 CInputDevRoot::SetCooperativeLevel(u32 flags) {
    i32 hr = m_device2->SetCooperativeLevel(m_hwnd, flags);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x128, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135140, 0x40)
i32 CInputDevRoot::SetProperty(REFGUID rguid, LPCDIPROPHEADER prop) {
    if (prop == NULL) {
        return 0;
    }
    i32 hr = m_device2->SetProperty(rguid, prop);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x148, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135180, 0x40)
i32 CInputDevRoot::SetPropertyDword(REFGUID rguid, u32 dwObj, u32 dwHow, u32 dwData) {
    DIPROPDWORD prop;
    prop.diph.dwObj = dwObj;
    prop.diph.dwHow = dwHow;
    prop.dwData = dwData;
    prop.diph.dwSize = sizeof(prop);
    prop.diph.dwHeaderSize = sizeof(prop.diph);
    return SetProperty(rguid, &prop.diph);
}

RVA(0x001351c0, 0x29)
i32 CInputDevRoot::Acquire() {
    i32 hr = m_device2->Acquire();
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x17a, hr);
        return 0;
    }
    return 1;
}

RVA(0x001351f0, 0x13)
i32 CInputDevRoot::Unacquire() {
    i32 hr = m_device2->Unacquire();
    return hr == 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135210, 0x3b)
i32 CInputDevRoot::Escape(LPDIEFFESCAPE data) {
    if (data == NULL) {
        return 0;
    }
    i32 hr = m_device2->Escape(data);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x1b8, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135250, 0x65)
i32 CInputDevRoot::PollDevice() {
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
