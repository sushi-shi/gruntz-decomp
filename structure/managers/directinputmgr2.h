#ifndef MANAGERS_DIRECTINPUTMGR2_H
#define MANAGERS_DIRECTINPUTMGR2_H

/*
 * DirectInputMgr2 — DirectInput keyboard/mouse/joystick manager.
 *
 * Leaked source TU:  C:\Proj\DinMgr2\DinMgr2.cpp   companion: InputDevice.cpp
 *
 * Provenance: the names DirectInputMgr2 / InputDevice are mined from strings
 * (STRINGS_ANALYSIS.md §17 manager map). NOT in RTTI. NO layout recovered —
 * name-only stubs. The "2" suggests a second-generation rewrite of the input
 * manager. Error: "The application requires a newer version of DirectInput."
 * DIERR_* stringify table is embedded (ACQUIRED/INPUTLOST/NOTFOUND/READONLY/
 * OLDDIRECTINPUTVERSION). The "Disable Joystick" registry flag gates joystick init.
 */

/* InputDevice — a single DirectInput device wrapper (InputDevice.cpp). */
class InputDevice    { /* wraps IDirectInputDevice; layout unknown */ };

/* DirectInputMgr2 — the input manager (primary class of DinMgr2.cpp). */
class DirectInputMgr2 { /* device enumeration/acquire/poll; layout unknown */ };

#endif /* MANAGERS_DIRECTINPUTMGR2_H */
