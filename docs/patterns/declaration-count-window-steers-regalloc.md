# The TU's file-scope DECLARATION COUNT picks the register scheme - map the window, then land a real `#include`

tags: cpp:include cpp:class | asm:mov asm:lea asm:push | topic:codegen-idiom topic:regalloc topic:wall
symptoms: a function is block-exact and instruction-selection-exact but every register is
rotated (`eax`<->`edx`, one extra callee-saved `push`/`pop`, `mov eax,[disp32]` vs
`mov edx,[disp32]`), and NO spelling inside the function moves it by a single byte -
hand-authored matrices, the rename forest and the generated AST trees all score identically
confidence: 10/10 (6 functions, 6 TUs; the graded probe is a repeatable measurement)

This is the quantitative form of
[string-h-intrinsics-reallocate-the-tu.md](string-h-intrinsics-reallocate-the-tu.md):
cl 5.0 picks a function's register assignment partly from **how many file-scope
declarations it has parsed before reaching it**. You can MEASURE the missing count.

## The measurement: a graded prototype axis

Put N throwaway prototypes above the first project include and sweep N:

```json
{ "schema": 1, "source": "src/Image/ImagePool.cpp", "rva": "0x176da0",
  "axes": [ { "name": "ndecl", "find": "#include <ComOutRef.h>\n",
              "options": [ {"name":"n00"},
                           {"name":"n01","replace":"#include <ComOutRef.h>\n\nint m34_probe_0(int);\n"},
                           ... ] } ] }
```

```
python -m gruntz.permute.match_variants <src> <rva> --max-depth 0 --axes-from a.json -o m.json
python - <<'EOF'
import json; p='m.json'; d=json.load(open(p)); d.pop('candidates',None); json.dump(d,open(p,'w'))
EOF
python -m gruntz.permute.batch_source_variants m.json --limit 32 --top 8
```

`CRezImage::FillRectAt` 0x176da0, N = 0..24 - a clean **period 16 with an 8-wide window**:

| N mod 16 | 0-7 | 8-15 |
|---|---|---|
| score | 66.44 (size 77) | **100.00 (size 75, retail)** |

(measured N=0..7 bad, 8..15 good, 16..23 bad, 24 good). So bit 3 of the declaration
count selects the scheme, and the TU is 8 declarations short of retail's parse state.

`CImage::BlitShadeNorm` 0x154270 has a narrower, multi-modal window over N = 0..16:
99.85 at N<=8, **100.00 at N = 9, 10, 11 and 16**, 99.94/99.86 between. So the WINDOW IS
FUNCTION-SPECIFIC - always map it, never assume the Blowfish "one declaration wide" figure.

A class with **inline member function bodies** is the densest single carrier, which is why
the `tu_state_member` island family flips these functions 5/5 where `tu_state_class`
(data members only) and `tu_state_function` (free functions with bodies) do nothing - it
contributes several member declarations at once.

## The only landable form: a REAL missing `#include`

Throwaway prototypes and probe classes are diagnostics, never commits. Once the window is
known, search real headers - a header the TU does not include contributes its whole
declaration set, so roughly half of them land in the window. Two landed this way on
2026-08-08, and both are ALSO plain self-sufficiency fixes:

| TU | added include | why it is genuinely needed | result |
|---|---|---|---|
| `src/Gruntz/GruntPowerupSprite.cpp` | `<Rez/FrameClock.h>` | `Update()` reads `g_engineFrameDelta`, whose owner header the TU did not include | 89.82 -> **100.00** |
| `src/Gruntz/SBI_Image.cpp` | `<Gruntz/StatusBarMgr.h>` | `SetupImage` takes `CStatusBarMgr*`; the TU had only the forward decl | 74.07 -> **100.00** |

Prefer a candidate that owns a symbol the TU already uses (a global's declaring header, a
parameter type's defining header): then the include stands on its own merits and the score
merely confirms it.

**The include is a CLAIM about retail's TU, and the byte score is the only evidence for
it - so test every one.** A blanket sweep adding `<Gruntz/StatusBarMgr.h>` to the eight
other TUs that name `CStatusBarMgr` was REFUTED by the oracle: `CAniPlayer::RenderCel`
98.12 -> 76.00 and `CSBI_ImageSet::Render` 98.58 -> 86.74. Keep the include only where it
reaches byte-exact; revert it where it is neutral or worse.

Also measured to reach 100.00 under a probe but with no real-header carrier found yet:
`CRezImage::FillRectAt` 0x176da0 (66.44, `<Gruntz/GruntzMgr.h>` or `<Wap32/ZVec.h>` both
flip it but neither is used by the TU), `CImage::BlitShadeNorm` 0x154270 (99.85,
`<Gruntz/GameRegistry.h>` / `<Io/FileMem.h>`), `CDDrawWorkerHost::Save` 0x163780 (99.98,
`<Gruntz/TriggerMgr.h>` / `<Image/ImagePool.h>` / `<Gruntz/StatusBarMgr.h>`).

## Where the count is NOT the knob (measured, 50 islands each, dead flat)

`CPlay::StepScroll` 0xd1ac0 (88.03), `CTriggerMgr::NotifyCell` 0x79fb0 (85.89),
`CTriggerMgr::ToggleRegionA` 0x7d450 (79.13), `CGrunt::ResolveArrivalNeighbor` 0xf26f0
(86.61) - 50/50 island cells identical to the baseline, so their residue is intra-function
and no amount of TU state reaches it. `CDDrawChildGroup::SumWeighted` 0x15aaf0 moves only
99.852 -> 99.889.

2026-08-08, `sbi_rectonly` + `gruntentrancearrival`: 155 more dead-flat cells, equal to
six decimal places. `CStatusBarMgr::SetSpritePos` 0xfe860 (25 graded free-function
prototypes AND 80 `tu_state_*` islands across all twelve families),
`CStatusBarMgr::HitTestRects` 0xffcb0, `CStatusBarMgr::PlaceCursorTarget` 0x105800 and
`CGrunt::RearmAttackAnim` 0x61940 (17/17/13 graded classes-with-inline-bodies, the
densest carrier). `#include <string.h>` above the first project include is neutral in
that TU too. The substitution machinery was verified live in the same run - a
deliberately broken option is dropped as a compile failure - so the flatness is real.
Two of the four then closed on a plain source-shape fix
([one-flag-local-carries-both-tests.md](one-flag-local-carries-both-tests.md),
[retail-duplicates-small-return-epilogues.md](retail-duplicates-small-return-epilogues.md)).
**Map the window ONCE per TU, on one function. If it is flat there, do not re-run it on
that TU's other functions - spend the budget on the disassembly instead.**

related: [string-h-intrinsics-reallocate-the-tu.md](string-h-intrinsics-reallocate-the-tu.md),
[commutative-operand-order-is-canonical.md](commutative-operand-order-is-canonical.md),
[dead-eight-byte-coord-temp-is-unreproduced.md](dead-eight-byte-coord-temp-is-unreproduced.md)
