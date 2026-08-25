# A `return` inside the null guard is what lets cl home a local in the dead parameter slot

- **confidence** c9
- **tags** `cpp:branch` `cpp:local` `cpp:call` | `asm:cmp` `asm:xor` `asm:mov` | `topic:codegen-idiom`

## Symptom

A tiny accessor-shaped function (one guarded call, one member store) sits in the 60s with
the *same instruction count* as retail. Both sides materialise a zero, both spill one local
to a stack slot, both reload it - but every register is swapped and the zero store differs:

```
 base                                   target (retail)
 push esi                               mov  eax,DWORD PTR [esp+0x4]   <- param read BEFORE the push
 mov  esi,ecx                           push esi
 mov  ecx,DWORD PTR [esp+0x8]           mov  esi,ecx
 xor  eax,eax                           xor  ecx,ecx                   <- the zero lands in the RESULT reg
 test ecx,ecx                           cmp  eax,ecx                   <- guard compares against that zero
 je   <join>                            je   <join>
 ...
 mov  DWORD PTR [esp+0x8],0x0           mov  DWORD PTR [esp+0x8],ecx   <- register store, not an immediate
 ...
 mov  eax,DWORD PTR [esp+0x8]           mov  ecx,DWORD PTR [esp+0x8]
 mov  DWORD PTR [esi+0x54],eax          mov  DWORD PTR [esi+0x54],ecx
```

The tells are (a) retail loads the parameter into a register **before** the first `push`,
(b) retail's guard is `cmp <param>,<zeroreg>` instead of `test <param>,<param>`, and
(c) retail stores the zero **from a register** where we store an immediate.

## Cause

Retail's source guards with an early `return`, not with an `if` whose body is the whole
function:

```cpp
// ours - 64.21%
void CInGameIcon::SetupSprite(const char* category) {
    SoundCue* cue = 0;
    if (category != NULL) {
        void* found = 0;
        g_gameReg->m_world->m_soundRegistry->m_cues.Lookup(category, found);
        cue = static_cast<SoundCue*>(found);
    }
    m_cue = cue;
}

// retail - 100.00% EXACT
void CInGameIcon::SetupSprite(const char* category) {
    if (category == NULL) {
        m_cue = NULL;
        return;
    }
    void* found = 0;
    g_gameReg->m_world->m_soundRegistry->m_cues.Lookup(category, found);
    m_cue = static_cast<SoundCue*>(found);
}
```

With the early `return`, the parameter's live range ends at the guard on the failing path and
at the `push` on the succeeding one, so cl promotes it to a register in the very first
instruction and the **incoming parameter's stack home becomes free** for the address-taken
out-param. That in turn frees `ecx` to carry the zero across the guard, which is why the
comparison becomes `cmp eax,ecx` and the spill becomes a register store.

With the `if`-wraps-everything form the local's live range overlaps the parameter's, cl
cannot coalesce, and it allocates a fresh slot (`push ecx` / `pop ecx` appears in the
prologue - a frame retail does not have).

## Rule

When a small function's instruction count already matches but every register is rotated and
you see `mov <reg>,[esp+N]` as instruction 0 on the retail side, rewrite the guard as an
early `return` and let the assignment fall out of the tail. Measured: `CInGameIcon::SetupSprite`
0x99b10, 64.21 -> **100.00 EXACT**, frame 0 -> 0 (the `if`-wrapped single-variable spelling
scored *worse* than the original by adding a `push ecx`).

Corollary from the same session: the inverse lever also exists - a call plus an early return
placed *ahead* of a local's definition can **suppress** the coalesce. Count the rets in the
target first; retail sometimes genuinely emits a separate early-exit tail.

related: [identical-arms-need-distinct-locals.md](identical-arms-need-distinct-locals.md),
[error-report-guard-falls-through-to-a-shared-return.md](error-report-guard-falls-through-to-a-shared-return.md)
