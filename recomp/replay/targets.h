/* targets.h - the replayable functions, and where OUR version of each lives.
 *
 * Adding a target is three lines here plus the unit name on the build.sh
 * command line. The address of our side is taken through a pointer-to-member
 * so the C++ MANGLING binds it: `&DSoundList::Unlink` resolves to
 * ?Unlink@DSoundList@@QAEXPAUDSoundLink@@@Z in the linked-in
 * build/objdiff/base/soundvoicelist.obj - the exact bytes the decompilation
 * ships, not a transcription. That is the same rule recomp/harness/recomp.h
 * follows.
 *
 * A target belongs here only if `python -m gruntz.audit.iat_tiers` calls it
 * CLEAN *and* it carries no relocations. The reloc condition is separate and
 * currently load-bearing: our object is LINKED INTO replay.exe, so a global it
 * references resolves to OUR image's copy, not to the restored game's. A
 * function that touches a global would therefore read a different variable
 * than retail does and disagree for a reason that is an artefact of the
 * harness. Lifting that restriction means loading the .obj at run time and
 * binding its relocations to game_base + rva - see README.md, "what is next".
 */

#ifndef GRUNTZ_REPLAY_TARGETS_H
#define GRUNTZ_REPLAY_TARGETS_H

#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundVoiceList.h>

typedef struct ReplayTarget {
    const char *name;
    unsigned int rva;
    void *ours;
} ReplayTarget;

/* MSVC cannot cast a pointer-to-member to void* directly; the union is the
 * standard escape and is exact for a non-virtual member on this ABI. */
template<class F> static void *pmf_addr(F f)
{
    union {
        F m;
        void *p;
    } u;
    u.p = 0;
    u.m = f;
    return u.p;
}

static ReplayTarget g_targets[] = {
    /* Doubly-linked-list unlink. Writes prev->next and next->prev through two
     * different heap pointers, or the list head/tail through `this` - four
     * control-flow arms, twelve bytes of effect, zero relocations. */
    { "?Unlink@DSoundList@@QAEXPAUDSoundLink@@@Z", 0x001391e0,
      pmf_addr(&DSoundList::Unlink) },
    /* RIFF/WAVE chunk walk. Writes three OUT-parameters that the caller keeps
     * in ITS OWN STACK FRAME - above the callee's entry ESP, so they land on
     * the compared side of the stack rule, which is the point of putting the
     * boundary there rather than dropping the whole stack. */
    { "_ParseWaveChunks", 0x00137110, (void *)ParseWaveChunks },
};

#define REPLAY_NTARGETS (sizeof(g_targets) / sizeof(g_targets[0]))

#endif /* GRUNTZ_REPLAY_TARGETS_H */
