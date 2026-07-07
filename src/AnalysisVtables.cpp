// AnalysisVtables.cpp - RESIDUAL analysed vtables not yet re-homed onto their real
// class. Each entry below is being migrated to the owning class's real header/TU
// (VTBL bound next to the real polymorphic class); this file shrinks toward empty.
// Owners identified by the vtable's slot-function RVAs + the ctor/dtor that stamps
// the vptr (see the fold campaign notes). Placeholder NAMES only where the real
// class is still being reconstructed.
#include <rva.h>
#include <Wap32/Object.h>

struct CEngObj_1ef6c8 { // 2 slots (first=__purecall)
    virtual void Slot00();
    virtual void Slot01();
};
SIZE_UNKNOWN(CEngObj_1ef6c8);
VTBL(CEngObj_1ef6c8, 0x001ef6c8);

struct CEngObj_1ef744 { // 1 slots (first=sub_13c340)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1ef744);
VTBL(CEngObj_1ef744, 0x001ef744);

struct CEngObj_1ef75c { // 1 slots (first=sub_13c4c0)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1ef75c);
VTBL(CEngObj_1ef75c, 0x001ef75c);

struct CEngObj_1ef760 { // 1 slots (first=__purecall)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1ef760);
VTBL(CEngObj_1ef760, 0x001ef760);

struct CEngObj_1efe3c { // 10 slots (first=Reset)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
};
SIZE_UNKNOWN(CEngObj_1efe3c);
VTBL(CEngObj_1efe3c, 0x001efe3c);

struct CEngObj_1efe74 { // 10 slots (first=Reset)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
};
SIZE_UNKNOWN(CEngObj_1efe74);
VTBL(CEngObj_1efe74, 0x001efe74);

struct CEngObj_1f04dc { // 1 slots (first=sub_16ea80)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1f04dc);
VTBL(CEngObj_1f04dc, 0x001f04dc);

struct CEngObj_1f04e4 { // 1 slots (first=ConstructTail_ea20)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1f04e4);
VTBL(CEngObj_1f04e4, 0x001f04e4);

struct CEngObj_1f0510 { // 1 slots (first=`scalar_deleting_destructor')
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1f0510);
VTBL(CEngObj_1f0510, 0x001f0510);

struct CEngObj_1f0518 { // 1 slots (first=sub_174e70)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1f0518);
VTBL(CEngObj_1f0518, 0x001f0518);

