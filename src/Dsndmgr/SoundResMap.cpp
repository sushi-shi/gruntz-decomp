// SoundResMap.cpp - CSoundResMap::RemoveByValue (0x157b00): remove one map entry
// by its value pointer and delete the object.
//
// The inlined GetStartPosition (m_nCount!=0 ? -1 : 0 -> neg/sbb), the throwing
// GetNextAssoc / RemoveKey, and the CString key temp (default-constructed,
// torn down on every exit) force the /GX EH frame. The map helpers + the value's
// virtual scalar-deleting destructor are reloc-masked NAFXCW/engine externs.
#include <Dsndmgr/SoundResMap.h>

// ===========================================================================
// CSoundResMap::RemoveByValue  (0x157b00)
// ===========================================================================
// @early-stop
// Loop-rotation + EH-frame stack-slot wall (topic:regalloc; see
// docs/patterns/stack-slot-coalesce-frame-4b.md + loop-preheader-vs-exit-block-
// order.md). The logic + instruction selection are byte-identical to retail (the
// inlined GetStartPosition neg/sbb, the GetNextAssoc/RemoveKey calls, the virtual
// scalar-deleting `delete p`, the CString temp + /GX frame), but two MSVC5 /O2
// choices diverge with NO source lever:
//   (a) retail keeps ONE loop body (post-tested do-while looping back to
//       GetNextAssoc); our cl peels the first iteration into a second
//       GetNextAssoc copy (the `break`-carrying find-one loop rotates).
//   (b) the {pos, key, value} stack slots are assigned differently - retail puts
//       the CString key in the high EH-object slot (+0x20) with pos/value at
//       +0x8/+0xc; cl puts pos at +0x20 and key/value at +0xc/+0x8.
// CheatMgr::Empty (0x22b00, 100%) is the same map-walk idiom WITHOUT the break -
// its simpler all-delete loop neither peels nor mis-slots; the find-one-and-stop
// break is what flips both. Tried: do-while / for(;;)+if-break (identical),
// value declared first/last (70-74%), value inside the loop (58%). ~74.8%,
// logic complete; deferred to the final sweep.
RVA(0x00157b00, 0xb2)
void CSoundResMap::RemoveByValue(CSoundRes* p) {
    if (p == 0) {
        return;
    }
    POSITION pos = (POSITION)(m_map.GetCount() != 0 ? -1 : 0);
    CString key;
    void* value = 0;
    if (pos != (POSITION)0) {
        do {
            m_map.GetNextAssoc(pos, key, value);
            if (value == p) {
                m_map.RemoveKey(key);
                delete p;
                break;
            }
        } while (pos != (POSITION)0);
    }
}
