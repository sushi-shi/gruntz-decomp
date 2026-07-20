#include <DinMgr2/DirectInputMgr2.h>
#include <rva.h>
#include <Win32.h>

#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

DATA(0x001ef458)
extern "C" const GUID IID_IDirectInputDevice2A;

RVA(0x00134cb0, 0x94)
i32 CInputDevRoot::Create(IDirectInputA* di, const void* deviceGuid, HWND hwnd) {
    if (di == 0) {
        return 0;
    }
    if (hwnd == 0) {
        return 0;
    }
    m_hwnd = hwnd;
    i32 hr = di->CreateDevice(*static_cast<const GUID*>(deviceGuid), &m_device, 0);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x32, hr);
        return 0;
    }
    if (m_device == 0) {
        return 0;
    }
    hr = m_device->QueryInterface(IID_IDirectInputDevice2A, reinterpret_cast<void**>(&m_device2));
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x3e, hr);
        return 0;
    }
    return m_device2 != 0;
}

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

RVA(0x00134eb0, 0x3b)
i32 CInputDevRoot::SetDataFormat(const void* fmt) {
    if (fmt == 0) {
        return 0;
    }
    i32 hr = m_device2->SetDataFormat(static_cast<LPCDIDATAFORMAT>(fmt));
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x108, hr);
        return 0;
    }
    return 1;
}

RVA(0x00134ef0, 0x3c)
i32 CInputDevRoot::SetCooperativeLevel(u32 flags) {
    i32 hr = m_device2->SetCooperativeLevel(m_hwnd, flags);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x128, hr);
        return 0;
    }
    return 1;
}

RVA(0x00134f30, 0x40)
i32 CInputDevRoot::SetProperty(REFGUID rguid, void* prop) {
    if (prop == 0) {
        return 0;
    }
    i32 hr = m_device2->SetProperty(rguid, static_cast<LPCDIPROPHEADER>(prop));
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x148, hr);
        return 0;
    }
    return 1;
}

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

RVA(0x00134fb0, 0x29)
i32 CInputDevice::Acquire() {
    i32 hr = m_device2->Acquire();
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x17a, hr);
        return 0;
    }
    return 1;
}

RVA(0x00134fe0, 0x13)
i32 CInputDevRoot::Unacquire() {
    i32 hr = m_device2->Unacquire();
    return hr == 0;
}

RVA(0x00135000, 0x3b)
i32 CInputDevRoot::Escape(void* data) {
    if (data == 0) {
        return 0;
    }
    i32 hr = m_device2->Escape(static_cast<LPDIEFFESCAPE>(data));
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x1b8, hr);
        return 0;
    }
    return 1;
}

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
