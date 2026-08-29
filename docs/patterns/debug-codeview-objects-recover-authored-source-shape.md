# Original debug objects can recover the authored source shape without a PDB

tags: cpp:local cpp:scope cpp:loop cpp:inline | asm:codeview asm:coff | topic:source-oracle topic:wall
symptoms: a structurally complete function remains on one compiler island after broad local and TU-state searches, while an original release contains paired Debug and Release static libraries but no source file or PDB
confidence: 10/10

An original LithTech release contains both Debug and Release
`ButeMgrNoMFC.lib` archives.  Their `ptins.obj` and `ptadd.obj` members are the
same Patricia-tree family as Gruntz's `zPTree`.  The source `.cpp` files and PDB
did not survive, but the Debug members still contain old CodeView records:

- the original `ptins.cpp` and `ptadd.cpp` paths;
- `BPREL32` local names, types, and frame positions;
- lexical block records that distinguish function-scope and nested locals;
- type records tying the locals to `zPTreeNode`, not a layout-compatible
  reconstruction.

The paired Release members corroborate the optimized operation and branch
topology.  The surviving `ztools.h` supplies the class layer: `zPTreeNode`, its
`ptr` selector, the `bit` and `diffpos` helpers, and the `root`/`p`/`q`/
`sbits`/`preview` state.  The retail Gruntz binary remains the authority for
revision differences and final code generation.

For `ptins.cpp`, the records recover this exact function-scope local census and
declaration order:

```cpp
int* bp;
int newbranch;
int stack[32];
int dp;
int branch;
zPTreeNode* t;
```

They also prove a top-tested `while (p)`, the path-stack update, and an outer
`if (p) ... else root = t` whose common root store is important to C2.  For
`ptadd.cpp`, the records recover `newbranch`, `dp`, and `t` at function scope,
with the second splice direction `b` in its nested block.  Restoring the full
family, rather than permuting the hand transcription, produced:

| Gruntz body | Prior MAX | Exact result |
|---|---:|---:|
| `zPTree::insert` `0x1933b0` | 78.7362% | `0x28f`, 235 instructions, 6 calls, 35 branches, 3 returns, 10 relocations |
| `zPTree::add` `0x16db90` | 93.0619% | `0x206`, 194 instructions, 9 calls, 21 branches, 4 returns, 15 relocations |

This falsifies the earlier conclusion that `insert` required a guarded
bottom-tested loop.  That conclusion was reproducible only inside the
hand-transcribed source family.  On the complete sourced helper/local/shared-
tail base, the authored top-tested loop is byte-exact.  The earlier 128-state
TU campaign and the 60-cell structural campaign bounded their old source
hashes; they did not bound an untested source family.

## Reverse use

1. List archive members and pair identically named Debug and Release objects.
2. Confirm the Debug member's source path and function identity before using
   its records.
3. Read `.debug$S` and `.debug$T`.  Older CodeView streams may be only partly
   accepted by current LLVM tools; a parser failure is not evidence that the
   records are absent.  Walk each length-prefixed record and decode the local,
   type, and block records directly when necessary.
4. Restore the complete declaration/helper/local/control-flow family.  Do not
   copy debug stack offsets into source or infer optimized register choices
   from `/Od` code.
5. Compile the real Gruntz TU with the pinned VC5 toolchain and compare retail
   instructions, ordered relocations, and referents.  Treat revision-specific
   differences as explicit lineage-ledger decisions.

The source-specific adoption state is recorded by the `nolf-zptree-*` rows in
`config/lithtech_lineage.tsv`.
