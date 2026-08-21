# `a->b->c` guarded call: hoist the MIDDLE link into a local so cl chains through eax

**Tags:** cpp:local cpp:method cpp:branch | asm:mov | topic:codegen-idiom topic:regalloc

## Symptom

A null-guarded call down a three-deep pointer chain

```cpp
if (m_world->m_level->m_mainPlane != 0) {
    m_world->m_level->m_mainPlane->CenterScrollB();
}
```

emits the right three loads but parks the *intermediate* link in the callee's
`this` register instead of chaining through the first:

```
retail:  mov eax,[esi+0xc]   ; m_world
         mov eax,[eax+0x24]  ; m_level      <- reuses eax
         mov ecx,[eax+0x5c]  ; m_mainPlane  <- only the last load goes to ecx
base:    mov eax,[esi+0xc]
         mov ecx,[eax+0x24]  ; m_level      <- already in ecx
         mov ecx,[ecx+0x5c]
```

Two instructions, same length, same order. (The register pair varies —
`edx`/`ecx` instead of `eax`/`ecx` in a busier frame — but the shape is the same:
retail overwrites the FIRST register, we overwrite the LAST.)

## Fix

Bind the middle link to a named local:

```cpp
CGameLevel* lvl = m_world->m_level;
if (lvl->m_mainPlane != 0) {
    lvl->m_mainPlane->CenterScrollB();
}
```

Binding the *last* link instead (`CDDrawWorkerHost* pl = m_world->m_level->m_mainPlane;`)
does **not** work — it produces the same code as the fully-inline chain.

## Why

Written inline, the chain is one address expression: cl evaluates it into the
register the consumer wants (`ecx`, the `__thiscall` receiver) and keeps
extending it in place. With `lvl` a named local, `m_world->m_level` is a value
with its own (short) live range, so it is materialised into the scratch register
already holding `m_world` — and only the final `->m_mainPlane` load targets
`ecx`.

## Evidence

`src/Gruntz/Play.cpp`, two functions in the same TU, one edit each (2026-07-28):

- `CPlay::ProfileDeltaFrame` 0x0ca0a0 — 99.89% -> **100% EXACT** (sole residue).
- `CPlay::EnterMode` 0x0d6fa0 — 99.87% -> **100% EXACT**. This one had a *second*
  residue 20 instructions further down (`mov eax,[esi+0x1cc]` + the short `A3`
  store form, where retail uses `edx` + `89 15`); the `lvl` hoist fixed BOTH,
  because the whole tail's colouring follows from the eax/edx assignment at the
  chain. It had been filed as a "large state-machine wall, not source-steerable".

`CActionOptionsMenuBar::Render` 0x0094c0 supplies the address-taken-argument
extension (2026-08-21). Moving the back-buffer local before the clip-rectangle
copy first reproduced retail's post-`WrapCoord` statement order and raised
82.31% to 90.12%. Binding `m_world->m_level` before passing `&sx` and `&sy`
then made the complete opening through `WrapCoord` byte-exact and raised the
function to 94.48%. Binding the last link (`CDDrawWorkerHost*`) and leaving the
full chain inline both stop at 90.12%. A 33-state forest and all 256 depth-1/2
source variants each produced one compiler island; the remaining residue is
three repeated post-call `mov edx,[g_gameReg]` loads where retail uses the
five-byte EAX form.

## Related

- [member-store-direct-not-via-temporary](member-store-direct-not-via-temporary.md)
  — the opposite direction: a temporary that should NOT exist.
