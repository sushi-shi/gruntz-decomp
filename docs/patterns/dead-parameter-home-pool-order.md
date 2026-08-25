# WHICH dead parameter home a temp lands in: what controls it, and what provably does not

tags: cpp:local cpp:param cpp:scope | asm:lea asm:mov asm:sub | topic:regalloc topic:wall
symptoms: frame size AGREES, the instruction stream agrees, but one `lea ecx,[esp+N]`
(a CString/temp constructor) or one spill store picks a different incoming parameter's
home slot than retail; the arms of one `switch` disagree with each other about which
home they use
confidence: 9/10 (the rules below), 2/10 (steering the last free choice)
variants: macro-local-decl-order-picks-param-home.md,
early-return-kills-the-param-home-coalesce.md,
null-guard-early-return-frees-the-parameter-home.md

cl 5.0 reuses a dead incoming parameter's stack home for a local. Three lanes have now
plateaued on *which* home it picks. A 22-cell standalone probe
(`__thiscall Fn(void* g, int x, int y)` with per-arm `CString` temps, `/O2 /MT /GX /GR`)
plus two sweeps on real functions settles what is reachable from source.

## Reachable from source (use these)

| axis | effect |
|---|---|
| **is the parameter still read FROM MEMORY later** | decisive. A home whose parameter is re-loaded after the temp's live range starts is NOT a candidate. With `x`/`y` register-resident, the temp took one of theirs; with both re-read for the `Format` call, only `g`'s home was free and the temp took it. |
| **size of the local** | decisive. Only 4-byte objects go into homes. A `RECT r;` (16 B) got a fresh `sub esp,0x10` area even with three homes free. |
| **declaration order of SIMULTANEOUSLY-LIVE temps** | decisive for the assignment. Two co-live `CString`s take the two free homes in declaration order, ascending: declared `a,b` gives `a`@lower/`b`@higher, declared `b,a` gives `b`@lower/`a`@higher. |
| **disjoint scopes** | temps in sibling blocks SHARE one slot (two `CString`s in two sibling blocks both `lea ecx,[esp+0x24]`). |
| **address-taken** | NOT a disqualifier - an escaped `int n; TakeI(&n);` still took a parameter home. |

## Measured NO-OPS (stop trying these)

* **Block-scope nesting DEPTH.** `if (…) { CString msg; }`, one extra `{}`, and two extra
  `{}` are byte-identical. Only sibling-vs-nested matters, never depth.
* **The order of the operands that feed the guard.** `Hit(g, x + y)` and `Hit(g, y + x)`
  emit the same two loads in the same order.
* **The `Format` argument order.** Reversing it does not move the temp.
* **Local NAMES, under /O2.** Renaming `dir` -> `zdir` in `CRezArchive::ImportDirectoryTree` left
  the frame byte-identical. (Contrast `od-local-slot-ordering.md`: the name-hash rule is
  a `/Od` rule and does not apply here.)
* **Declaration ORDER of address-taken ARRAYS.** All 40 permutations of the five
  `_splitpath` buffers in `ImportDirectoryTree` produced an identical frame layout. Array slot
  order is driven by the SIZES, not by the source order - which is why a frame-size
  mismatch is a buffer-set fact, never an ordering one.
* **TU declaration count.** 18 cells of throwaway prototypes above the first project
  include (`declaration-count-window-steers-regalloc.md`'s knob) left
  `CTriggerMgr::WireTileSwitchLogic` 0x6c130 dead flat at `0x30 x10 / 0x34 x4 / 0x38 x10`.
  The banking trick does not reach this.

## The residual free choice is NOT source-reachable

When two or more homes are genuinely free at the temp, the pick is a function of the
whole function, not of any construct near the temp. Two probe cells whose arm is
**instruction-identical with identical register assignment** - same loads, same order,
same registers - put the temp in different homes; the only difference is how many other
`case` arms the function has.

```cpp
// identical arm text, identical arm codegen, DIFFERENT slot:
//   3-arm function -> lea ecx,[esp+0x20]   (x's home)
//   1-arm function -> lea ecx,[esp+0x24]   (y's home)
case 3: if (Hit(g, x + y) == 0) { CString msg; msg.Format("m %d %d", x, y); … }
```

`WireTileSwitchLogic` 0x6c130 remains the parameter-home calibration: identical
`sub esp,0x10`, the same parameter-home reads, and retail funnels all twelve
`CString msg` temps into ONE slot (`[esp+0x30]`, the slot `cx` vacated) where we
spread them over three. Per-arm declarations, bracing the clamp/lookup prologue
into its own scope, swapping the clamp locals' order and swapping the key
expression's operands are all byte-identical no-ops there.

It is **not a pure frame-only case**, as an older version of this pattern claimed.
After `diagnose` learned llvm's i386 `calll` spelling, candidate has 96 calls to
retail's 95: all eight source `StepArrivalDrop` sites emit here, while retail
cross-jumps the fixed RIGHT arm into the nested CURRENT/EAST tail and carries the
arguments there in its opposite `x`/`y` register coloring. The same function
fingerprint has repeatedly alternated between this 96-call state and retail's 95-call
shape as unrelated included headers changed; the corresponding 90.29/92.43 MAX-ledger
oscillation is the integration-scale control. The slot displacement and repeated-call
delta are two downstream symptoms of the same whole-function allocation decision;
neither is proof of a missing statement or inline body.

**So: before filing a slot difference as pool order, rule out the reachable axes above -
above all the FRAME SIZE.** In the two other functions opened on this lane the "spill
pool" reading was a misdiagnosis: `CRezArchiveDir::ReadDirectoryBody` 0x13a640 was a 4-byte record
cursor plus a cached collection pointer (76.18 -> **100.00 EXACT**) and
`CRezArchive::ImportDirectoryTree` 0x13b300 was wrong buffer SIZES, one missing buffer and
`strcpy`-for-`strcat` (79.76 -> 99.67). Genuine pool order is the residue of last
resort, and when it is genuine it is a permuter job.
