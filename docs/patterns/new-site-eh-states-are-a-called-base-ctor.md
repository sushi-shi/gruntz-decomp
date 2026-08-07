# EH states at every `new` are a CALLED base ctor, not a destructible local
tags: cpp:eh cpp:ctor cpp:new | asm:call asm:mov | topic:wall topic:eh
symptoms: `insn_count --eh` reports retail has an EH frame and the recompile has none, in a big builder function full of `new`; retail's trylevel counts 0,1,2,...,N with one state per `new`, and there is no object in the source that could need unwinding
confidence: 9/10

`insn_count --eh` flags a function where retail carries a `/GX` frame and we do not, and the
usual reading is "retail has a destructible local we never reconstructed". **That reading is
wrong when the states line up one-per-`new`.**

A `new T` whose constructor is a real **call** must free the raw block if that ctor throws, so cl
opens an EH state around it:

```
push   0x3c
call   <operator new>
add    esp,0x4
mov    ebp,eax
mov    DWORD PTR [esp+0x1c],ebp
cmp    ebp,ebx
mov    DWORD PTR [esp+0x38],ebx     <-- trylevel 0: "raw block live"
je     <null>
mov    ecx,ebp
call   <??0CStatusBarItem@@QAE@XZ>  <-- the base ctor, NOT inlined
mov    DWORD PTR [ebp],<vftable>
```

The same `new T` with the ctor chain **fully inlined** emits no call, cannot throw, and gets **no
state**. So the EH frame is a readout of cl's inliner, not of the source.

**And the inliner's choice is not source-steerable here.** Within ONE retail function the identical
`new CSBI_MenuItem` both calls and inlines `CStatusBarItem::CStatusBarItem()`:
`CStatusBarMgr::BuildStatusBarTabs` (`0xffde0`) calls it at 4 sites and inlines it at the 5th;
`BuildGameMenu` (`0x101580`) calls at 6 of 9; `LoadTabSprites` (`0x102250`) calls at ~19 sites and
inlines the tail. The class, the chain and the spelling are the same at every one. Watch for the
ILT aliasing when you tabulate this - `0x22c0` and `0x1e88` are two thunks to the SAME
`??0CStatusBarItem@@QAE@XZ` at `0x1005d0`, and screening for only one of them manufactures a
false "inline" site.

**Measured lever (does not close it).** `#pragma inline_depth(3)` reproduces the call at the
depth-4 site (`CSBI_MenuItem -> CSBI_Image -> CSBI_RectOnly -> CStatusBarItem`) and makes the EH
frame appear, taking `BuildStatusBarTabs` 71.58 -> 78.21 - but it costs `BuildGameMenu`
72.33 -> 66.93, is net zero on the unit (`sbi_rectonly` 88.86 -> 88.81), still gets the tail sites
wrong, and there is no evidence retail's source carried such a pragma. Tree-wide it is clearly
wrong: `#pragma inline_depth(3)` in `rva.h` takes Overall 87.23 -> 87.13 and exact 3296 -> 3267.

So: when `--eh` fires on a `new`-heavy builder, tabulate the `new` sites first. One EH state per
`new` with a ctor call = this wall, and the function's logic is already right. A destructible
local shows up differently - a state opened at a declaration and closed at a scope exit, with a
`??1` call on the normal path too.
