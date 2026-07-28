/* objmod.h - a COFF object, laid out and PRE-RELOCATED against the restored
 * game image. Written by objbind.py, loaded by replay.cpp.
 *
 * ---------------------------------------------------------------------------
 * Why this file exists at all
 * ---------------------------------------------------------------------------
 * The first version of the replay LINKED our compiled object into replay.exe.
 * That works only for a function that references nothing: a global it touches
 * resolves to replay.exe's OWN copy of that variable rather than to the one in
 * the restored game image, and a call to another function in the same TU
 * resolves to a /FORCE:UNRESOLVED null. Both are harness artefacts, and both
 * make the verdict meaningless, so targets were restricted to bodies with zero
 * relocations. That restriction - not the size of the state, not the tier -
 * was what capped the reachable set.
 *
 * Binding the object's relocations to `game_base + rva` removes it. Every
 * symbol our object names is looked up in build/gen/symbol_names.csv (plus the
 * FLIRT library labels), and the reference is written to point at the RETAIL
 * address, which the snapshot has restored. So our function reads the game's
 * real globals and calls the game's real callees - the verdict is then about
 * OUR FUNCTION'S BODY and nothing else.
 *
 * ---------------------------------------------------------------------------
 * The binding rule, in one line
 * ---------------------------------------------------------------------------
 *   a reference binds to RETAIL if the symbol has a known retail rva;
 *   otherwise to OUR loaded copy; otherwise it is UNRESOLVED and the function
 *   that contains it is refused.
 *
 * "Retail wins" is deliberate and it is what isolates the test. A call from our
 * function to another of our functions binds to RETAIL's copy of the callee, so
 * a second unmatched body cannot contaminate the verdict on the first. The
 * symbols that legitimately fall through to our own copy are the ones with no
 * retail identity: intra-function jump-table labels and `$SG` string literals.
 * Both are counted per function (`n_local_foreign` below) so a target that
 * depends on one is visible rather than silent.
 */

#ifndef GRUNTZ_REPLAY_OBJMOD_H
#define GRUNTZ_REPLAY_OBJMOD_H

#define OBJMOD_MAGIC "GRUNTZOM"
#define OBJMOD_VERSION 1

/* Symbol flags. */
#define OBJSYM_FUNC 0x0001    /* defined in a section carrying code */
#define OBJSYM_OURS 0x0002    /* defined in this module (has a loaded copy) */
#define OBJSYM_RETAIL 0x0004  /* has a known retail rva */
#define OBJSYM_REFUSE 0x0008  /* its section has an unresolved relocation */
#define OBJSYM_VOIDRET 0x0010 /* the mangled name proves it returns void, so eax
                               * is not part of the observable - see
                               * objbind.void_return() */

typedef struct ObjModSym {
    unsigned int name_off;  /* into the string table */
    unsigned int ours;      /* VA of our loaded copy, or 0 */
    unsigned int retail;    /* image_base + rva, or 0 */
    unsigned int flags;
    unsigned int n_reloc;         /* relocations in this symbol's section */
    unsigned int n_bound_retail;  /* ... bound to the restored game image */
    unsigned int n_local_intra;   /* ... to a label inside the same section */
    unsigned int n_local_foreign; /* ... to our own copy of something else */
    unsigned int n_unresolved;    /* ... bound to nothing: the function is refused */
} ObjModSym;

typedef struct ObjModHeader {
    char magic[8];
    unsigned int version;
    unsigned int hdr_size;
    unsigned int sym_size;   /* sizeof(ObjModSym) */
    unsigned int load_base;  /* the VA the blob must be placed at */
    unsigned int image_base; /* the game base the relocations were bound against */
    unsigned int blob_off;
    unsigned int blob_size;
    unsigned int sym_off;
    unsigned int n_syms;
    unsigned int str_off;
    unsigned int str_size;
    unsigned int total_size;
} ObjModHeader;

#endif /* GRUNTZ_REPLAY_OBJMOD_H */
