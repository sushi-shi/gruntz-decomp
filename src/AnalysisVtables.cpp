// AnalysisVtables.cpp - RESIDUAL analysed vtables not yet re-homed onto their real
// class. Each entry below is being migrated to the owning class's real header/TU
// (VTBL bound next to the real polymorphic class); this file shrinks toward empty.
// Owners identified by the vtable's slot-function RVAs + the ctor/dtor that stamps
// the vptr (see the fold campaign notes). Placeholder NAMES only where the real
// class is still being reconstructed.
#include <rva.h>
#include <Wap32/Object.h>

// Rehomed to their real owning classes in the Bute/Dsndmgr subsystem headers:
//   0x1ef6c8 -> PureSoundElem       <Dsndmgr/SoundVoiceList.h>  (2-slot abstract elem base)
//   0x1ef744 -> CHashInsertNode     src/Bute/SymRec.cpp         (CSymRec +0x4 hash-node prefix)
//   0x1ef750 -> CSymParser          <Bute/SymParser.h>          (parser primary vtable, 3 slots)
//   0x1ef75c -> CObjList            <Bute/SymParser.h>          (CSymParser +0x10 list, ctor vtable)
//   0x1ef760 -> CObjListBase        <Bute/SymParser.h>          (that list's abstract-base dtor vtable)

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

// 0x1f0518 REHOMED -> CButeCfgNode174d's +0x08 MI-secondary vtable
// (??_7CButeCfgNode174d@@6BCButeNodeEntry@@), bound via @data-symbol in src/Bute/ButeNode.cpp.

