# A wrong literal CONSTANT is invisible to every other sieve — diff the immediates

**Tags:** cpp:global cpp:switch | asm:cmp asm:and asm:imul | topic:tooling topic:correctness topic:mis-model

## Symptom

A function scores high — often 95-100% — and is still wrong at run time, or a
function is stuck in the 70s and every structural check comes back clean: the
block skeleton aligns, `--branches --diff` says the branch sequence AGREES, the
callee multiset matches, `store_offsets` reports no one-sided member store, and
the reloc-addend sieve is quiet.

The defect is a bare number: a mask, a divisor, a shift count, a buffer size, a
packed state tag.

## Why nothing else catches it

Each existing sieve is scoped to a different operand class, and none of them
covers *non-relocated literals*:

| sieve | sees |
|---|---|
| `gruntz sema disasm --diff` | instructions — but it **MASKS large immediates** as `<addr>`, so a `/9`-vs-`/30` divisor shows only as a downstream `sar` |
| `--branches --diff` | conditional-branch mnemonics and targets |
| `gruntz.audit.store_offsets` | *where* we store, never *what* |
| [reloc-addend](reloc-addend-is-masked-diff-the-addends.md) | only operands that carry a relocation (`g_tbl + K`) |
| objdiff fuzzy % | one changed immediate in a 600-byte function is a rounding error |

## The extraction is exact, not heuristic

`llvm-objdump` prints AT&T for these objects, and in AT&T an immediate is
**precisely a `$`-prefixed literal**:

```
andl   $0x1f, %eax          <- immediate            COUNT
movl   0x14(%ebx), %eax     <- memory displacement  ignore (tracks the frame)
je     0x58b <$L27601>      <- branch target        ignore (tracks code layout)
```

So no heuristic is needed to separate constants from displacements — the two are
lexically distinct. Compare the per-function multiset of immediates between the
base obj and the delinked target obj:

```
python -m gruntz.audit.immediates                 # whole tree
python -m gruntz.audit.immediates --min-pct 95    # only near-matches
python -m gruntz.audit.immediates --unit ddsurface
```

## Three noise classes must be subtracted, or the report is unreadable

The tool does the first two automatically; the third is yours to recognise.

1. **Relocated immediates** (`movl $g_foo+0x20, %ecx`). Our side carries the
   addend, the delinked side carries a linked address or a delinker-chosen
   `symbol + addend`. objdiff masks the operand, so we must too. Filter by
   *"does a relocation land inside this instruction's bytes"*, never by
   magnitude — a magnitude cut-off silently drops genuine large constants.
2. **Frame adjusts** (`subl $0x4c, %esp`). The frame size is a regalloc outcome;
   a 4-byte delta is the single commonest benign row.
3. **Instruction selection for the same operation** — see below.

## The false-positive that costs the most time (measured, 2026-08-10)

`andl $0xff, %eax` and `movzbl %al, %eax` are both `(u8)x`; **only the first
carries an immediate**. A one-sided `0xff` / `0xffff` / `0xffffffff` is
therefore usually a narrowing *both* sides perform with different encodings.

Proof, paid for the hard way: `CShadeTableCache::GammaTable` 0x14e9f0 showed
`OURS-ONLY 0xff x3`, from three `static_cast<u8>` on the interpolated channels.
Deleting the casts to match retail's apparently-unmasked code took it
**94.07 -> 86.29**; the same edit on `GreyTable` 0x14eef0 (`OURS-ONLY 0xff x2`)
took it **92.86 -> 89.14**. The casts were correct all along.

Two more benign shapes:

- **A strength-reduced loop bound.** `for (i = 0; i < 256; i++) g[i] = …`
  compiles either to a counter (`cmp $0x100`) or to a pointer compare against
  `&g[256]`, i.e. `g + 0x200` — which shows as a one-sided `0x200`
  (`CDDSurface::Blit168`) or `0x400` (`CDDSurface::DecodeBmp`).
- **A cross-jump.** cl merges two identical arms retail keeps apart, so a
  constant retail stores twice appears once on our side —
  `CDDrawSurfaceChildA::SetGeometry` 0x1644a0 has two `WORLDERR_CREATE_DEVICE`
  (`0xbb9`) blocks in retail (the switch `default:` reached by the jump-table
  range check, and the `err == NONE` path) and one in ours. Writing the explicit
  `default:` arm does **not** split them (measured byte-identical), which is the
  same over-merge wall that module already documents.

## What survives all three is the real class

A mask, divisor, shift count or packed tag that only ONE side has anywhere.
Live rows from the first tree-wide run (107 functions at >= 90%):

- `CTileActionEvent::MorphByTool` 0x113420, 98.66% — retail has `$0x60a0a0a`
  **four times** and our source contains that constant nowhere.
- `CTriggerMgr::ApplySwitch` 0x6d300, 93.64% — retail-only `$0x5020501`.

A packed four-byte constant appearing several times is a lookup table folded
into an immediate; its absence means a whole decision table is missing, not a
byte mis-typed.
