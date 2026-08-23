# `sar` vs `shr` is decided by the SHIFTED OPERAND's declared type, and nothing else

**Tags:** `cpp:int` `cpp:expr` `cpp:inline` | `asm:sar` `asm:shr` | `topic:codegen-idiom` `topic:correctness`

**Symptom.** One function emits `sar r,K` at some sites and `shr r,K` at others for
what looks like the SAME expression — most sharply when the expression is one inline
function expanded several times. Or the two sides of a diff differ only in that
mnemonic. `walls signscan` names it as a `sar`↔`shr` swap at one immediate.

## The rule

cl 5.0 SP3 `/O2` picks the arithmetic shift when the shifted value's declared type is
signed and the logical shift when it is unsigned. **That decision is made on the
operand, not on the expression.** Nothing applied to the RESULT moves it: not a cast,
not a mask that provably discards every sign bit, not the destination's type, not a
surrounding `?:` or comparison.

Sixteen spellings were compiled against the real compiler around the Monolith LCG
(`static long holdrand; return ((holdrand = holdrand*214013L + 2531011L) >> 16) & 0x7fff;`)
and asked for the coin flip `... & 1`. All sixteen emit `sar eax,0x10`:

```
GRN() & 1                      (unsigned)GRN() & 1        GRN() & 1u
{ unsigned v = GRN(); v & 1; }  { int v = GRN(); v & 1; }  (unsigned)GRN() % 2u
GRN() & 0x1                    (GRN() & 1) ? 1 : 0        (unsigned)(GRN() & 1)
MRN() & 1   (the same body as a MACRO, so C1 folds it at parse time)
GRNU() & 1  (the helper's RETURN type changed to unsigned)
GRNC() & 1  (an (unsigned long) cast applied AFTER the shift)
```

`& 0x7fff & 1` folds to `& 1`, so the demanded mask really is one bit and `sar` and
`shr` really do agree on it — cl still keeps `sar`. **The positive control** is the one
change that works, and it changes the operand:

```cpp
// sar eax,0x10 / and eax,0x1
(((holdrand = holdrand * 214013L + 2531011L) >> 16) & 1)
// shr eax,0x10 / and eax,0x1
((int)(((unsigned long)(holdrand = holdrand * 214013L + 2531011L) >> 16) & 1))
```

## What that buys you when you read retail

A `sar`/`shr` SPLIT inside one function is therefore **proof that two different
declared types feed the two shifts** — it can never be a peephole, a scheduling coin,
or an allocation artifact. When the split falls across several expansions of what you
modelled as ONE inline helper, the model is wrong: retail had two helpers, or one
helper and one hand-written expression, reading the same storage through lvalues of
different signedness.

`CStatusBarMgr::StartChipMachineCycle` 0x107d00 is the clean instance. The same LCG
expands eight times; retail shifts the seed `shr` in the four `range == 0` coin arms
and `sar` in the four `% range` arms, against the same `holdrand` relocation. Our
single `WapRand`/`GetRandomNumber` pair cannot produce that, and no spelling of the
coin ARM can either — the fix has to give the coin site an unsigned lvalue over the
same seed, which in our tree means the seed leaving `GetRandomNumber`'s function-local
static (12 game TUs share that one folded copy, so it is a cross-TU change, not a
local one).

## The whole-image census that closes the question

Scanned over `GRUNTZ.EXE`'s `.text` (2026-08-23), so the reading rests on the image
rather than on one row:

| opcode pair | occurrences | adjacent to the shared `holdrand` |
|---|---|---|
| `shr r32,16` then `and r32,imm8` | 4 | 4 - all four in StartChipMachineCycle |
| `sar r32,16` then `and r32,imm8` | 0 | - |
| `sar eax,16` then `and eax,imm32` | 23 | 19 |
| `shr eax,16` then `and eax,imm32` | 1 | 0 - it is the CRT's own `rand` at 0x11fee0 |

So retail spells `>>16 & 1` on that seed with `shr` EVERY time it appears and `sar`
NEVER, while `>>16 & 0x7fff` on the same seed is `sar` nineteen times.  Two different
declared types over one shared static.

Six cl 5.0 SP3 /O2 probes rule the peephole reading out: `sar` survives a single `& 1`
mask, a `& 0x7fff & 1` pair folded to `& 1`, a `& 3`, a `& 0xff`, the assignment form
and the plain-load form.  cl never rewrites the shift; only the operand's declared type
does.  Separately, the `+1` after `cdq / idiv` that identifies `WapRand`'s `% range`
arm occurs at exactly those four sites out of the nineteen, which confirms the helper
is file-local to SBI_RectOnly and its shape is right.

## Reverse use

Beyond the split: `shr` on a value you modelled as `int` is a signedness defect in the
member/local/parameter type, and it is a CORRECTNESS difference, not a score one. The
companion readings are all operand-type facts too — `packed-color-unpack-is-the-
getrgbvalue-macros.md` reads retail's blue-channel `shr` as "the shifted operand is
unsigned", and `hand-expanded-abs-costs-the-cdq-form.md` reads a lone `sar r,0x1f` as
a transcribed sign mask.

## Detection

```
gruntz walls signscan                 # the sweep: a sar<->shr swap at one immediate
gruntz walls signscan 0x00107d00      # the split, site by site
gruntz walls signscan --control       # re-prove the detector fires
```

Do NOT count `shr r,0x1f`: that is the sign-extraction step of cl's SIGNED
magic-divide sequence, so counting it makes the signed form read as the unsigned one.
