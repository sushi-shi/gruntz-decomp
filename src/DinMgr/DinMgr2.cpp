// DinMgr2.cpp - DirectInputMgr2 error-string mapper.
// Maps DI HRESULT codes to symbol names + descriptions, outputs via the
// configured reporting channels (OutputDebugStringA / MessageBoxA).
//
// @address: 0x133590
// @size:    0x5b3
#include "DinMgr2.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Module-level globals (pinned @data addresses).
// ---------------------------------------------------------------------------
int  g_dinBeep      = 0;   // @0x653aac
int  g_dinDebug     = 0;   // @0x653aa4
int  g_dinMsgBox    = 0;   // @0x653aa8
int  g_dinOutputDbg = 0;   // @0x653ab0
char g_szDinModule[] = "DirectInputMgr2";  // @0x6199d8

// Format strings (used by EngFormat, reloc-masked).
static const char s_fmtDebug[]      = "%s, line %i: %s (%i) - %s\n";   // @0x619a20
static const char s_fmtDebugShort[] = "%s (%i) - %s\n";                // @0x619a10
static const char s_fmtMsgBox[]     = "%s, line %i\n\n%s (%i)\n\n%s";  // @0x6199f4
static const char s_fmtMsgBoxShort[]= "%s (%i)\n\n%s";                 // @0x6199e8

// Utility strings
static const char s_unknownErrMsg[] = "Unknown Error Message";   // @0x619ec0
static const char s_unknownErrCode[]= "Unknown Error Code";      // @0x619eac
static const char s_empty[]         = "";                        // @0x6293f4
static const char s_noError[]       = "No error";                // @0x619ad4
static const char s_noMsg[]         = "No message";              // @0x619d48
static const char s_ddOk[]          = "DD_OK";                   // @0x619ae0

// DIERR symbol names + descriptions
static const char s_dierrGeneric[]           = "DIERR_GENERIC";             // @0x619e9c
static const char s_descGeneric[]            = "An undetermined error occured inside the DInput subsystem."; // @0x619e60
static const char s_dierrNoInterface[]       = "DIERR_NOINTERFACE";        // @0x619e4c
static const char s_descNoInterface[]        = "The specified interface is not supported by the object."; // @0x619e14
static const char s_dierrUnsupported[]       = "DIERR_UNSUPPORTED";        // @0x619e00
static const char s_descUnsupported[]        = "The function called is not supported at this time."; // @0x619dcc
static const char s_dierrDeviceNotReg[]      = "DIERR_DEVICENOTREG";       // @0x619db8
static const char s_descDeviceNotReg[]       = "The device or device instance or effect is not registered with DirectInput."; // @0x619d6c
static const char s_dierrInsufficientPrivs[] = "DIERR_INSUFFICIENTPRIVS";  // @0x619d54
static const char s_dierrNotFound[]          = "DIERR_NOTFOUND";           // @0x619d38
static const char s_descNotFound[]           = "The requested object does not exist."; // @0x619d10
static const char s_dierrReadOnly[]          = "DIERR_READONLY";           // @0x619d00
static const char s_descReadOnly[]           = "The specified property cannot be changed."; // @0x619cd4
static const char s_dierrNotAcquired[]       = "DIERR_NOTACQUIRED";        // @0x619cc0
static const char s_descNotAcquired[]        = "The operation cannot be performed unless the device is acquired."; // @0x619c7c
static const char s_dierrOutOfMemory[]       = "DIERR_OUTOFMEMORY";        // @0x619c68
static const char s_dierrNotInitialized[]    = "DIERR_NOTINITIALIZED";     // @0x619c50
static const char s_descNotInitialized[]     = "This object has not been initialized."; // @0x619c28
static const char s_dierrInputLost[]         = "DIERR_INPUTLOST";          // @0x619c18
static const char s_descInputLost[]          = "Access to the device has been lost.  It must be re-acquired."; // @0x619bd8
static const char s_dierrInvalidParam[]      = "DIERR_INVALIDPARAM";       // @0x619bc4
static const char s_dierrBadDriverVer[]      = "DIERR_BADDRIVERVER";       // @0x619bb0
static const char s_descBadDriverVer[]       = "The object could not be created due to an incompatible driver version or mismatched or incomplete driver components."; // @0x619b38
static const char s_dierrAcquired[]          = "DIERR_ACQUIRED";           // @0x619b28
static const char s_descAcquired[]           = "The operation cannot be performed while the device is acquired."; // @0x619ae8
static const char s_dierrAlreadyInit[]       = "DIERR_ALREADYINITIALIZED"; // @0x619ab8
static const char s_descAlreadyInit[]        = "This object is already initialized."; // @0x619a94
static const char s_dierrOldDIVersion[]      = "DIERR_OLDDIRECTINPUTVERSION"; // @0x619a78
static const char s_descOldDIVersion[]       = "The application requires a newer version of DirectInput."; // @0x619a3c

// ---------------------------------------------------------------------------
// @address: 0x133590
// @size:    0x5b3
// ---------------------------------------------------------------------------
void DirectInputMgr2::GetErrorString(const char *file, int line, long hr)
{
    char sym[0x40];    // [esp+0x10]
    char desc[0x100];  // [esp+0x50]
    char out[0x100];   // [esp+0x150]

    int hrLow = hr & 0xFFFF;

    // Optional beep
    if (g_dinBeep)
        MessageBeep(0x30);

    // Early exit if no output channel active
    if (!g_dinDebug && !g_dinMsgBox && !g_dinOutputDbg)
        return;

    // Default strings
    strcpy(desc, s_unknownErrMsg);
    EngFormat(sym, s_unknownErrCode);
    strcpy(out, s_empty);

    // Map HRESULT -> symbol name + description
    if (hr == 0x80004005) {                              // DIERR_GENERIC
        strcpy(sym, s_dierrGeneric);
        strcpy(desc, s_descGeneric);
    } else if (hr == 0x80004002) {                       // DIERR_NOINTERFACE
        strcpy(sym, s_dierrNoInterface);
        strcpy(desc, s_descNoInterface);
    } else if (hr == 0x80004001) {                       // DIERR_UNSUPPORTED
        strcpy(sym, s_dierrUnsupported);
        strcpy(desc, s_descUnsupported);
    } else if (hr == 0x80040154) {                       // DIERR_DEVICENOTREG
        strcpy(sym, s_dierrDeviceNotReg);
        strcpy(desc, s_descDeviceNotReg);
    } else if (hr == 0x80040200) {                       // DIERR_INSUFFICIENTPRIVS
        strcpy(sym, s_dierrInsufficientPrivs);
        strcpy(desc, s_noMsg);
    } else if (hr == 0x80070002) {                       // DIERR_NOTFOUND
        strcpy(sym, s_dierrNotFound);
        strcpy(desc, s_descNotFound);
    } else if (hr == 0x80070005) {                       // DIERR_READONLY
        strcpy(sym, s_dierrReadOnly);
        strcpy(desc, s_descReadOnly);
    } else if (hr == 0x8007000c) {                       // DIERR_NOTACQUIRED
        strcpy(sym, s_dierrNotAcquired);
        strcpy(desc, s_descNotAcquired);
    } else if (hr == 0x8007000e) {                       // DIERR_OUTOFMEMORY
        strcpy(sym, s_dierrOutOfMemory);
        strcpy(desc, s_noMsg);
    } else if (hr == 0x80070015) {                       // DIERR_NOTINITIALIZED
        strcpy(sym, s_dierrNotInitialized);
        strcpy(desc, s_descNotInitialized);
    } else if (hr == 0x8007001e) {                       // DIERR_INPUTLOST
        strcpy(sym, s_dierrInputLost);
        strcpy(desc, s_descInputLost);
    } else if (hr == 0x80070057) {                       // DIERR_INVALIDPARAM
        strcpy(sym, s_dierrInvalidParam);
        strcpy(desc, s_noMsg);
    } else if (hr == 0x80070077) {                       // DIERR_BADDRIVERVER
        strcpy(sym, s_dierrBadDriverVer);
        strcpy(desc, s_descBadDriverVer);
    } else if (hr == 0x800700aa) {                       // DIERR_ACQUIRED
        strcpy(sym, s_dierrAcquired);
        strcpy(desc, s_descAcquired);
    } else if (hr == 0x800704df) {                       // DIERR_ALREADYINITIALIZED
        strcpy(sym, s_dierrAlreadyInit);
        strcpy(desc, s_descAlreadyInit);
    } else if (hr == 0x8007047e) {                       // DIERR_OLDDIRECTINPUTVERSION
        strcpy(sym, s_dierrOldDIVersion);
        strcpy(desc, s_descOldDIVersion);
    } else if (hr == 0) {                                // DI_OK / DD_OK
        strcpy(sym, s_ddOk);
        strcpy(desc, s_noError);
    }

    // Output via debugger channel
    if (g_dinDebug) {
        if (file && line > 0)
            EngFormat(out, s_fmtDebug, file, line, sym, hrLow, desc);
        else
            EngFormat(out, s_fmtDebugShort, sym, hrLow, desc);
        OutputDebugStringA(out);
    }

    // Output via message box channel
    if (g_dinMsgBox) {
        if (file && line > 0)
            EngFormat(out, s_fmtMsgBox, file, line, sym, hrLow, desc);
        else
            EngFormat(out, s_fmtMsgBoxShort, sym, hrLow, desc);
        MessageBoxA(0, out, g_szDinModule, 0x30);
    }
}
