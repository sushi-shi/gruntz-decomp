// CAniRecordView.h - the minimal polymorphic view of the 0x34-byte 'ANI' frame
// record that CAniElement (element + collection views) owns in its CObArray.
//
// The REAL class is CAniRecord (src/Gruntz/CAniRecord.cpp, VTBL @0x1f02c0), a
// fully reconstructed dual-base manual-vtable record. It is modeled NON-polymorphic
// there to keep its matched leaf offsets, so the callers can't dispatch through it;
// they need this tiny polymorphic view to emit the `mov edx,[ecx]; call [edx+4]`
// scalar-deleting-dtor virtual call (slot 1) + the two non-virtual parse entries.
//
// Previously duplicated (divergently) in CAniElement.h and CAniElementCollection.h;
// consolidated here so both catalog views share one definition. Folding it into the
// real CAniRecord is the deferred header-dedup follow-up (the identity is known).
#ifndef GRUNTZ_CANIRECORDVIEW_H
#define GRUNTZ_CANIRECORDVIEW_H

#include <Ints.h>

struct CAniRecordView {
    virtual void Slot00();                           // slot 0 (+0x00)
    virtual void* ScalarDtor(u32 flag);              // slot 1 (+0x04) scalar-deleting dtor
    i32 Parse_168c60(void* ctx, const char* cursor); // 0x168c60 __thiscall
    i32 GetSize_168e50();                            // 0x168e50 __thiscall
};

#endif // GRUNTZ_CANIRECORDVIEW_H
