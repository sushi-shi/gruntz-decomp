// CWapObj.h - the WAP engine's abstract intermediate base between Wap::CObject
// (the 5-slot grand-base, vtable @0x5e8cb4) and the surface/image/loadable game
// objects. vtable_hierarchy --tree/--audit proves it: EVERY object in the
// CImage / CLoadable / CDDrawWorker / CDrawSubWorker / CResolveNode /
// CAni* / LeafElementObj / AnimWorkerObj cluster carries the SAME two extra
// virtual slots directly after CObject's five:
//
//   slot 5 (@+0x14)  IsLoaded  default @0x0013b6 -> body 0x0d5dc0:
//                      `mov edx,[ecx+0x10]; xor eax,eax; test edx,edx; setg al`
//                      == `return this->m_10 > 0` (the first DERIVED field is a
//                      count/size; loaded iff > 0). Most derived classes override.
//   slot 6 (@+0x18)  IsReady   default @0x001c08 -> body 0x0d5da0: `mov eax,1; ret`
//                      == `return 1` (always ready). CImageSet1/2/3 override it.
//
// Only CImage inherits BOTH defaults unchanged (its vtable slots 5/6 hold the raw
// 0x0013b6 / 0x001c08 ILT thunks); every sibling overrides slot 5 and keeps slot 6
// = 0x001c08. That shared slot-6 address is the binary fingerprint of this base.
//
// ABSTRACT - never instantiated, so cl emits NO ??_7CWapObj. As a middle class in
// single inheritance its subobject-dtor vptr re-stamp (to a would-be ??_7CWapObj)
// is a DEAD STORE - immediately overwritten by ~Wap::CObject's grand-base stamp
// (0x5e8cb4) with no intervening vptr read - so MSVC5 /O2 dead-store-eliminates it.
// Net: a CWapObj-derived dtor keeps retail's exact TWO vptr stamps (own ??_7 then
// the grand-base 0x5e8cb4), and no CWapObj vtable is ever materialised. Deriving
// classes declare ONLY their own further virtuals + overrides; slots 0..4 come from
// Wap::CObject and slots 5/6 from here, never re-declared.
//
// CWapObj adds no data members of its own: sizeof == sizeof(Wap::CObject) == 4
// (the shared vptr). A derived class's own fields therefore begin at +0x04 (the
// three-word +0x04/+0x08/+0x0c header some siblings reset in their dtors is owned
// by the derived class / a further intermediate, not here). Field names elsewhere
// are placeholders; only offsets + emitted bytes are load-bearing.
#ifndef WAP32_CWAPOBJ_H
#define WAP32_CWAPOBJ_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/CObject.h> // Wap::CObject - the 5-slot engine grand-base (vtbl 0x5e8cb4)

// Abstract intermediate: sizeof == 4 (inherited vptr only, no own fields).
SIZE(CWapObj, 0x04);
class CWapObj : public Wap::CObject {
public:
    // slot 5 (@+0x14) default @0x0013b6: `return m_10 > 0`. Derived classes
    // (CLoadable::IsLoaded, CGameLevel::IsLoaded, ...) override; CImage keeps it.
    virtual i32 IsLoaded();
    // slot 6 (@+0x18) default @0x001c08: `return 1`. CImageSet1/2/3 override; the
    // rest of the family (CImage, CResolveNode, the workers, ...) keep it.
    virtual i32 IsReady();
};

#endif // WAP32_CWAPOBJ_H
