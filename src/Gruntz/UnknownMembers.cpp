// UnknownMembers.cpp - Stubs for base constructors, member-type constructors.
// Provides @address annotations so the PDB carries consistent mangled names.
// The stub bodies are NOT byte-matched (this TU is "wip").
#include "UnknownHarryPotter.h"

// === Base-class constructors (one per target class) ===
// These are called from the derived ctor's initialiser list (regular
// non-virtual inheritance).  The real functions at these RVAs dispatch
// through vtable slots; our stubs just provide the PDB symbol name.

// @address: 0x154ac0
// @size:    0x12
// Calls pure virtual VFuncAt0x58 (slot 22) to trigger vtordisp in derived ctor.
HogwartsBase::HogwartsBase()
{
    this->VFuncAt0x58();
}

// @address: 0x1591e0
// @size:    0x05
LuciusBase::LuciusBase()
{
}

// @address: 0x15d1f0
// @size:    0x87
RemusBase::RemusBase()
{
}

// @address: 0x165210
// @size:    0xa2
SiriusBase::SiriusBase()
{
}

// === Member-type constructors (library functions) ===

// @address: 0x1b7ef2
// @size:    0x33
HogwartsMember10::HogwartsMember10()
{
}

// @address: 0x1b5a2b
// @size:    0x33
LuciusMember10::LuciusMember10()
{
}

// @address: 0x1b8665
// @size:    0x33
LuciusMember2C::LuciusMember2C()
{
}

// @address: 0x1b561c
// @size:    0x37
CByteArray::CByteArray()
{
}

// Out-of-line dtors to anchor vftables in this TU.
HogwartsMember10::~HogwartsMember10() {}
LuciusMember10::~LuciusMember10() {}
LuciusMember2C::~LuciusMember2C() {}
CByteArray::~CByteArray() {}
HogwartsBase::~HogwartsBase() {}
LuciusBase::~LuciusBase() {}
RemusBase::~RemusBase() {}
SiriusBase::~SiriusBase() {}
UnknownCGruntzMgrHogwarts::~UnknownCGruntzMgrHogwarts() {}
UnknownCGruntzMgrLucius::~UnknownCGruntzMgrLucius() {}
UnknownRemus::~UnknownRemus() {}
UnknownSirius::~UnknownSirius() {}
UnknownPettigrew::~UnknownPettigrew() {}
