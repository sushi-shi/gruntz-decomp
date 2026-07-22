#ifndef GRUNTZ_CFIXEDPTRARRAY32_H
#define GRUNTZ_CFIXEDPTRARRAY32_H

#include <Ints.h>
#include <rva.h>

class CInputDevBase; // <DinMgr2/DirectInputMgr2.h> - the held device configs

// The game-controller poll list (0x88 B: 8 + 32*4). CGruntzMgr::Run builds it
// from DirectInputMgr2::m_devices (AddControllerArr @0x133260 - the entries
// ALIAS the manager's device objects; ctor-stamp-proven CDeviceConfigC joystick
// leaves, new(0x2b8) + ??_7CDeviceConfigC @0x1ef658) and publishes it as
// g_actorList; the attract-family states Poll() it per frame and read the
// press-edge flags (m_currentKeys: 0x100 = button 9 "exit attract"). (Ex the
// "AttractActor/AttractActorList" fake dispatch view - the "actors" were the
// joysticks.)
class CFixedPtrArray32 {
public:
    void Clear();
    i32 FillFrom(CInputDevBase** src, i32 n, i32 unused);
    i32 Add(CInputDevBase* item);

    i32 m_00;                   // +0x00 flag/tag (reset to 0 by FillFrom, untouched by ctor)
    i32 m_count;                // +0x04
    CInputDevBase* m_items[32]; // +0x08
};
SIZE_UNKNOWN();

// The published poll list (0x245574; written once by CGruntzMgr::Run).
extern "C" CFixedPtrArray32* g_actorList;

#endif // GRUNTZ_CFIXEDPTRARRAY32_H
