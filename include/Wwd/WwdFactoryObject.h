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

class CWwdFactoryObject {
public:
    virtual void Vs00();
    virtual ~CWwdFactoryObject(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void Vs08();
    virtual void Vs0C();
    virtual void Vs10();
    virtual void Vs14();
    virtual void Vs18();
    virtual void Vs1C();
    virtual i32 Vs20(); // slot 8 (@+0x20) - object type tag (== 5 in Find_15a8c0)
    virtual void Vs24();
    virtual i32 Build(i32 a, i32 b, i32 c, i32 d); // +0x28 deserialize/build

    // Reset/clear the wide object: release the four +0x7c..0x90 sub-objects and
    // reset the geometry/status fields. Documented raw-offset access. (I obj.)
    void Reset_15b980(); // 0x15b980
    void Reset_15bf00(); // 0x15bf00
    // 0x166810 base reset is CWwdGameObjectB::Clear_166810 (same wide object; called
    // through a CWwdGameObjectB* view in Reset_15bf00) - no fake local placeholder.

    // Release the four +0x7c..0x90 sub-objects + re-seed status (the dtor's
    // shared "drop members" helper; two identical instantiations + a +0x18c
    // variant). Raw-offset access (documented). (I obj.)
    void ReleaseSubs_15b5d0();
    void ReleaseSubs_15bc50();
    void ReleaseSubsClearKey_15c200();
    // 0x15b650: tick/notify - under flag 0x8 decrement the +0x128 budget (latch
    // the +0x7c sub-object's error on underflow); else hand `p` to the +0x80
    // notifier's +0x10 cdecl callback. (I obj.)
    void Notify_15b650(void* p);
};
SIZE_UNKNOWN(CWwdFactoryObject);

// The +0x80 notifier: a cdecl callback pointer at +0x10 invoked with the owner.
class CWwdNotifier {
public:
    char m_pad00[0x10];        // +0x00..0x0f
    void (*m_callback)(void*); // +0x10
};
SIZE_UNKNOWN(CWwdNotifier);

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
