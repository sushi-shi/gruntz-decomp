#ifndef MANAGERS_DIRECTINPUTMGR2_H
#define MANAGERS_DIRECTINPUTMGR2_H

/*
 * DirectInputMgr2 — DirectInput keyboard/mouse/joystick manager.
 *
 * Leaked source TU:  C:\Proj\DinMgr2\DinMgr2.cpp
 *   companion:       InputDevice.cpp
 *
 * Provenance: the names DirectInputMgr2 / InputDevice are mined from strings
 * (STRINGS_ANALYSIS.md §17 manager map). NOT in RTTI — no @rtti tags. NO layout
 * recovered — all @todo. The "2" suggests a second-generation rewrite of the
 * input manager. Error: "The application requires a newer version of DirectInput."
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>
#include <dinput.h>

/* InputDevice — a single DirectInput device wrapper (InputDevice.cpp). */
class InputDevice
{
public:
    //@size: unknown @todo
    //@todo: wraps IDirectInputDevice (keyboard/mouse/joystick). Layout unknown.
    //       DIERR_* stringify table is embedded (ACQUIRED/INPUTLOST/NOTFOUND/
    //       READONLY/OLDDIRECTINPUTVERSION).
};

/*
 * DirectInputMgr2 — the input manager (primary class of DinMgr2.cpp).
 * Name source: strings/manager-map only — no RTTI.
 */
class DirectInputMgr2
{
public:
    //@size: unknown @todo
    //@todo: device enumeration/acquire/poll unknown. "Disable Joystick" registry
    //       flag gates joystick init.
};

#endif /* MANAGERS_DIRECTINPUTMGR2_H */
