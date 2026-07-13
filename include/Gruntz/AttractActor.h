// AttractActor.h - the per-frame "attract actor" list the state Render polls
// (global DAT_00645574 == g_actorList). Shared by the states that poll it:
// CAttract (Attract.cpp), CDemo (Demo.cpp) and CHelpState (HelpState.cpp) each
// walk g_actorList->m_data[i], calling the actor's slot-4/slot-5 virtual and
// reading its +0x2ac flags word (0x100 == "request exit"). Previously each TU
// carried its own identical view; unified here (identity of the actor class is
// not yet recovered - a genuine placeholder, only OFFSETS load-bearing).
#ifndef GRUNTZ_GRUNTZ_ATTRACTACTOR_H
#define GRUNTZ_GRUNTZ_ATTRACTACTOR_H

#include <Ints.h>
#include <rva.h> // SIZE_UNKNOWN

// Each actor: slot-4 (+0x10) is the per-frame Update (CAttract::Render drives it);
// slot-5 is a secondary poke (CAttract::Vslot09); +0x2ac is a flags word whose
// 0x100 bit requests the attract exit.
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

// THE global (0x645574): a pointer to the live list. ONE declaration, ONE linkage.
//
// It used to exist under two names with two different types and NO definition at all:
// `AttractActorList* g_actorList` (C++ linkage -> ?g_actorList@@3PAUAttractActorList@@A,
// referenced by demo/helpstate/attractstate/attract) and `CGMEntityList* g_645574`
// (extern "C" -> _g_645574, referenced by menustate/splashstate/creditsstate). Both were
// unresolved externals; the two element types were the same class modelled twice (same
// vtable slots, same +0x2ac flag word) - GameMode.h now typedefs onto these. extern "C"
// keeps the one symbol unmangled; DEFINED (with storage) in MenuState.cpp.
extern "C" AttractActorList* g_actorList;

#endif // GRUNTZ_GRUNTZ_ATTRACTACTOR_H
