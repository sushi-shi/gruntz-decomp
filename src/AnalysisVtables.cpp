// AnalysisVtables.cpp - REAL polymorphic classes modelling every analysed vtable that
// has no RTTI name and no other source owner. Each is a real class whose `virtual`
// methods model the vtable's slots one-for-one (Wap::CObject supplies slots 0..4 =
// GetRuntimeClass/~/Serialize/AssertValid/Dump where the first slot is GetRuntimeClass);
// VTBL() binds the compiler-emitted ??_7 at the retail rva. Placeholder NAMES (the
// engine classes carry no retail RTTI name) but REAL, fully-virtual types - satisfies
// both vtable_coverage and vtable_virtuality. Generated; owners to be renamed as found.
#include <rva.h>
#include <Wap32/Object.h>

struct CMultiHelpDlg { // 54 slots (first=sub_1d2fab)
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
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot16();
    virtual void Slot17();
    virtual void Slot18();
    virtual void Slot19();
    virtual void Slot20();
    virtual void Slot21();
    virtual void Slot22();
    virtual void Slot23();
    virtual void Slot24();
    virtual void Slot25();
    virtual void Slot26();
    virtual void Slot27();
    virtual void Slot28();
    virtual void Slot29();
    virtual void Slot30();
    virtual void Slot31();
    virtual void Slot32();
    virtual void Slot33();
    virtual void Slot34();
    virtual void Slot35();
    virtual void Slot36();
    virtual void Slot37();
    virtual void Slot38();
    virtual void Slot39();
    virtual void Slot40();
    virtual void Slot41();
    virtual void Slot42();
    virtual void Slot43();
    virtual void Slot44();
    virtual void Slot45();
    virtual void Slot46();
    virtual void Slot47();
    virtual void Slot48();
    virtual void Slot49();
    virtual void Slot50();
    virtual void Slot51();
    virtual void Slot52();
    virtual void Slot53();
};
SIZE_UNKNOWN(CMultiHelpDlg);
VTBL(CMultiHelpDlg, 0x001ea474);

struct CArchiveStream { // 14 slots (first=QueryInterface)
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
SIZE_UNKNOWN(CArchiveStream);
VTBL(CArchiveStream, 0x001ed98c);

struct CEngObj_1ef670 { // 4 slots (first=vector_deleting_destructor)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
};
SIZE_UNKNOWN(CEngObj_1ef670);
VTBL(CEngObj_1ef670, 0x001ef670);

struct CEngObj_1ef680 { // 6 slots (first=`scalar_deleting_destructor')
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
};
SIZE_UNKNOWN(CEngObj_1ef680);
VTBL(CEngObj_1ef680, 0x001ef680);

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

struct CEngObj_1ef750 { // 3 slots (first=sub_13b9f0)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
};
SIZE_UNKNOWN(CEngObj_1ef750);
VTBL(CEngObj_1ef750, 0x001ef750);

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

struct CEngObj_1ef7f0 { // 9 slots (first=`scalar_deleting_destructor')
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
};
SIZE_UNKNOWN(CEngObj_1ef7f0);
VTBL(CEngObj_1ef7f0, 0x001ef7f0);

struct CEngObj_1efa58 { // 12 slots (first=ScalarDelete)
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
};
SIZE_UNKNOWN(CEngObj_1efa58);
VTBL(CEngObj_1efa58, 0x001efa58);

struct CEngObj_1efb80 : Wap::CObject {  // 10 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1efb80() OVERRIDE; // slot 1 (CObject dtor)
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
};
SIZE_UNKNOWN(CEngObj_1efb80);
VTBL(CEngObj_1efb80, 0x001efb80);

struct CEngObj_1efbe8 : Wap::CObject {  // 17 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1efbe8() OVERRIDE; // slot 1 (CObject dtor)
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
};
SIZE_UNKNOWN(CEngObj_1efbe8);
VTBL(CEngObj_1efbe8, 0x001efbe8);

struct CEngObj_1efc30 : Wap::CObject {  // 9 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1efc30() OVERRIDE; // slot 1 (CObject dtor)
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
};
SIZE_UNKNOWN(CEngObj_1efc30);
VTBL(CEngObj_1efc30, 0x001efc30);

struct CEngObj_1efca0 : Wap::CObject {  // 9 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1efca0() OVERRIDE; // slot 1 (CObject dtor)
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
};
SIZE_UNKNOWN(CEngObj_1efca0);
VTBL(CEngObj_1efca0, 0x001efca0);

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

struct CEngObj_1efd88 : Wap::CObject {  // 14 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1efd88() OVERRIDE; // slot 1 (CObject dtor)
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
SIZE_UNKNOWN(CEngObj_1efd88);
VTBL(CEngObj_1efd88, 0x001efd88);

struct CEngObj_1efdc0 : Wap::CObject {  // 17 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1efdc0() OVERRIDE; // slot 1 (CObject dtor)
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
};
SIZE_UNKNOWN(CEngObj_1efdc0);
VTBL(CEngObj_1efdc0, 0x001efdc0);

struct CEngObj_1efe08 : Wap::CObject {  // 10 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1efe08() OVERRIDE; // slot 1 (CObject dtor)
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
};
SIZE_UNKNOWN(CEngObj_1efe08);
VTBL(CEngObj_1efe08, 0x001efe08);

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

struct CEngObj_1effd0 : Wap::CObject {  // 19 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1effd0() OVERRIDE; // slot 1 (CObject dtor)
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
};
SIZE_UNKNOWN(CEngObj_1effd0);
VTBL(CEngObj_1effd0, 0x001effd0);

struct CEngObj_1f0020 : Wap::CObject {  // 16 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1f0020() OVERRIDE; // slot 1 (CObject dtor)
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
};
SIZE_UNKNOWN(CEngObj_1f0020);
VTBL(CEngObj_1f0020, 0x001f0020);

struct CEngObj_1f0060 : Wap::CObject {  // 17 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1f0060() OVERRIDE; // slot 1 (CObject dtor)
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
};
SIZE_UNKNOWN(CEngObj_1f0060);
VTBL(CEngObj_1f0060, 0x001f0060);

struct CEngObj_1f00a8 : Wap::CObject {  // 16 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1f00a8() OVERRIDE; // slot 1 (CObject dtor)
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
};
SIZE_UNKNOWN(CEngObj_1f00a8);
VTBL(CEngObj_1f00a8, 0x001f00a8);

struct CEngObj_1f00e8 : Wap::CObject {  // 16 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1f00e8() OVERRIDE; // slot 1 (CObject dtor)
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
};
SIZE_UNKNOWN(CEngObj_1f00e8);
VTBL(CEngObj_1f00e8, 0x001f00e8);

struct CEngObj_1f0128 : Wap::CObject {  // 9 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1f0128() OVERRIDE; // slot 1 (CObject dtor)
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
};
SIZE_UNKNOWN(CEngObj_1f0128);
VTBL(CEngObj_1f0128, 0x001f0128);

struct CEngObj_1f02c0 : Wap::CObject {  // 5 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1f02c0() OVERRIDE; // slot 1 (CObject dtor)
};
SIZE_UNKNOWN(CEngObj_1f02c0);
VTBL(CEngObj_1f02c0, 0x001f02c0);

struct CEngObj_1f0328 : Wap::CObject {  // 6 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1f0328() OVERRIDE; // slot 1 (CObject dtor)
    virtual void Slot05();
};
SIZE_UNKNOWN(CEngObj_1f0328);
VTBL(CEngObj_1f0328, 0x001f0328);

struct CEngObj_1f04cc { // 1 slots (first=`scalar_deleting_destructor')
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1f04cc);
VTBL(CEngObj_1f04cc, 0x001f04cc);

struct CEngObj_1f04dc { // 1 slots (first=sub_16ea80)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1f04dc);
VTBL(CEngObj_1f04dc, 0x001f04dc);

struct CEngObj_1f04e0 { // 1 slots (first=ScalarDtor)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1f04e0);
VTBL(CEngObj_1f04e0, 0x001f04e0);

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

struct CEngObj_1f0760 : Wap::CObject {  // 5 slots (0..4 from Wap::CObject; first=sub_1bef01)
    virtual ~CEngObj_1f0760() OVERRIDE; // slot 1 (CObject dtor)
};
SIZE_UNKNOWN(CEngObj_1f0760);
VTBL(CEngObj_1f0760, 0x001f0760);

struct CEngObj_1f07d8 { // 5-slot vtable (== CObject shape; modeled standalone)
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
};
SIZE_UNKNOWN(CEngObj_1f07d8);
VTBL(CEngObj_1f07d8, 0x001f07d8);

struct CEngObj_216104 { // 1 slots (first=FUN_0051f510)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_216104);
VTBL(CEngObj_216104, 0x00216104);

struct CEngObj_216108 { // 1 slots (first=FUN_0051f510)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_216108);
VTBL(CEngObj_216108, 0x00216108);

struct CEngObj_216464 { // 1 slots (first=FUN_00524df0)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_216464);
VTBL(CEngObj_216464, 0x00216464);

struct CEngObj_216558 { // 1 slots (first=sub_1273f0)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_216558);
VTBL(CEngObj_216558, 0x00216558);
