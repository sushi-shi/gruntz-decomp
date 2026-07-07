// AnalysisVtables.cpp - RESIDUAL analysed vtables not yet re-homed onto their real
// class. Each entry below is being migrated to the owning class's real header/TU
// (VTBL bound next to the real polymorphic class); this file shrinks toward empty.
// Owners identified by the vtable's slot-function RVAs + the ctor/dtor that stamps
// the vptr (see the fold campaign notes). Placeholder NAMES only where the real
// class is still being reconstructed.
#include <rva.h>
#include <Wap32/Object.h>
#include <Gruntz/Dialogs.h> // CDialog (MFC hierarchy)

struct CMultiHelpDlg : public CDialog {
    virtual ~CMultiHelpDlg() OVERRIDE;            // slot 1
    virtual const void* GetMessageMap() OVERRIDE; // slot 12
    virtual void WndVsl35() OVERRIDE;             // slot 35
};
SIZE_UNKNOWN(CMultiHelpDlg);
VTBL(CMultiHelpDlg, 0x001ea474);

struct IStream {
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
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
};
SIZE_UNKNOWN(IStream);

struct CArchiveStream : public IStream { // 14 slots (first=QueryInterface)
    virtual void Slot00() OVERRIDE;
    virtual void Slot01() OVERRIDE;
    virtual void Slot02() OVERRIDE;
    virtual void Slot03() OVERRIDE;
    virtual void Slot04() OVERRIDE;
    virtual void Slot05() OVERRIDE;
    virtual void Slot06() OVERRIDE;
    virtual void Slot07() OVERRIDE;
    virtual void Slot08() OVERRIDE;
    virtual void Slot09() OVERRIDE;
    virtual void Slot10() OVERRIDE;
    virtual void Slot11() OVERRIDE;
    virtual void Slot12() OVERRIDE;
    virtual void Slot13() OVERRIDE;
};
SIZE_UNKNOWN(CArchiveStream);
VTBL(CArchiveStream, 0x001ed98c);

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

struct CEngObj_1efd28 : Wap::CObject {  // 23 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1efd28() OVERRIDE; // slot 1 (CObject dtor)
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual void Slot18();
    virtual void Slot19();
    virtual void Slot20();
    virtual void Slot21();
    virtual void Slot22();
};
SIZE_UNKNOWN(CEngObj_1efd28);
VTBL(CEngObj_1efd28, 0x001efd28);

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

struct CEngObj_1eff70 : Wap::CObject {  // 11 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1eff70() OVERRIDE; // slot 1 (CObject dtor)
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
};
SIZE_UNKNOWN(CEngObj_1eff70);
VTBL(CEngObj_1eff70, 0x001eff70);

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

