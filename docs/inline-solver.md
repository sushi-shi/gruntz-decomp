# Inline trace and inline-wall solver

`gruntz walls inline-trace` records the decisions cl 5.0's inline pass makes in
a unit; `gruntz walls inline-solve` replays them and searches for the source
difference that would reproduce retail's call set. Use them on a wall whose
`gruntz walls diagnose` class is inline/call-set, after referents are settled.

```sh
gruntz walls inline-trace sbi_rectonly                 # every root with sites
gruntz walls inline-trace sbi_rectonly --root Render   # roots matching TEXT
gruntz walls inline-solve 0x101580                     # rva, mangled or Class::Member
gruntz walls inline-solve 0x101580 --max-edits 3 --seconds 120
gruntz walls inline-solve 0x101580 --what-if --cb 700 --add 12:2 --remove 7 \
    --cost '?GetAt@CPtrArray:60'
gruntz walls inline-reach 0x11bee0                     # force retail's decisions, score
gruntz walls inline-solve --check sbi_rectonly         # replay == trace, per root
gruntz walls inline-solve --check-all
```

## The traced back end

cl 5.0 runs its back end as a separate program, `c2.exe`, which `/B2<path>`
replaces. `build/c2-shim/` holds a private copy of the pinned `c2.exe`, the
real `MSPDB50.DLL` renamed `mspdb50r.dll`, and a stand-in `mspdb50.dll` built
from `scripts/gruntz/walls/c2shim/` with the pinned compiler. `c2.exe` loads
`mspdb50.dll` from its own directory; the stand-in forwards each of the 16
imports to `mspdb50r.dll`. With `GRUNTZ_C2_TRACE` set, its `DllMain` also
writes jumps into the inline pass. `c2.exe` has no relocations, so these
addresses are fixed:

| address | hook | records |
| --- | --- | --- |
| `0x42492d` | inline pass entry (`FUN_0042491e`) | the root and its front-end cost `cb` |
| `0x424eef` | each candidate site (`FUN_004249cb`) | callee, `cb`, depth, depth allowance, budget, sites remaining, running size |
| `0x424f15` | argument-count compare | actual and formal argument counts |
| `0x424f36` | the budget compare, after the argument-count and depth gates | forced decisions (`GRUNTZ_C2_FORCE`) |
| `0x424f59` | every gate passed | admit |
| `0x424f79` | the running-size add (after the budget charge) | charge |
| `0x424c0b` | recursive walk of an expanded body | the nested budget share |

Each hook re-executes the instructions it displaced. The shim refuses to load
if the bytes at any hook address differ from the pinned `c2.exe`. Every trace
must also pass an identity check: the traced compile has to reproduce the
unit's build object byte for byte (TimeDateStamp and the per-compile
number in anonymous-namespace names masked) or the trace is
refused. Rebuild the unit first if it is stale. Traces are cached in
`build/inline-trace/<unit>.json`, keyed by the build object, the unit source and the newest header.

## The replay rule

One site list at a time, in tuple order, with a running budget per list:

1. refuse when the argument counts differ or the depth exceeds the site's
   allowance;
2. refuse when the callee costs more than 40 and more than the budget left;
3. refuse when the running size exceeds 35000;
4. otherwise expand: charge the cost (over 40 only), give the body
   `trunc(budget / sites remaining)`, then charge what the body spent.

C2 refuses some admitted sites later, depending on the call's context (seen
with by-value class arguments and returns). Some of these refusals happen
after the charge and some before it. The trace records which, and the replay
keeps each such site refused, charging it only if the trace shows a charge.

The root's budget is `clamp(2 x cb, 1000, 35000)` and its running size starts
at `cb`. Every expansion adds its cost to the running size, and an enclosing
expansion adds the same spending again. `--check` replays every traced root
and lists any site whose budget, running size or verdict differs from the
trace. A solver result is meaningful only for a root that replays exactly.

## The search

The target is retail's minus our retained calls, for each traced callee, from
the normalized pair. Call-set differences on callees outside the traced sites
are reported separately; no budget edit can reach them. Edit kinds:

| edit | meaning in source |
| --- | --- |
| `cb` | the caller's own front-end mass (more or fewer statements) |
| `insert` | K free (cost <= 40) candidate calls after a site, either in the root's list or inside a helper's body (every expansion of that helper) |
| `remove` | a site retail does not have |
| `callee` | a helper body lighter or heavier by d at every site; a cost of 1000 or more leaves the candidate set |

The search tries every single edit, then pairs among the best partial fits,
then each single combined with a root-`cb` shift. `--max-edits 3` adds a
greedy third edit. Each fit reports the root-`cb` window it holds over.
Fits that differ only in insertion position, insertion count, or cost delta
are merged into a range.

A refused site has unknown children unless the same callee expanded somewhere
else in the unit; the replay then borrows that expansion's site list. A fit
that expands a site whose children remain unknown is marked `incomplete`.

A fit names a place and a kind of difference to look for, such as a missing
accessor, an open-coded helper, or a heavier caller. It is not source to adopt.
Budget-steering code is forbidden; restore the original structure that the fit
points to, then confirm with `gruntz match`.

## Forcing retail's decisions

`gruntz walls inline-reach` answers whether a wall is an inline-budget wall
at all, before any search for source. `GRUNTZ_C2_FORCE` names a rule file the
shim reads, one rule per line:
`root<TAB>owner<TAB>callee<TAB>occurrence<TAB>E|K`. The root is a mangled name
or `*`; owner and callee are substrings (`*` matches any); occurrence counts
the matching budget compares within the root from 1 (0 means every one). At
the budget compare, `E` takes C2's admit path and `K` its decline path. Sites
refused earlier, by argument count or depth, cannot be forced. An empty rule
list must reproduce the build object, the same identity check as tracing.

Each round aligns our call stream with retail's from the canonicalized retail
object, so the score matches what `gruntz permute` reports. It proposes rules
at the differences: `E` on a site of a callee that we call and retail does
not, and `K` on an expansion of a callee that retail calls. It compiles each
proposal and keeps the one that most shrinks the traced call difference,
using the score to break ties. When no single rule helps, it tries
two-rule chains, since an expansion brings its own nested sites along. The
verdict is one of:

- **inline-budget wall:** retail's decisions make the function exact;
- **partial:** forcing moves the score toward retail, and the rest is another
  wall class;
- **not inline:** the traced calls already agree, or forcing does not help.

Each rule's `need` is the budget change at that site that would make C2
decide the same way unforced, or the reason a budget change cannot (a cost
of 40 or less is exempt; the running size is over 35000). A forced object is
a diagnostic: never bank it and never write source that steers the budget.
Use `inline-solve` to find the source difference that produces the need.

