#ifndef GRUNTZ_WWD_WWDFACTORYOBJECT_H
#define GRUNTZ_WWD_WWDFACTORYOBJECT_H

// WwdFactoryObject.h - the wide CWwdGameObject as seen by the factory/lifecycle
// method set (raw-offset access; campaign doctrine: the offsets are load-bearing,
// the field names are placeholders); the polymorphic slot dispatch is modeled via
// a typed vtable interface so the calls lower exactly. Hoisted from
// DDrawSubMgr.cpp (wave4-L): the finds/factories live in the H obj
// (src/Wwd/WwdObjMgr.cpp), the Release/Reset lifecycle pass + Notify in the I obj
// (src/Wwd/WwdFactoryObject.cpp).

#include <Ints.h>
#include <rva.h>

// (The former CWwdFactoryObject dispatch view of the wide object is DISSOLVED
// onto the real family (<Wwd/WwdGameObjectFamily.h>): its Vs20 was slot-8
// GetClassId, its Build slot-10 Setup28, and its Reset_/ReleaseSubs_ method set
// the per-kind Unload overrides (0x15b5d0/0x15b980/0x15bf00/0x15bc50/0x15c200);
// Notify_15b650 is CWwdGameObjectE::Notify_15b650.)

// (The former CWwdNotifier view of the +0x80/+0x88 notifier is DISSOLVED onto
// the canonical AnimWorkerObj (<DDrawMgr/AnimWorkerObj.h>): its "+0x10 cdecl
// callback" is the worker's m_notify.)
#include <DDrawMgr/AnimWorkerObj.h>

// The RECT overlap predicate pair (RectsOverlap defined in the I obj @0x15bfb0;
// BoxesOverlap in the H obj @0x15a130 - each declared here for the cross-obj call).
struct CDDrawRect {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};
SIZE_UNKNOWN(CDDrawRect);
i32 __stdcall RectsOverlap_15bfb0(CDDrawRect* a, CDDrawRect* b); // 0x15bfb0 (I obj)

#endif // GRUNTZ_WWD_WWDFACTORYOBJECT_H
