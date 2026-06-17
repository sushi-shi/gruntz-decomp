// UnknownHarryPotter.h - Shared class declarations for HarryPotter-related
// Unknown* classes. Placeholder names; offsets are load-bearing.
//
// Approach: standalone classes (no inheritance). The constructor calls a
// base-init "function" and then initialises members. The two vtable stores
// in the target are reproduced by the compiler when a base class constructor
// is present and the non-trivial member objects force an SEH frame.
// We use a single non-virtual base (no vtordisp/vbptr extra code) to obtain
// the two vtable stores from the derived-vs-base init ordering.
#ifndef UNKNOWNHARRYPOTTER_H
#define UNKNOWNHARRYPOTTER_H

// --- Member object types (library stubs) -----------------------------------

class HogwartsMember10 {
public:
    HogwartsMember10();
    virtual ~HogwartsMember10();
    char m_pad[0x18];           // +4(vptr)=0x1c total
};

class LuciusMember10 {
public:
    LuciusMember10();
    virtual ~LuciusMember10();
    char m_pad[0x18];
};

class LuciusMember2C {
public:
    LuciusMember2C();
    virtual ~LuciusMember2C();
    char m_pad[0x18];
};

class CByteArray {
public:
    CByteArray();
    virtual ~CByteArray();
    char m_pad[0x10];           // +4(vptr)=0x14 total
};

// --- Base classes (each target has its own) ---------------------------------
// The constructor calls the base constructor from the initialiser list. The
// base constructor is the function at the given RVA.  The final vtable store
// in the target is the base vtable (same for all bases; ICF-folded to 0x5e8cb4).
// This is regular (non-virtual) inheritance.

class HogwartsBase {
public:
    HogwartsBase();             // @0x154ac0 — calls vtable slot 0x58(=22)
    virtual ~HogwartsBase();
    virtual void VF0() {}
    virtual void VF1() {}
    virtual void VF2() {}
    virtual void VF3() {}
    virtual void VF4() {}
    virtual void VF5() {}
    virtual void VF6() {}
    virtual void VF7() {}
    virtual void VF8() {}
    virtual void VF9() {}
    virtual void VF10() {}
    virtual void VF11() {}
    virtual void VF12() {}
    virtual void VF13() {}
    virtual void VF14() {}
    virtual void VF15() {}
    virtual void VF16() {}
    virtual void VF17() {}
    virtual void VF18() {}
    virtual void VF19() {}
    virtual void VF20() {}
    virtual void VF21() {}
    virtual void VFuncAt0x58() = 0;  // pure virtual slot 22 (0x58/4)
};

class LuciusBase {
public:
    LuciusBase();               // @0x1591e0
    virtual ~LuciusBase();
    virtual void VF0() {}
};

class RemusBase {
public:
    RemusBase();                // @0x15d1f0
    virtual ~RemusBase();
    virtual void VF0() {}
};

class SiriusBase {
public:
    SiriusBase();               // @0x165210
    virtual ~SiriusBase();
    virtual void VF0() {}
};

// ---------------------------------------------------------------------------
// Target classes (regular inheritance from their respective base)
// ---------------------------------------------------------------------------

class UnknownCGruntzMgrHogwarts : public HogwartsBase {
public:
    UnknownCGruntzMgrHogwarts();
    virtual ~UnknownCGruntzMgrHogwarts();
    virtual void MyVF0() {}
    virtual void MyVF1() {}
    virtual void VFuncAt0x58() {}

    int  m_4;
    int  m_8;
    int  m_c;
    HogwartsMember10 m_member10;
};

class UnknownCGruntzMgrLucius : public LuciusBase {
public:
    UnknownCGruntzMgrLucius();
    virtual ~UnknownCGruntzMgrLucius();
    virtual void MyVF0() {}
    virtual void MyVF1() {}

    int  m_4;
    int  m_8;
    int  m_c;
    LuciusMember10 m_memA;
    LuciusMember2C m_memB;
    LuciusMember2C m_memC;
};

class UnknownRemus : public RemusBase {
public:
    UnknownRemus();
    virtual ~UnknownRemus();
    virtual void MyVF0() {}
    virtual void MyVF1() {}

    int  m_4;
    int  m_8;
    int  m_c;
    char m_gap10[0x10];
    CByteArray m_arrA;
    CByteArray m_arrB;
    CByteArray m_arrC;
};

class UnknownSirius : public SiriusBase {
public:
    UnknownSirius();
    virtual ~UnknownSirius();
    virtual void MyVF0() {}
    virtual void MyVF1() {}

    int  m_4;
    int  m_8;
    int  m_c;
    HogwartsMember10 m_member10;
};

// ---------------------------------------------------------------------------
// UnknownMinerva + UnknownPettigrew (no base)
// ---------------------------------------------------------------------------
class UnknownMinerva {
public:
    void ClearUnknownMap();
};

class UnknownPettigrew {
public:
    virtual ~UnknownPettigrew();
    virtual void VF0() {}
    virtual void VirtualMethodUnknown18();
    char m_pad04[0x28];
    int  m_2c;
};

#endif // UNKNOWNHARRYPOTTER_H
