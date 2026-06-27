// TriggerSound.cpp - CTriggerSound::IsCueActive (0x112860): a __thiscall bool
// wrapper that forwards `this` to the cue-state worker (0x1106b0, external) and
// returns whether it is non-zero. Best-guess class; only the call (reloc-masked)
// and the neg/sbb/neg bool-normalize are load-bearing.
#include <Ints.h>
#include <rva.h>

struct CTriggerSound {
    i32 CheckCue();    // 0x1106b0 (external worker, reloc-masked)
    i32 IsCueActive(); // 0x112860
};

RVA(0x00112860, 0xc)
i32 CTriggerSound::IsCueActive() {
    return CheckCue() != 0;
}
