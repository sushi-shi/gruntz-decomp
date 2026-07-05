// SfManager.h - the Gruntz-side runtime binding for the SFMAN32.DLL SoundFont
// Master Manager. The device interface itself is the REAL vendored SDK type
// SFMANL101API (<SFMAN.H>, Creative Technology / E-mu, rev 1.01) - this header adds
// only the two genuinely-Gruntz-side details the SDK header does not carry:
//   * SfManagerFactory - the shape of the DLL's single "SFManager" DATA export
//     (SF_FUNCTION_TABLE_NAME): a pointer-to-factory-fn-ptr the engine
//     LoadLibraryA/GetProcAddress's; (*factory)(flags, &recv) fills the device.
//   * the +0x34 slot (SF_GetLoadedBankPathname) is invoked 2-arg by the retail
//     SfDeviceInitKeys (0xf8ec0: push loc; push idx; call [ecx+0x34]; add esp,8) -
//     the SDK 1.01 prototype's 3rd PSFBUFFEROBJECT arg omitted. Reinterpret the slot
//     at that ONE call site; the SDK type stays byte-identical elsewhere.
// (Replaces the former vendor/sfman-1.01/sfman32.h dispatch-view shadow, which was a
//  reconstruction of SFMANL101API mislabelled as a vendored header.)
#ifndef GRUNTZ_DSNDMGR_SFMANAGER_H
#define GRUNTZ_DSNDMGR_SFMANAGER_H

#include <SFMAN.H> // the real SFMANL101API device interface + SFDEVINDEX + names

// The 2-arg form of the +0x34 slot the retail under-call uses.
typedef LRESULT (*SfGetLoadedBankPathname2)(SFDEVINDEX, PSFMIDILOCATION);

// The "SFManager" data export: (*factory)(flags, &deviceRecv) -> 0 on success.
typedef int(__cdecl* SfManagerFactory)(int flags, SFMANL101API** out);

#endif // GRUNTZ_DSNDMGR_SFMANAGER_H
