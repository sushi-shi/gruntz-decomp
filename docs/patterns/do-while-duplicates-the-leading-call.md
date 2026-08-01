# A guard + `do-while` DUPLICATES the loop's leading call at the bottom — write `while`

**Tags:** `topic:codegen-idiom` `cpp:loop` `cpp:branch` | `asm:call` `asm:cmp` `asm:jne` | `topic:regalloc`

## Symptom

The loop BODY is byte-identical to retail, but the recompile's loop *bottom* carries an
extra copy of the loop's FIRST call, and retail's is a single test:

```
retail (bottom)                        base (bottom)
cmp  BYTE PTR [ebx],0x0                cmp  byte ptr [ebx],0x0
jne  <loop top>                        je   <exit>
                                       push <"X">
                                       push ebx
                                       call <strstr>
                                       add  esp,0x8
                                       test eax,eax
                                       jne  <loop top+N>
```

Downstream, the cursor variable also loses its callee-saved register: retail pins it in
`ebx` from entry (`push ebx; mov ebx,[esp+0x10c]`), the recompile splits it into `eax`
for the peeled first iteration. That register split is a SYMPTOM, not the wall — it is
easy to misfile as a "loop-carried-cursor regalloc wall".

## Mechanism

```cpp
if (*str == 0) {          // hand-written entry guard
    return;
}
do {
    char* x = strstr(str, "X");   // the loop's leading call
    ...
} while (*str != 0);
```

cl5 rotates a `do-while` whose exit test is cheap by hoisting the *next* iteration's
leading work above the back edge, so the `strstr` gets **duplicated** into the bottom
block and the back edge lands after it. The peeled copy needs the cursor live in a
different place than the steady-state copy, which is what breaks the `ebx` pin.

Writing the plain `while` instead lets cl5 do its own rotation: it emits ONE entry test
(the same `test al,al; je exit` the hand guard produced) and a single `cmp [cursor],0;
jne top` at the bottom, with the leading call emitted exactly once.

## Fix

```cpp
while (*str != 0) {
    char* x = strstr(str, "X");
    ...
}
```

The hand `if (…) return;` guard is redundant with `while`'s own entry test — deleting it
is what removes the peel.

## Evidence

- `src/Rez/DebugPrintf.cpp`, `?AddFromString@CRangeSet@@QAEXPAD@Z` @ 0x184c10:
  **92.15% -> 100.00% EXACT** from that one restructure; no other edit. The prior note
  had filed it as a "loop-carried-cursor regalloc wall … not source-steerable".
- The reverse direction is also real and is the SAME test: where retail *has* no
  zero-trip skip (`mov edi,0x100; sub edi,eax` straight into the body) but the recompile
  emits `sub ecx,eax; je skip`, the retail source is the `do-while` and yours is the
  `for`/`while` (CFecFile::AddFile's name-padding loop). Read the loop bottom AND the
  loop entry off retail before picking the form.

## See also

- `docs/patterns/do-while-is-an-echo-write-while.md` — the sibling reading of the same
  rotation from the other end.
- `docs/patterns/INDEX.md` — `cpp:loop`.
