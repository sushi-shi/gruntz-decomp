# A field-pair copy between two objects loads BOTH before it stores either

tags: cpp:member cpp:assign cpp:alias | asm:mov | topic:codegen-idiom
symptoms: retail emits `mov r1,[src+a] / mov r2,[src+b] / mov [dst+x],r1 / mov [dst+y],r2` (and lets the `src` register die) where we emit `mov src,[this+N] / mov r,[src+a] / mov [dst+x],r / mov src,[this+N] / mov r,[src+b] / mov [dst+y],r` - one extra reload of the source pointer, and a whole-function register-role rotation behind it
confidence: 9/10

## The mechanism

```cpp
dst->m_x = m_object->m_screenX;
dst->m_y = m_object->m_screenY;
```

`m_object` is a member of `this`, so the store `dst->m_x = ...` may alias it. cl 5.0
has no type-based alias analysis, so it must RE-LOAD `m_object` for the second
statement: load / store / load / store, with the pointer live across both stores.

Retail's shape - both loads, then both stores, with the source pointer register
reused for one of the loaded values - can only come from source that materialises
both values BEFORE the first store. Two locals do it:

```cpp
i32 focusX = m_object->m_screenX;
i32 focusY = m_object->m_screenY;
slot->m_focusX = focusX;
slot->m_focusY = focusY;
```

Retail, `CExitTrigger::CExitTrigger` 0x3ecf0:

```
mov edx,[eax+0x60]      ; m_screenY
mov eax,[eax+0x5c]      ; m_screenX  (kills the pointer register)
mov [ecx+0x220],eax
mov [ecx+0x224],edx
```

## Measured

`CExitTrigger::CExitTrigger` 93.79 -> 97.76, one build.

Local DECLARATION ORDER is irrelevant: `focusY` first and `focusX` first both score
97.76 to five decimals - cl schedules the two independent loads itself. Do not read
retail's `[0x60]`-before-`[0x5c]` load order as evidence for a declaration order.

## The two-arg setter is byte-identical and costs ripple - prefer the locals

An inline setter on the destination class (`GruntzPlayer::SetFocus(i32 x, i32 y)`)
produces the SAME 97.76: MSVC evaluates call arguments right-to-left, so both loads
happen in the caller before the expansion's stores, which is the same shape. But
the declaration lands in a widely-included header and moved the /O2 decl-count
window: 9 fresh sub-bank rows, two of them 100 -> 88/89
(`seams-stay-local-shared-headers-ripple`). The locals form is `.cpp`-local and
byte-identical. With one write site in the whole tree the setter is also weakly
evidenced, so it is not the dev shape the binary argues for.

Detection: `walls diagnose` will say REGALLOC/SCHEDULING, because the two forms have
the same call set and skeleton. Look for the DOUBLED `mov reg,[this+N]` of the same
member offset around a pair of stores to a foreign object - that is the signature,
and it is a source bug, not a register coin.

related: [global-reload-runs-prove-scoped-pointer-locals.md](global-reload-runs-prove-scoped-pointer-locals.md),
[inlined-container-method-reloads-members.md](inlined-container-method-reloads-members.md)
