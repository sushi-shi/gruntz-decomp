# Semantic abstraction queue

`gruntz walls inventory` answers *which* current functions remain below 100%
and preserves the historical-MAX campaign order. `gruntz walls diagnose`
answers which machine-code feature differs first. Neither answer alone says
which semantic level should be inspected before changing source.

Use:

```sh
gruntz walls abstractions --todo
gruntz walls abstractions --todo --module DDrawMgr --module Wwd
gruntz walls abstractions --level textual
```

The command joins the live inventory, normalized-object diagnosis, calibrated
aggregate sieves, source-visible inline and function-like macro definitions,
and source-hash-scoped reviews. It persists no queue. `--todo` uses exactly the
inventory exclusions: EH-band funclets, historically exact bodies, and current
terminal reviews do not become source-edit work.

## Levels

| Level | Question to settle first |
| :-- | :-- |
| `identity` | Does the relocation name the right object, alias, or owner? |
| `object` | Is the ABI/type/object/aggregate boundary modeled correctly? |
| `call` | Is this an out-of-line call, an expanded helper, or a duplicated call site? |
| `textual` | Did the operation originate as an inline function, macro, or open code? |
| `algorithm` | Is the branch/loop/return/tail-sharing structure authentic? |
| `expression` | With higher levels clean, which spelling/lifetime/schedule differs? |
| `pairing` | Is the report/model/normalized pair itself usable? |
| `state` | This source already reached exact; which compiler/TU state restores it? |
| `generated` | Which source-owned parent generated this EH funclet? |

The level is a route, not a reconstruction verdict. In particular, aggregate
copy shapes can also be pointer walks, incoming arguments, scalar spills, or
tail-merged blocks. The row names the detector and direction so its instruction
neighbourhood can be adjudicated before a declaration changes.

The command also audits fingerprint freshness. If a unit's per-function cache
is stale, its hash-scoped terminal reviews cannot safely suppress queue rows;
the warning names the units and requests `gruntz verify fingerprints` (a full
build normally supplies the same refresh).

## Why textual origin is above expression work

VC5's front end does not treat all semantically equivalent origins alike.
`CDDSurface::Blit1624` reached exact only when a general pixel-pack operation
was expressed as a function-like macro; five inline-function association forms
all stopped at the same 99.765625 local maximum. `FlashTable` similarly moved
from 85.0179 to 99.2032 when its repeated channel operation recovered a general
interpolation macro. A REGALLOC diagnosis therefore does not close the helper
or macro question. The source-aware census promotes repeated visible macro
expansions and file-local/repeated inline calls into the `textual` queue before
local expression permutations.

Historical MAX remains the primary sort key. Semantic level chooses the next
evidence-bearing action within that campaign order; it does not justify skipping
a lower structural bank for a convenient high-score expression wall.
