/* snapshot.h - the record/replay snapshot format.
 *
 * Deliberately dumb: a fixed-size header, a fixed-size region table, then raw
 * region blobs back to back. Every field is a little-endian u32 at a fixed
 * offset, so `struct.unpack` reads it and `xxd` greps it. No compression, no
 * varints, no nesting. The whole point of the format is that you can believe
 * what it says without running anything.
 *
 *     [SnapHeader]  [SnapRegion x n_regions]  [blob][blob]...
 *
 * A region's `blob_off` is a byte offset from the START OF THE FILE, or 0 when
 * no bytes were recorded for it (reserved / PAGE_NOACCESS / deliberately
 * skipped). Blobs appear in the same order as the table.
 *
 * ---------------------------------------------------------------------------
 * What is recorded
 * ---------------------------------------------------------------------------
 * SNAP_WRITABLE (tier 1)  every committed WRITABLE region, plus every region of
 *                         the game image (its read-only .text/.rdata too, since
 *                         the replay has to execute retail's code and read its
 *                         constants).
 * SNAP_FULL     (tier 2)  every committed region in the address space, whatever
 *                         its protection - a CRIU-style checkpoint. Needed only
 *                         when the replayed closure calls through the IAT, so
 *                         that wine's DLLs are present at their original
 *                         addresses and the import thunks are valid by
 *                         construction. `gruntz.audit.iat_tiers` says which
 *                         functions need this (the IMPORT tier) and which do
 *                         not (CLEAN, 31.6% of the binary).
 *
 * The capture apparatus excludes ITSELF - the capture DLL's image and the
 * buffers it allocates. That is not a fudge: those pages did not exist in the
 * un-instrumented process, so recording them would be recording the observer.
 *
 * ---------------------------------------------------------------------------
 * THE STACK EXCLUSION RULE  (stated once, here, and not widened anywhere else)
 * ---------------------------------------------------------------------------
 * `entry_esp` in the header is the value of ESP at the instant the target
 * function was entered - so [entry_esp] is its return address and [entry_esp+4]
 * onwards are its incoming stack arguments.
 *
 *   * Bytes BELOW entry_esp are NEVER compared. That is the callee's own frame.
 *     Our compiled function's frame is not retail's - different spills, a
 *     different frame size, a different amount of red zone touched - so those
 *     bytes are dead scratch on both sides by construction, not program state.
 *   * The four bytes AT entry_esp are NEVER compared either. The replay
 *     overwrites that slot with the address of its own trampoline so it can
 *     regain control when the callee returns; retail's `ret` only ever reads
 *     it. Four bytes, named exactly, at a fixed address.
 *   * Bytes AT entry_esp+4 AND ABOVE are compared in full. Those are the
 *     incoming stack arguments and the caller's frame, and an out-parameter
 *     pointing at a caller local lands there. Dropping the whole stack would
 *     silently hide the most common out-param idiom in this codebase, so the
 *     boundary sits above the arguments, not below them.
 *
 * The rule is a boundary, not a heuristic. It does not move because a diff
 * showed up: a difference below entry_esp is not evidence of anything and is
 * not looked at, and a difference above it is a real disagreement.
 *
 * The capture hook runs on its OWN stack (it switches ESP to a private scratch
 * region before doing any work), so the bytes below entry_esp are exactly what
 * the un-instrumented callee would have inherited from the previous call. The
 * observer does not perturb the scratch it excludes.
 */

#ifndef GRUNTZ_SNAPSHOT_H
#define GRUNTZ_SNAPSHOT_H

#define SNAP_MAGIC "GRZSNAP\x01"
#define SNAP_VERSION 1

/* header .flags */
#define SNAP_WRITABLE 0x0001 /* tier 1: writable regions + the game image */
#define SNAP_FULL 0x0002     /* tier 2: every committed region */

/* header .phase */
#define SNAP_PHASE_ENTRY 0
#define SNAP_PHASE_EXIT 1

/* SnapRegion .klass */
#define SNAPR_GAME_IMAGE 1  /* a section of GRUNTZ.EXE itself */
#define SNAPR_OTHER_IMAGE 2 /* a section of some other module (wine, MSS32...) */
#define SNAPR_PRIVATE 3     /* MEM_PRIVATE: heap, VirtualAlloc, thread stacks */
#define SNAPR_MAPPED 4      /* MEM_MAPPED: a file/section view */
#define SNAPR_HOOK_STACK 5  /* the stack of the thread that was hooked */

/* SnapRegion .rflags */
#define SNAPRF_HAS_BYTES 0x0001 /* blob_off is valid */
#define SNAPRF_WRITABLE 0x0002  /* protection allows writes */

typedef struct SnapRegs {
    unsigned int eax, ecx, edx, ebx;
    unsigned int esp, ebp, esi, edi;
    unsigned int eflags, eip;
} SnapRegs;

typedef struct SnapRegion {
    unsigned int base;
    unsigned int size;
    unsigned int protect;       /* MEMORY_BASIC_INFORMATION.Protect */
    unsigned int alloc_base;    /* .AllocationBase */
    unsigned int alloc_protect; /* .AllocationProtect */
    unsigned int state;         /* .State */
    unsigned int type;          /* .Type */
    unsigned int klass;         /* SNAPR_* */
    unsigned int rflags;        /* SNAPRF_* */
    unsigned int blob_off;      /* file offset of the bytes, 0 if none */
    char module[40];            /* owning module basename, "" if none */
} SnapRegion;                   /* 80 bytes */

typedef struct SnapHeader {
    char magic[8];
    unsigned int version;
    unsigned int flags;
    unsigned int phase;
    unsigned int hdr_size;   /* sizeof(SnapHeader) - so a reader can check */
    unsigned int region_size;/* sizeof(SnapRegion) */
    unsigned int n_regions;
    unsigned int region_off; /* file offset of the region table */
    unsigned int total_size; /* file size */

    unsigned int image_base; /* where GRUNTZ.EXE actually landed */
    unsigned int target_rva; /* the hooked function */
    unsigned int hit_index;  /* which call of it this is (0-based) */

    /* The stack exclusion boundary. See the block comment above. */
    unsigned int entry_esp;
    unsigned int ret_addr; /* the value at [entry_esp] at entry */
    unsigned int stack_base; /* the hooked thread's stack region, low ... */
    unsigned int stack_end;  /* ... and one past its top */

    unsigned int tick; /* GetTickCount() at capture */
    unsigned int pad0;

    SnapRegs regs; /* entry regs for phase 0, exit regs for phase 1 */

    char target_name[64];
} SnapHeader;

#endif /* GRUNTZ_SNAPSHOT_H */
