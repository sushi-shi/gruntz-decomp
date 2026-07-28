# Compensating errors: two bugs that cancel, so the diff reads as a "regalloc coin-flip"

**Tags:** cpp:call cpp:local | asm:mov asm:push | topic:wall topic:codegen-idiom
**Confidence:** 8/10

## Symptom

A function sits in the high 99s. The diff shows one or two instructions whose *operands*
differ — a scratch register, a displacement — and everything else is byte-identical. It
reads exactly like the documented regalloc-preference walls, so it gets an `@early-stop`
with a "coin-flip" note.

It is not a coin-flip. **Two source errors cancelled each other**, and what survives is
the uncancelled remainder.

## The two measured cases (2026-07-28)

**`CButeMgr::GetString` 0x1731d0 — wrong argument ORDER cancelled by a wrong register
assignment.** The source called `ReportError(fmt, key, tag)` where every sibling getter
in the file passes `(fmt, tag, key)`. cl then coloured the two parameters into the
opposite callee-saved registers from retail. In the three failure blocks the two errors
cancel exactly — `push edi; push ebx` is byte-identical either way — so the only visible
residue was the two argument LOADS at the head (`mov edi,[esp+0x10]` vs `mov ebx,…`), i.e.
a textbook "ebx/edi coin-flip". Fixing the order alone made the function *worse* (the
tail pushes then diverge too); the order AND the failure-path shape (one shared tail
`return`, not three early returns) both had to be right.

**`CPlay::DrawDebugStats` 0x0cf770 — wrong member offsets hidden by disp8.** It printed
the debug "Pos" from `m_viewRect.left/top` (+0x40/+0x44); retail reads +0x84/+0x88, the
plane's snapped scroll origin. Both wrong offsets encode as `disp8`, both right ones as
`disp32`, so the compiled body was **812 bytes against an annotated 862** while `--diff`
showed only "two `mov` operands differ".

**`CGruntzMgr::SetVideoMode` 0x8df00 — a fake `extern "C"` hiding a `__thiscall`, plus a
short buffer.** The TU declared `extern "C" i32 __stdcall SvmApply(i32,i32,i32)`; the real
callee is `m_world->SetDimensions(w,h,depth)` (0x155f60, already 100% EXACT elsewhere in
the tree). The fake convention dropped the `mov ecx,[esi+0x30]` receiver load, and the log
buffer was `char buf[0x70]` where retail's frame proves 0x80. Both together made the body
16 bytes short — and the whole thing had been filed as *"retail dedicates ebp to `w` so it
saves 4 registers where we save 3"*, which is not even true (both push four). 99.37 ->
**100 EXACT**.

## The invariant that catches the second class — compiled LENGTH

`gruntz.audit.rva_size` compares the ANNOTATION with Ghidra's carve. Nothing compared our
COMPILED body with it. That check is now `gruntz.audit.base_size`:

    python -m gruntz.audit.base_size --min-pct 99

**`fuzzy >= 99` with `|compiled - annotated| >= 4` is the suspect list.** A body that is
byte-for-byte convincing but the wrong LENGTH cannot be a codegen preference — cl does not
choose to emit fifty fewer bytes. Every instruction-aligned view (objdiff, `--diff`,
`--blocks`) is blind to it by construction: they pair instructions and score operands, so
a systematic encoding-width error shows up as a handful of cheap operand diffs.

Not a gate: a nonzero delta also comes from an over-long annotation (`rva_size` reports
those as LONG), from a COMDAT folded onto a shared tail (one annotation, two names), and
from a one-byte alignment tail. It is a suspect generator.

**First run of the audit, 2026-07-28: 4 of its 8 suspects were real bugs, all in one
sitting.** `SetVideoMode` (above, -16); `CSBI_ImageSet::SerializeFields` (-4) — the
`CObject*` out-param handed to `CMapStringToOb::Lookup` was uninitialised where retail
seeds it (`mov [esp+0x18],eax`, reusing the zero the strlen scan left); and
`CMenuState::Vslot06`/`Vslot09` (-2 each) — the `m_2c` restore belongs INSIDE the failure
arm and repeated after it, so cl hoists the common store above the branch and the failure
path has to materialize its 0 (`xor eax,eax`). Written as one store before the `if`, cl
proves eax is already zero and drops the xor; that had been filed as an
"identical-return-epilogue tail-merge wall, not steerable by source".

## The check that catches the first class — SIBLING CONSISTENCY

There is no cheap disassembly invariant for a swapped argument pair: with the registers
swapped too, even the argument *provenance* at the call site matches. What catches it is
the family. `CButeMgr` has eight getters with the same body shape; exactly one passed
`ReportError` its arguments in a different order, and that one was the one stuck at 99.6.

**Before believing a regalloc story about a function that has siblings, diff your source
against them.** A constant, an argument order, a member offset, or a control-flow shape
that differs in exactly ONE member of an obvious family is a bug, not a style choice. The
clusters this campaign keeps finding (`CGrunt::Create*Sprite`, `CDDSurface::DecodeRun*`,
`CButeMgr::Get*`, the `CDDrawWorker::CreateFrame*` set) all make this cheap to do by eye.

## Doctrine

A register-rename residue is *usually* a real wall — but it is also the exact shape a
compensating error leaves behind. So before writing the `@early-stop`, spend two checks:

1. `python -m gruntz.audit.base_size --min-pct 99` — is the length right?
2. Does the function have siblings, and does the source agree with them?

Both are cheap; both have now caught a real bug that had been filed as a wall.
