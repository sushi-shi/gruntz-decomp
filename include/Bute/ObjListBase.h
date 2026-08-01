#ifndef GRUNTZ_BUTE_OBJLISTBASE_H
#define GRUNTZ_BUTE_OBJLISTBASE_H

#include <Ints.h>
#include <rva.h>

VTBL(CObjListBase, 0x001ef760);
struct CObjListBase {
    virtual void V0() = 0; // slot 0 (__purecall)
    // NON-virtual, user-declared (the vtable has ONE slot - V0 - so the dtor is not in
    // it). It is what makes the implicit destructor chain NON-trivial, so every derived
    // dtor ends by restoring THIS table: retail's ~CRezList is the whole 7-byte body
    // `mov [ecx],??_7CObjListBase@@6B@ / ret`, and ~CRezDir inlines its two CRezList
    // members as two stores of the same table (0x13c9b0 tail). Without it cl leaves the
    // derived class's own stamp standing instead.
    ~CObjListBase() {}
};
SIZE(0x4);

#endif // GRUNTZ_BUTE_OBJLISTBASE_H
