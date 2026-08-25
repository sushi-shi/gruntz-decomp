# An early `return` before a destructible local KILLS cl's dead-parameter-home coalesce

tags: cpp:branch cpp:return cpp:local cpp:eh | asm:sub asm:lea | topic:codegen-idiom
symptoms: frame exactly 4 bytes bigger than retail; retail constructs a `CString`
  (or other 4-byte local) at `lea ecx,[esp+N]` where `[esp+N]` is an INCOMING
  PARAMETER's home slot, and your build allocates a fresh frame slot for it instead;
  every later `[esp+K]` shifted by 4
confidence: 9/10

cl 5.0 *will* place a local — including an EH-tracked `CString` — into the home slot
of a parameter that is dead by that point. Whether it does is decided by the SHAPE of
the guard in front of the local, and a controlled probe settles it in four compiles:

| probe body | frame | CString lands at |
|---|---|---|
| `if (arg==9) return 1;` + body | `0x10` | the param home |
| `m_sub->Call(); m_saved=g; if (arg==9) return 1;` + body | `0x14` | a fresh slot |
| `m_saved = g;` (no call) `if (arg==9) return 1;` + body | `0x10` | the param home |
| **`m_sub->Call(); if (arg!=9) { body }`** | **`0x10`** | **the param home** |

So it is the **combination** — a CALL *and* an early `return` ahead of the local's
definition — that suppresses the coalesce. Neither alone does. Inverting the guard so
the body is *wrapped* rather than the guard *returning* restores it:

```cpp
// WRONG - frame 0x14, `s` gets its own slot
m_mgr->m_voiceManager->PauseAllVoices();          // a call ...
if (arg == GAMESTATE_HELP) { return 1; }     // ... plus an early return
RECT r; CString s; s.LoadString(IDS_PLEASE_WAIT);
...
return 1;

// RIGHT - frame 0x10, `s` lands in the dead `arg` parameter's home
m_mgr->m_voiceManager->PauseAllVoices();
if (arg != GAMESTATE_HELP) {
    RECT r; CString s; s.LoadString(IDS_PLEASE_WAIT);
    ...
}
return 1;
```

STEERABLE. `CMulti::LeaveState` 0xb63f0 and `CPlay::LeaveState` 0xc8b80 (identical
bodies) both **92.75 -> 100.00 EXACT** — the guard inversion took the frame
`0x14 -> 0x10` and put the CString in the param home, and the residual
`ShowHudMessage` block then closed with the by-value size temp
([byvalue-size-accessor-temp.md](byvalue-size-accessor-temp.md)). Both had been filed
as a non-steerable "dead-parameter-home coalesce" wall after ~15 spellings (CRect,
block scopes, decl order, an LPRECT alias, writing through the parameter) and a
14-flag optimizer sweep — none of which is the knob. **The knob is control flow.**

## How to test it rather than guess

Do not spray spellings at the real function. Write the 20-line model into
`build/probe/p.cpp` — the class, the dead param, the guard, the destructible local —
compile it standalone and read `sub esp,N`:

```
python3 scripts/gruntz/core/cc_wrap.py --out build/probe/p.obj --src build/probe/p.cpp \
        -- /nologo /c /O2 /MT /GX
llvm-objdump -d --no-show-raw-insn build/probe/p.obj
```

Each compile is ~2 s, so a five-way bisect of the statements ahead of the local costs
less than one build of the real tree and gives a categorical answer.

## Two ways to be wrong about this - check both BEFORE inverting

1. **Both sides already use param homes, just for different variables.** That is a
   placement permutation, not a suppressed coalesce, and the inversion makes it worse:
   `CWarpStoneFly::Init` 0x109bd0 (retail homes a delta scalar in `fragment`'s slot and
   `dist2` in `owner`'s; we do the reverse) went 91.33 -> 82.26, reverted. Read WHICH
   variable is in the home first.
2. **Retail genuinely wants the early return.** Count the `ret`s / read the exit tails
   before inverting - the ret counts in `gruntz walls diagnose`, or
   `gruntz walls diagnose <rva>`. `CPlay::DrawCursorSaveUnder` 0xd0b30
   has the same call+early-return+DDSCAPS shape, but retail emits a SEPARATE early-exit
   epilogue (`jne` into its own `xor eax,eax` / pops / `ret 4`); inverting the guard
   merges that tail away and cost 99.99 -> 90.57, reverted. Same caveat as
   [positive-gate-enables-shrink-wrap.md](positive-gate-enables-shrink-wrap.md): count
   the rets first.

related: positive-gate-enables-shrink-wrap.md, frame-size-counts-the-locals.md,
return-inside-dtor-scope-splits-the-exit-tails.md
