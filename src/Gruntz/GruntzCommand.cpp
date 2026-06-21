// GruntzCommand.cpp - the command-pattern class family (CGruntzCommand base +
// CGruntzSingleCommand / CGruntzMultiCommand leaves). See GruntzCommand.h for
// the hierarchy + layout. Names are placeholders; offsets + code bytes are
// load-bearing.
//
// Functions matched in this TU (RVAs in GRUNTZ.EXE):
//   CGruntzCommand::~CGruntzCommand() (??_G) 0x024330 - slot-0 scalar-deleting
//       dtor: restore vftable, (flag&1) -> operator delete(this).  BYTE-EXACT.
//   CGruntzSingleCommand::Allocate()  0x024220 - new-or-recycle allocator:
//       recycle-list tail-call else operator new(0x14) + inline-ctor vftable
//       store.  (Retail labels this RVA ??0CGruntzSingleCommand; the body is the
//       allocator, not a plain ctor - see the allocator note below.)
//   CGruntzMultiCommand::Allocate()   0x024360 - same shape, Multi list/vftable.
//
// NOT matched (see report): the two 7-byte void-thiscall vtable-set helpers at
// 0x0242f0 / 0x024430 (?CGruntzCommand_0242f0 / _024430). Their retail bytes are
// a bare `mov [this],&vftable; ret` (void, no eax-return, no null guard) - an
// out-of-line materialization of the inlined base-vftable store that MSVC 5.0
// will NOT emit as a standalone COMDAT from the canonical class under the locked
// flags (a real ??0 emits `mov eax,ecx`; placement-new emits a null guard).
// They stay in the src/Stub/ backlog as the documented COMDAT-duplication case.
#include <Gruntz/GruntzCommand.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CGruntzCommand::~CGruntzCommand() - the slot-0 scalar-deleting dtor (??_G).
// Restore the vftable, then (if the low bit of the hidden flags arg is set)
// operator delete(this). Empty body (no members, no base) - MSVC synthesizes
// the deleting thunk from `virtual ~CGruntzCommand() {}` in the header. The
// thunk has no source body so it cannot carry an RVA() attribute; pin the
// deleting-dtor symbol by mangled name here.
// @rva-symbol: ??_GCGruntzCommand@@UAEPAXI@Z 0x024330 0x20

// Out-of-line vtable anchors (slots 1..7) so the CGruntzCommand vftable is
// emitted in this TU (the ctor/dtor reference it). Bodies are placeholders.
void CGruntzCommand::Vfunc1() {}
void CGruntzCommand::Vfunc2() {}
void CGruntzCommand::Vfunc3() {}
int CGruntzCommand::Vslot04() {
    return 1;
}
int CGruntzCommand::Vslot05() {
    return 1;
}
void CGruntzCommand::Vslot06() {}
void CGruntzCommand::Vslot07() {}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Allocate() - 0x024220.
//
// new-or-recycle allocator: if the per-class recycle list is non-empty
// (g_singleCmdCount != 0), tail-call g_singleCmdList.RemoveTail() to reuse a
// node; otherwise operator new(0x14) a fresh object and (if non-null) run the
// inline ctor, which only stamps the leaf vftable. Returns the object (or 0).
// ---------------------------------------------------------------------------
RVA(0x024220, 0x2b)
CGruntzSingleCommand* CGruntzSingleCommand::Allocate() {
    if (g_singleCmdCount) {
        return (CGruntzSingleCommand*)g_singleCmdList.RemoveTail();
    }
    return new CGruntzSingleCommand;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Allocate() - 0x024360. Same shape, Multi list/vftable.
// ---------------------------------------------------------------------------
RVA(0x024360, 0x2b)
CGruntzMultiCommand* CGruntzMultiCommand::Allocate() {
    if (g_multiCmdCount) {
        return (CGruntzMultiCommand*)g_multiCmdList.RemoveTail();
    }
    return new CGruntzMultiCommand;
}
