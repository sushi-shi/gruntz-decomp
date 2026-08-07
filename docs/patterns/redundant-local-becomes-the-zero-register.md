# A redundant local turns a callee-saved register into the function's ZERO register

**Symptom.** The whole function diffs as "retail uses immediates, we use a
register": every `test eax,eax` in retail appears as `cmp eax,ebp` in the base,
every `mov dword ptr [x],0` appears as `mov dword ptr [x],ebp`, and the prologue
has one extra `push ebp` / epilogue one extra `pop ebp`. Block topology is
identical. It reads like a diffuse scheduling or "compiler mood" wall.

**It is not.** It is ONE spurious local variable.

## Mechanism

cl5 materializes a constant into a callee-saved register when (a) a register is
free and (b) the constant is used often enough to pay for the extra push/pop.
A local that is *initialized to 0 on a path that dominates most of the function*
hands it both conditions at once: the local already needs a callee-saved home,
and its initializer is a zero the allocator can then spend everywhere else.

The classic source shape that creates one is the "lookup then copy" pair:

```cpp
CGameObject* found  = 0;
CGameObject* result = 0;                       // <- the spurious one
if (MapLookup(map, key, found)) {
    result = found;
}
if (result == NULL) { ... }
```

`found` is address-taken (it is an out-parameter) so it must live in memory;
`result` is a plain pointer, so cl gives it `ebp`, notices `ebp` holds 0 on the
`MapLookup`-failed path, and re-uses `ebp` as the constant 0 for the entire
function. Retail has no `result` at all — it tests the out-parameter directly.

```cpp
CGameObject* found = 0;
if (MapLookup(map, key, found) == 0) {
    found = NULL;
}
if (found == NULL) { ... }
```

The `found = NULL` on the failure path is what MSVC5 needs in order to fold the
two branches into the single `je` retail emits; it is store-to-load-forwarded
away, so it costs nothing.

## Evidence

`CGrunt::StepEntranceRelatchB` (0x00065c20). With the redundant `result`:
90.86% and a `push ebp` retail does not have. Deleting `result`: **98.22%**,
prologue/epilogue exact, and every `cmp X,ebp` / `mov [X],ebp` collapsed to the
`test` / immediate form in one build. No other edit.

## How to spot it

Run `gruntz sema disasm <rva> --diff` and count the diff rows of the form
`-cmp <r>,<callee-saved>` / `+test <r>,<r>` and
`-mov dword ptr [..],<callee-saved>` / `+mov dword ptr [..],0x0`. If they are
spread across the whole body while the block skeleton matches, do NOT reach for
the permuter: go look for a local whose only job is to copy another local, or
whose initializer is a `0`/`NULL` that dominates the body. Delete it.

The inverse also occurs — a **one** register (`mov ebx,0x1` then `push ebx` /
`mov [x],ebx` where retail has `push 0x1`). Same cause, same fix: one live value
too many.

variants: [loop-bound-local-vs-inline-invariant.md](loop-bound-local-vs-inline-invariant.md)
