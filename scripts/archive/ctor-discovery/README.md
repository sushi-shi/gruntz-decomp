# ctor-discovery (archived)

Spent one-shot tooling used to recover **Gruntz game/engine constructors** and
their class sizes from `GRUNTZ.EXE`. The work is done — kept for reference only,
**out of the importable `gruntz` package** (so they no longer load as
`gruntz.analysis.*`).

The durable output is in the tree:

- **`include/Stub/MallocConstructors.h`** — the discovered constructor classes,
  each `SIZE()`-set from its `operator new` / `malloc` site (real RTTI name where
  the class wasn't already defined, else a `MallocCtor_<rva>` placeholder for a
  non-RTTI class).
- **`src/Stub/MallocConstructors.cpp`** — the RVA-labeled constructor bodies
  (so they delink/diff against retail), `#include`-ing the header.

Only the two most useful scripts were kept (the generic fan-out/compile one-shots
were discarded):

- **`ctors.py`** — parse RTTI (TypeDescriptor → COL → vftable), scan `.text` for
  `mov [this], <vftable>` ctor stores → vtable / ctor candidates.
- **`grind_ctors.py`** — the grinder: unions vtable-stamp + alloc-adjacency +
  sub-object recursion to a fixpoint, keeping ctor-shaped (returns `this`) game
  functions. `--label` emits `MallocConstructors.{h,cpp}`.

They read intermediate CSVs from a scratch dir that is not preserved; `ctors.py`
regenerates its input on run.
