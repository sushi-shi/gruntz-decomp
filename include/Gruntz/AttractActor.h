#ifndef GRUNTZ_GRUNTZ_ATTRACTACTOR_H
#define GRUNTZ_GRUNTZ_ATTRACTACTOR_H

#include <Ints.h>
#include <rva.h> // SIZE_UNKNOWN

// VTBL_ABSENT: never-constructed dispatch view of the g_actorList entries (the
// attract-mode actors are concrete classes with their own vtables). @identity-TODO:
// recover the real actor base via the attract spawners' ctor stamps.
VTBL_ABSENT(AttractActor);
// VTBL_ABSENT: never-constructed dispatch view of the g_actorList entries (the
// attract-mode actors are concrete classes with their own vtables). @identity-TODO:
// recover the real actor base via the attract spawners' ctor stamps.
VTBL_ABSENT(AttractActor);
class AttractActor {
public:
    virtual void Vslot00();
    virtual void Vslot01();
    virtual void Vslot02();
    virtual void Vslot03();
    virtual void Update();  // slot 4 (+0x10)
    virtual void Vslot05(); // slot 5 (+0x14)  the Vslot09 poke
    char m_pad04[0x2ac - 0x4];
    i32 m_2ac; // +0x2ac flags (0x100 == request exit)
};
SIZE_UNKNOWN(AttractActor);

struct AttractActorList {
    char m_pad00[0x4];
    i32 m_count;             // +0x04
    AttractActor* m_data[1]; // +0x08  inline pointer array
};
SIZE_UNKNOWN(AttractActorList);

extern "C" AttractActorList* g_actorList;

#endif // GRUNTZ_GRUNTZ_ATTRACTACTOR_H
