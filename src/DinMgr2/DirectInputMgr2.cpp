// DirectInputMgr2.cpp - the DinMgr2 module's DirectInput manager group
// (C:\Proj\DinMgr2\). Two classes share one TU because both funnel failed
// HRESULTs through DirectInputMgr2::GetErrorString (the DInput sibling of
// CDirectDrawMgr::GetErrorString / DDrawMgr's DIRSURF.CPP):
//
//   * DirectInputMgr2 (DinMgr2.cpp) - the device manager: GetErrorString plus
//     the DirectInputCreateA bring-up (Create) and the EnumDevices wrapper.
//   * CInputDevice (InputDevice.cpp) - thin IDirectInputDevice wrapper thunks
//     (Create/SetDataFormat/SetCooperativeLevel/SetProperty/Acquire). Each does
//     iface->vtbl->Method(iface, args...) so the retail `call *off(reg)` COM
//     dispatch falls out; on a nonzero HRESULT it reports via GetErrorString.
//
// GetErrorString maps a DirectInput error code to a "<DIERR_NAME> (<code>) -
// <description>" string and, depending on three reporting-mode globals, beeps,
// formats it and/or pops a MessageBox. `this` is unused; the work is driven
// entirely by the (file, line, hr) arguments the call sites push.
//
// Same archetype as CDirectDrawMgr::GetErrorString, with three differences in
// the retail body: (1) the DInput case set / strings, (2) the case dispatch is
// a cmp/je binary-search tree (sparse DIERR values), not a jump table, and
// (3) the log path only sprintf-formats the line - there is NO separate logger
// call (the DDraw version's DDrawLogLine has no DinMgr2 counterpart here).
//
// The function self-identifies its module via the strings it references (every
// DIERR_* name + "DirectInputMgr2"); names of locals are placeholders, the
// switch case VALUES and string contents are load-bearing.
#include <DinMgr2/DirectInputMgr2.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA + BOOL/HWND/LPCSTR/UINT come from the real
// <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's uType is
// MB_ICONEXCLAMATION (0x30); the old hand-rolled macro mislabeled that value as
// the "hand" icon, whose real windows.h value is 0x10.
#include <Win32.h>

// The __FILE__ strings the wrappers pass to GetErrorString - two source-path
// $SG pooled constants ($SG at 0x6199bc / 0x619ed8) referenced across the run.
#define DINMGR2_FILE "C:\\Proj\\DinMgr2\\DinMgr2.cpp"
#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

// DINPUT.dll DirectInputCreateA - called via a direct `e8 rel32` to its
// incremental-link thunk (the thunk is `jmp ds:[IAT]`); reloc-masked, like
// DirectSoundCreate / DirectDrawCreate, not an `ff 15 [IAT]` indirect.
extern "C" long __stdcall
DirectInputCreateA(void* hinst, unsigned long version, IDirectInputZ** ppDI, void* punkOuter);

// IID_IDirectInputDevice2A - a dxguid GUID constant in .rdata (0x5ef458),
// passed to the device QueryInterface. Reloc-masked DATA() extern.
DATA(0x1ef458)
extern const unsigned char IID_IDirectInputDevice2A[16]; // 0x5ef458

// The static DIEnumDevicesCallbackA the EnumDevices wrapper passes by address
// (0x532fc0, a separate DinMgr2.cpp callback not yet matched). Referenced only
// by address so the `push offset` operand carries a reloc-masked DIR32.
extern "C" int __stdcall DinEnumDevicesCallback(const void* instance, void* ref); // 0x132fc0

// Reporting-mode globals (live in .data). g_logEnabled drives the format-line
// path, g_msgBoxEnabled the MessageBox path; g_beepEnabled gates the startup
// beep, g_thirdEnabled is a third "any output wanted" gate checked at entry.
DATA(0x253aac)
extern "C" int g_beepEnabled; // 0x653aac
DATA(0x253aa4)
extern "C" int g_logEnabled; // 0x653aa4
DATA(0x253aa8)
extern "C" int g_msgBoxEnabled; // 0x653aa8
DATA(0x253ab0)
extern "C" int g_thirdEnabled; // 0x653ab0

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x2293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// ===========================================================================
// DirectInputMgr2 (DinMgr2.cpp) - the device manager.
// ===========================================================================

// DirectInputMgr2::Create (__thiscall, ret 0xc => 3 args). Creates the DInput
// object via DirectInputCreateA (version 0x500) into m_0, reporting + bailing on
// failure; caches owner/hinst/flags, then runs the three sub-initializers, each
// gated on a flags bit being CLEAR (InitA unless bit 4, InitB unless bit 2,
// EnumGameControllers unless bit 8) and short-circuiting to 0 if a step fails.
RVA(0x132ce0, 0xae)
int DirectInputMgr2::Create(void* owner, void* hinst, unsigned long flags) {
    if (owner == 0) {
        return 0;
    }
    if (hinst == 0) {
        return 0;
    }
    int hr = DirectInputCreateA(hinst, 0x500, &m_0, 0);
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0x32, hr);
        return 0;
    }
    m_4 = owner;
    m_8 = hinst;
    m_c = flags;
    if ((flags & 4) == 0) {
        if (InitA(flags) == 0) {
            return 0;
        }
    }
    if ((m_c & 2) == 0) {
        if (InitB(flags) == 0) {
            return 0;
        }
    }
    if ((m_c & 8) == 0) {
        if (EnumGameControllers(flags) == 0) {
            return 0;
        }
    }
    return 1;
}

// DirectInputMgr2::EnumGameControllers (__thiscall, ret 4 => 1 arg; the arg is
// unused). When the DInput object exists, enumerates game controllers via
// IDirectInput::EnumDevices(devType=4, callback, ref=this, flags=1); reports a
// failed HRESULT and returns 0, else 1.
RVA(0x132f80, 0x3d)
int DirectInputMgr2::EnumGameControllers(unsigned long) {
    IDirectInputZ* di = m_0;
    if (di == 0) {
        return 0;
    }
    long hr = di->vtbl->EnumDevices(di, 4, (void*)&DinEnumDevicesCallback, (void*)this, 1);
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0xfb, hr);
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// DirectInputMgr2::GetErrorString
RVA(0x133590, 0x5be)
void DirectInputMgr2::GetErrorString(char* file, int line, long hr) {
    char szCode[64];  // error-code name
    char szMsg[256];  // description
    char szLine[512]; // formatted output line

    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled) {
        return;
    }

    int code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case (int)0x80004001:
            strcpy(szCode, "DIERR_UNSUPPORTED");
            strcpy(szMsg, "The function called is not supported at this time.");
            break;
        case (int)0x80004002:
            strcpy(szCode, "DIERR_NOINTERFACE");
            strcpy(szMsg, "The specified interface is not supported by the object.");
            break;
        case (int)0x80004005:
            strcpy(szCode, "DIERR_GENERIC");
            strcpy(szMsg, "An undetermined error occured inside the DInput subsystem.");
            break;
        case (int)0x80040154:
            strcpy(szCode, "DIERR_DEVICENOTREG");
            strcpy(
                szMsg,
                "The device or device instance or effect is not registered with DirectInput."
            );
            break;
        case (int)0x80040200:
            strcpy(szCode, "DIERR_INSUFFICIENTPRIVS");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80070002:
            strcpy(szCode, "DIERR_NOTFOUND");
            strcpy(szMsg, "The requested object does not exist.");
            break;
        case (int)0x80070005:
            strcpy(szCode, "DIERR_READONLY");
            strcpy(szMsg, "The specified property cannot be changed.");
            break;
        case (int)0x8007000c:
            strcpy(szCode, "DIERR_NOTACQUIRED");
            strcpy(szMsg, "The operation cannot be performed unless the device is acquired.");
            break;
        case (int)0x8007000e:
            strcpy(szCode, "DIERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80070015:
            strcpy(szCode, "DIERR_NOTINITIALIZED");
            strcpy(szMsg, "This object has not been initialized.");
            break;
        case (int)0x8007001e:
            strcpy(szCode, "DIERR_INPUTLOST");
            strcpy(szMsg, "Access to the device has been lost.  It must be re-acquired.");
            break;
        case (int)0x80070057:
            strcpy(szCode, "DIERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80070077:
            strcpy(szCode, "DIERR_BADDRIVERVER");
            strcpy(
                szMsg,
                "The object could not be created due to an incompatible driver version or "
                "mismatched or incomplete driver components."
            );
            break;
        case (int)0x800700aa:
            strcpy(szCode, "DIERR_ACQUIRED");
            strcpy(szMsg, "The operation cannot be performed while the device is acquired.");
            break;
        case (int)0x8007047e:
            strcpy(szCode, "DIERR_OLDDIRECTINPUTVERSION");
            strcpy(szMsg, "The application requires a newer version of DirectInput.");
            break;
        case (int)0x800704df:
            strcpy(szCode, "DIERR_ALREADYINITIALIZED");
            strcpy(szMsg, "This object is already initialized.");
            break;
        case 0:
            strcpy(szCode, "DD_OK");
            strcpy(szMsg, "No error");
            break;
        default:
            break;
    }

    if (g_logEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
    }
    if (g_msgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA((HWND)0, szLine, "DirectInputMgr2", MB_ICONEXCLAMATION);
    }
}

// ===========================================================================
// CInputDevice (InputDevice.cpp) - the IDirectInputDevice wrapper thunks. Each
// routes a failed HRESULT through DirectInputMgr2::GetErrorString (the InputDevice.cpp
// $SG __FILE__) and returns a 0/1 success bool.
// ===========================================================================

// CInputDevice::Create (__thiscall, ret 0xc => 3 args). Caches the
// cooperative-level hwnd (m_29c), creates the device via
// IDirectInput::CreateDevice into m_4, then QueryInterfaces it to the v2 device
// interface (m_8). Each COM failure is reported; returns whether m_8 is non-null.
RVA(0x134cb0, 0x94)
int CInputDevice::Create(IDirectInputZ* di, const void* deviceGuid, void* hwnd) {
    if (di == 0) {
        return 0;
    }
    if (hwnd == 0) {
        return 0;
    }
    m_29c = hwnd;
    int hr = di->vtbl->CreateDevice(di, deviceGuid, &m_4, 0);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x32, hr);
        return 0;
    }
    if (m_4 == 0) {
        return 0;
    }
    hr = m_4->vtbl->QueryInterface(m_4, IID_IDirectInputDevice2A, (void**)&m_8);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x3e, hr);
        return 0;
    }
    return m_8 != 0;
}

// CInputDevice::SetDataFormat (__thiscall, ret 4 => 1 arg). Pass-through to
// IDirectInputDevice::SetDataFormat; report on failure.
RVA(0x134eb0, 0x3b)
int CInputDevice::SetDataFormat(void* fmt) {
    if (fmt == 0) {
        return 0;
    }
    long hr = m_8->vtbl->SetDataFormat(m_8, fmt);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x108, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetCooperativeLevel (__thiscall, ret 4 => 1 arg). Re-issues
// IDirectInputDevice::SetCooperativeLevel with the cached hwnd (m_29c) and the
// given flags; report on failure.
RVA(0x134ef0, 0x3c)
int CInputDevice::SetCooperativeLevel(unsigned long flags) {
    long hr = m_8->vtbl->SetCooperativeLevel(m_8, m_29c, flags);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x128, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetProperty (__thiscall, ret 8 => 2 args). Pass-through to
// IDirectInputDevice::SetProperty; report on failure.
RVA(0x134f30, 0x40)
int CInputDevice::SetProperty(const void* rguid, void* prop) {
    if (prop == 0) {
        return 0;
    }
    long hr = m_8->vtbl->SetProperty(m_8, rguid, prop);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x148, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::Acquire (__thiscall, ret 0 => no args). Pass-through to
// IDirectInputDevice::Acquire; report on failure.
RVA(0x134fb0, 0x29)
int CInputDevice::Acquire() {
    long hr = m_8->vtbl->Acquire(m_8);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x17a, hr);
        return 0;
    }
    return 1;
}
