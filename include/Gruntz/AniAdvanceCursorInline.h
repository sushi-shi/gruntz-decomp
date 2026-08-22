#ifndef GRUNTZ_ANIADVANCECURSORINLINE_H
#define GRUNTZ_ANIADVANCECURSORINLINE_H

#include <Gruntz/AniAdvanceCursor.h>

// A CAniAdvanceCursor predicate written as a free function in its own header,
// included by 16 TUs (nine of them MID-FILE).  Nothing structural defends it:
// retail has no out-of-line entity for it, so a dev would have written
// cursor.IsComplete() as a member and this file would not exist.
// Collapse measured 2026-08-22 (member i32 IsComplete() const in
// AniAdvanceCursor.h, all 34 call sites rewritten, header deleted; in-class and
// out-of-class-inline spellings scored identically): -31.55 total, -3 exact,
// every row sub-0.5 /O2 ripple from the extra declaration in a 37-TU header
// except CGrunt::StepArrivalDrop, whose 32.30 <-> 0.00 is bistable under any
// perturbation at unchanged size.
// The blocking cost is the ledger, not the bytes: rewriting the call sites
// changes those functions' source hashes, which RESETS their banked MAX to the
// current value - CTeleporter::Update is banked at 100.00 but currently sits at
// 98.91 on unrelated ripple, so the rewrite would bank the dip.
// REMOVAL CONDITION: collapse it in a build where wormhole's CTeleporter::Update
// reads its banked 100.00, so no call-site rewrite resets a bank.
inline i32 IsAniCursorComplete(const CAniAdvanceCursor* cursor) {
    return cursor->m_finished != 0 && cursor->m_frameTicksLeft == 0;
}

#endif // GRUNTZ_ANIADVANCECURSORINLINE_H
