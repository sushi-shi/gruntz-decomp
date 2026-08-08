# cl5 cuts a ctor chain OUT-OF-LINE at a depth that varies PER `new`-SITE
tags: cpp:ctor cpp:new cpp:inline | asm:call asm:eh topic:wall
symptoms: base has NO `/GX` EH frame where retail does (`push -1 / push <handler> / mov fs:0,esp`), a smaller `sub esp,N`, and retail has a `call <ILT thunk>` at a `new T` site where the base stamps the base-class fields inline; retail also sets an EH state (`mov [esp+N],1/2/3`) around each such `new`
confidence: 9/10 (three distinct cut depths measured in two functions; every OOL body identified by address)
variants: ool-ctor-device (docs/patterns/ INDEX), phantom-method-reconstruction.md

`new CSBI_MenuItem` runs a four-deep ctor chain
`CSBI_MenuItem -> CSBI_Image -> CSBI_RectOnly -> CStatusBarItem`, all four defined in-class
in headers. cl5 inlines a PREFIX of that chain and emits a `call` for the rest, and **the
cut depth is not a property of the declaration** - the same ctor is called at one site and
inlined at another **inside a single function**:

| site | retail | cut after |
|---|---|---|
| `BuildStatusBarTabs` @0xffde0, the three `new CSBI_RectOnly` | 5 inline stores, no call | nothing (whole chain inlined) |
| `BuildStatusBarTabs`, all five `new CSBI_MenuItem` | `call 0x22c0` -> `??0CStatusBarItem@@QAE@XZ` @0x1005d0 | depth 3 |
| `BuildGameMenu` @0x101580, sites 1-3 | `call 0x1e88` -> **`??0CSBI_RectOnly@@QAE@XZ` @0x101fa0** (27 B, unnamed in our tree) | depth 2 |
| `BuildGameMenu`, site 4 | `call 0x22c0` | depth 3 |
| `BuildTabzDialog` @0x10a340, `new CSBI_Image` | `call 0x22c0` | depth 2 |

The consequence is structural, not cosmetic: an out-of-line ctor **can throw**, so cl must
be able to `operator delete` the half-constructed object. That forces the whole function
into a `/GX` EH frame with a state variable (`mov [esp+N],1`, `2`, `3` at successive
`new`s, reset to `-1` once each ctor returns), and grows the frame. A fully-inlined chain
contains no call, cannot throw, and gets no frame at all. So one inliner decision moves the
prologue, the epilogue, the frame size and the register allocation of the entire function.

## What does NOT control it

- **Declaration form.** All four ctors are in-class inline on both sides; `??0CSBI_RectOnly`
  is inlined at some sites and called at others in the same object file.
- **`#pragma inline_depth(3)`** reproduces `BuildStatusBarTabs` (71.58 -> 78.21, EH frame and
  all five calls appear) but is a global setting and therefore cannot express a per-site cut;
  it drives `BuildGameMenu` the wrong way (72.33 -> 66.93). It is a fitted artifact - do not
  land it.
- **Per-site typed locals** instead of one reused `CStatusBarItem* it` - byte-identical
  (71.58 either way). Worth doing for cleanliness, not for the match.

## Consequences for the worklist

Any function whose `eh 0->1` differs in
`python -m gruntz.audit.insn_count`-style prologue comparison and which contains a `new` of
a deeply-derived class is in this family. Until the heuristic is understood these are
`@early-stop` walls, and the OOL bodies retail emitted (`??0CSBI_RectOnly@@QAE@XZ` @0x101fa0
is still unnamed) need `RVA_COMPGEN` pins in whichever TU emits the COMDAT.
