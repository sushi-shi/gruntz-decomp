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

// 0x1efe3c was a vtable_scan artifact (FF15 call-through-slot addr, not a real
// vtable) - dropped by the scanner FF15/FF25 refinement; the slot belongs to
// CFileMem @0x1efe30 (Io/FileMem.h, 13 slots).

// 0x1efe74 was a vtable_scan artifact (FF15 call-through-slot addr, not a real
// vtable) - dropped by the scanner FF15/FF25 refinement; the slot belongs to
// CFileMemBase @0x1efe68 (Io/FileMem.h, 13 slots).

// 0x1f04dc REHOMED -> g_buteTree runtime +0x08 secondary (??_7CButeStore@@6BCButeStoreSecond@@),
// bound via @data-symbol in include/Bute/ButeMgr.h (CButeStore==CButeTree emits it).

struct CEngObj_1f04e4 { // 1 slots (first=ConstructTail_ea20)
    virtual void Slot00();
};
SIZE_UNKNOWN(CEngObj_1f04e4);
VTBL(CEngObj_1f04e4, 0x001f04e4);

// 0x1f0510 REHOMED -> CBSecStream primary vtable (zPTree-derived, src/Bute/ButeSectionCtor.cpp).

// 0x1f0518 REHOMED -> CButeCfgNode174d's +0x08 MI-secondary vtable
// (??_7CButeCfgNode174d@@6BCButeNodeEntry@@), bound via @data-symbol in src/Bute/ButeNode.cpp.

