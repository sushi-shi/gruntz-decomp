# Constant hoisting is decided before tail merging

## Signature

Retail keeps a small constant in a fresh callee-saved register for a whole
region (`push ebx` / `mov ebx,1`, then `bl`/`ebx` at every store, push, EH-state
store, bit test, and the return). Ours emits the same instructions with the
immediate, and the calls, branches, and relocations are identical.

## Observation

cl decides whether to give such a constant its own callee-saved register by
weighing its uses against the temporaries competing for registers in the
function. It makes this decision before cross-jumping merges identical block
tails. So two sources with the same final code can hoist differently:

```cpp
if (crypt) { data = new iostream(...); } else { data = new fstream(...); }
data->precision(100);                       // once, after the join: no EBX

if (crypt) { data = new iostream(...); data->precision(100); }
else       { data = new fstream(...);  data->precision(100); }   // EBX = 1
```

Both forms merge into one `mov [esi+0xa4],eax` / virtual-base store tail at the
join. Only the second one hoists the constant 1, which matches
`CButeMgr::Save` (0x171640) exactly. In that function the decision is marginal.
Each of the following changes also turned the hoist on: one more `true` store,
`precision(1)` in place of `precision(100)`, or removing any single
virtual-base `clear`/`precision`/`delete`. Four extra `CString::GetLength`
temporaries turned it back off. Plain member stores of other constants had no
effect.

## Limits

- The join form can be the authentic one elsewhere. Retail evidence decides.
  The pattern only shows that a matching tail does not fix where the source
  placed the statement.
- Dead loads, local types, named results, chained assignments, and loop
  spellings that emit the same code did not move the decision in Save. Only
  changes that reach the optimized graph can move it.
- This does not measure the weighing function itself. No threshold or unit is
  established.
- The per-arm form is the exception. A mechanical pass over 280 join sites in
  120 sub-100 functions (every statement after an if/else moved into each arm,
  or a common arm tail moved after the join) raised 7 functions, left 86
  unchanged and lowered 187. The winners were a `return`, a call, or a member
  store whose retail copy sits inside each arm's scheduled block; the losers
  were mostly cross-jumped tails that retail keeps shared.
