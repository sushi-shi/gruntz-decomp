# A local coalesced onto a dead PARAMETER home with no init copy: write through the parameter

tags: cpp:local cpp:call | asm:mov asm:lea | topic:codegen-idiom topic:regalloc
symptoms: one extra `mov [esp+N],reg` in the prologue (or just after the arg pushes) that
stores a parameter back into its OWN incoming slot; `lea ecx,[esp+N]` where `[esp+N]` is a
parameter home; retail reading the value back from that home (`mov eax,[esp+N]`) where you
use the register copy; a callee-saved colour swap that follows the extra store
confidence: 9/10
variants: macro-local-decl-order-picks-param-home.md, stack-slot-coalesce-frame-4b.md

MSVC5 reuses an incoming parameter's home slot for a local once the parameter is dead.
When the local is spelled `i32 coord = a1;` cl still emits the copy - a **store of a1's
own value back into a1's own slot**, which retail does not have. Retail's shape only
appears when the SOURCE writes through the parameter itself:

```cpp
// before - the copy is redundant and retail has no such store
i32 coord = a1;
if (a1 > t->m_screenX) result = StepAxisLo(t, a1, cursor, &coord, a3);
t->m_screenX = coord;

// after - the out-param IS the parameter slot
if (a1 > t->m_screenX) result = StepAxisLo(t, a1, cursor, &a1, a3);
t->m_screenX = a1;
```

The same lever applies to a call RESULT that retail parks in a parameter home:

```cpp
i32 cursor = AdvanceB(t, x, y, flags);   // -> a local frame slot
y = AdvanceB(t, x, y, flags);            // -> y's incoming home, which is what retail uses
```

Do not infer the parameter's source width from the width of a later coalesced local.
Every argument owns a four-byte ABI stack slot here, including `char` arguments, and cl
may place an unrelated `i32` local into that dead home. `CPlay::ExecuteCommand` is the
negative control: two `CellHitTest` outputs occupy incoming command-argument homes, but
changing the seven narrow parameters to `i32` destroys two byte-exact callers (61.55%
and 72.67%). One function-scope pair of `i32` coordinate locals removes the synthetic
frame while preserving those caller-proven parameter types. Matching a home address or
write width proves storage coalescence, not source identity; direct `&param` requires
independent type/caller evidence.

```asm
base:   mov ebp,[esp+0x20] | ... | mov [esp+0x28],ebp   ; the redundant copy
target: mov ebp,[esp+0x20] | ... |                      ; slot already holds it
```

STEERABLE, but check the direction: taking `&param` marks the whole variable
address-taken, so cl stops enregistering it. That is FINE when the address escapes at the
first call (`CGameLevel::MoveHandlerB` 78.9 -> the whole prologue + dispatch matched) and
WRONG when the parameter is still pushed as a register argument earlier
(`CGameLevel::MoveHandlerD`: `&a1` split retail's shared argument-push block, 47 -> 107
differing rows). Evidence (2026-07-28, `src/Gruntz/GameLevel.cpp`):
`ResolveMoveUp` 93.51 -> **100 EXACT** (with the early-return shrink-wrap),
`ResolveMoveDown` 83.49 -> **100 EXACT**, `MoveHandlerB`/`MoveHandlerA`/`MoveHandlerC`
prologues. All were filed "register-scheduling wall ... not source-steerable". The
source-identity boundary was confirmed by `CPlay::ExecuteCommand` (2026-08-12).
