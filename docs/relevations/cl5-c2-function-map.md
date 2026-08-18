# VC5 SP3 `c2.exe` reverse-engineering function map

This is the durable address map for compiler routines identified while explaining
Gruntz code-generation walls. It covers functions whose behavior was inspected in
Ghidra or isolated by a patched-compiler control; it is not a claim that all of C2 has
been named.

The pinned binary is `build/il-probe/re/c2.exe`:

| property | value |
|---|---|
| product | Microsoft Visual C++ 5.0 SP3 C2 back end |
| size | 660240 bytes |
| SHA-256 | `e75aecaf4073b0817ffb638cae5fff636b2e2d1a090daabf8f68dbc954515fae` |
| image base | `0x00400000` |
| `.text` | RVA `0x1000`, raw file offset `0x600`, virtual size `0x8beff` |
| `.data` | RVA `0x8e000`, raw file offset `0x8d200`, virtual size `0x10700` |

For a `.text` VA, the raw file offset is `VA - 0x00400a00`. The semantic names below
are working names, not recovered Microsoft symbols. `P` means proved by direct data
flow plus a causal compiler patch or repeated measured behavior; `H` means high-confidence
static RE; `T` means a tentative boundary marker whose exact domain still needs work.

## Per-function pipeline

| VA | working name | confidence | established behavior |
|---:|---|:---:|---|
| `0x00453305` | `OptimizeAndEmitFunction` | H | Per-function C2 pipeline driver. It reaches register allocation at `0x0042b3e2` and dispatches frame allocation at `0x0042b750`. |
| `0x0042a8aa` | `PrepareAndLowerFunctionTuples` | H | Clears nine per-function globals, allocates eight bitsets, selects the optimized analysis path, then walks current-function blocks/tuples. Called from `0x00453305`. |

The important boundary is that later allocators are entered per function. A TU-global
choice can affect them only by first changing the optimized graph they receive.

## Global optimization and phase state

| VA | working name | confidence | established behavior |
|---:|---|:---:|---|
| `0x0040be34` | `GlobalOptWalk` | H | `globopt.c`-anchored optimizer walker. Uses visitation epochs and sparse sets while walking optimizer nodes/operands. |
| `0x00421a8a` | `ResetGlobalOptFunctionState` | H | Per-function optimizer-state reset; instruction `0x00421ad4` clears epoch `DAT_0048fa34`. |
| `0x0040aa4f` | `HashExpressionModulo101` | P | Recursively hashes expression structure into 101 buckets. Divisor controls `101 -> 97/89/83/79` did not move the `ProbeHeadSoft` boundary. |
| `0x0041256a` | `Initialize511KeyTable` | H | Clears the 511 bucket heads at `DAT_00496000` and installs predefined entries. |
| `0x00412655` | `LookupOrInsert511Key` | P | Uses `abs(signed key) % 511`, 0x14-byte chained nodes, key at +4 and next at +0x10. Divisor controls `511 -> 509/479` did not move the measured phase boundary. |

The 511-periodic `ProbeHeadSoft` result is real, but neither visible modulus table above
is its carrier. Do not rename either one as the phase source. See
[cl5-globalopt-has-a-511-handle-phase.md](cl5-globalopt-has-a-511-handle-phase.md).

## Register allocation

| VA | working name | confidence | established behavior |
|---:|---|:---:|---|
| `0x0042ac33` | `AnalyzeRegisterLivenessAndCost` | H | Builds register liveness/cost sets and calls the descriptor initializer. |
| `0x0042b204` | `InitializeRegisterDescriptors` | H | Initializes per-function register costs, distinguishing caller- from callee-saved descriptors. |
| `0x0042b2c4` | `PickRegisterFromRotatingCursor` | P | Two-pass picker over `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}`. First pass prefers already-used registers; successful picks advance `DAT_004911a8`. |
| `0x0042b3e2` | `AllocateRegistersByTupleOrder` | P | Walks current-function blocks and tuples, resets the cursor per block, asks for a register, and binds the first eligible def/use operand. Tuple order is the allocation coin. |
| `0x00435f77` | `BindRegisterToValue` | H | Writes the chosen register into a value and rewrites the associated tuples. |
| `0x0040181e` | `SparseSetContainsBit` | H | Generic sparse-bitset membership test. In the picker it acts as the per-value eligibility query; it is not intrinsically a register-allocation function. |

The full rotating-cursor proof and source-domain controls are in
[cl5-c2-register-picker-is-a-rotating-cursor.md](cl5-c2-register-picker-is-a-rotating-cursor.md).

## Optimized frame allocation

This is the chain that explains why `CFaderShape::RenderTile` is stable under TU-state
permutation.

| VA | working name | confidence | established behavior |
|---:|---|:---:|---|
| `0x0042b750` | `DispatchFrameAllocation` | H | Clears the current frame result and chooses optimized `0x0042b7aa` or alternate `0x00457b10` allocation. |
| `0x0042b7aa` | `AllocateOptimizedFrame` | P | Coordinates reset, local collection and numbering, CFG data-flow, interference construction, slot coloring, final offsets, and cleanup. |
| `0x0042b909` | `ResetFrameAllocator` | P | Zeros all frame-list/slot counters and links, resets every temporary descriptor's frame offset to -1, and allocates a fresh local set. This is the decisive per-function reset. |
| `0x0042ba01` | `ForEachTemporaryDescriptor` | T | Applies a callback across a compiler-owned descriptor registry. Frame reset uses it to clear descriptor state; the registry's wider ownership is not established. |
| `0x0042ba3f` | `CollectStackLocalDescriptors` | P | Walks only the current function's blocks and operands and admits eligible stack-resident descriptors through `0x004367f0`. |
| `0x004367f0` | `InsertOrReorderFrameLocal` | P | Orders descriptors by size ascending, then occurrence count descending. Repeated sightings increment +0x34 and move the descriptor earlier; exact ties retain discovery order. |
| `0x004368cb` | `InsertFrameLocalAfter` | H | Doubly-linked-list insertion helper used by `0x004367f0`; supports insertion at the head or after a descriptor. |
| `0x0042bbb9` | `NumberFrameLocals` | H | Traverses ordered descriptors, assigns dense IDs at +0x38, and accumulates total size and special-kind counts. |
| `0x0042bc12` | `CanUseSpecialFrameMode` | T | Calls `0x0047d3bd` to bound special operand occurrences before selecting a frame mode. Exact source-level feature remains unidentified. |
| `0x004401cd` | `BuildBlockFrameDataFlow` | P | Builds per-block def/use/live sparse sets from the current function's CFG and operands, then iterates them to a fixed point. |
| `0x0043fd5a` | `AccumulateFrameInterference` | P | Propagates block liveness and accumulates the local-interference sets consumed by slot compatibility. |
| `0x0043f93b` | `ColorFrameSlotsAndAssignOffsets` | P | Forms reusable slot groups, optionally sorts them, then walks slots in reverse and writes aligned final frame offsets. |
| `0x00440cbd` | `ReuseOrAppendFrameSlot` | P | Reuses a sufficiently sized slot only when the candidate is absent and its interference set does not intersect the slot's members; otherwise appends a slot. |
| `0x00440e40` | `SeedSpecialEightByteFrameOffset` | H | Establishes a special 8-byte offset/alignment seed under particular function/local flags. The source-level construct is not yet named. |
| `0x00459eb7` | `SortFrameSlotsByDensity` | H | Quicksort-like ordering by `(aggregate use count * 1000) / slot size`, used when the frame measure exceeds `0x80`. |
| `0x00459f8b` | `SwapFrameSlotRecords` | H | Swaps two 0x14-byte records in the slot array at `DAT_004910b8`; called only by `0x00459eb7`. |
| `0x0040ff46` | `SparseSetsIntersect` | H | Returns true when two sparse bitsets share any set bit; this is the decisive slot-interference query. |
| `0x0040df40` | `AllocateDownwardAlignedFrameRange` | H | Advances the current frame depth downward by a requested size with requested alignment and returns the resulting offset. |
| `0x0040df8a` | `FrameAlignmentForSize` | H | Computes the alignment argument used by frame allocation, including a distinct 2-byte case. |

The descriptor fields established in this chain are:

| offset | meaning |
|---:|---|
| `+0x20` | storage size |
| `+0x28` | assigned frame offset |
| `+0x2c` / `+0x30` | next / previous ordered descriptor |
| `+0x34` | occurrence count |
| `+0x38` | dense local ID |

At VA `0x00436875`, the original `jge` is the equal-size occurrence-order stop in
`InsertOrReorderFrameLocal`. A disposable `jge -> jle` patch changed unchanged
`RenderTile` from 747 bytes / hash `e7b93ee4b9f7c019` to 752 bytes / hash
`83ae40bf73433abb` and broadly recolored its homes. That is a causal ownership test,
not a compiler patch to retain.

## Alternate and special frame paths

| VA | working name | confidence | established behavior |
|---:|---|:---:|---|
| `0x00457b10` | `AllocateAlternateFrame` | T | Alternate/simple assignment path selected by `0x0042b750`; walks special operand trees and assigns offsets directly. Its complete option domain is unresolved. |
| `0x00476533` | `WalkAlternateFrameLeftFirst` | T | One of two mutually recursive graph walkers used only by `0x00457b10`; assigns an unseen descriptor's offset and traverses one operand order. |
| `0x00476585` | `WalkAlternateFrameRightFirst` | T | Mirror walker used only by `0x00457b10`; differs in which operand chain it visits first. |
| `0x00457883` | `AssignSpecialNestedFrameRecords` | T | Walks a special nested-record shape and applies alternate frame assignment. Exact source construct remains unresolved. |
| `0x0047d3bd` | `BoundSpecialFrameOperands` | H | Counts operand opcodes `0x120`/`0x124` in eligible tuples and rejects when the count exceeds a mode-dependent limit of one or three. Called only by `0x0042bc12`. |

## Sparse-bitset primitives used by these passes

These are generic compiler utilities. Naming them prevents a contextual use, such as the
register picker's call to `0x0040181e`, from being mistaken for their intrinsic meaning.

| VA | working name | confidence | established behavior |
|---:|---|:---:|---|
| `0x004013c5` | `IterateSparseSetBits` | H | Stateful iterator over set bits in ascending sparse-word order; uses global iterator state and is not re-entrant. |
| `0x0040181e` | `SparseSetContainsBit` | H | Membership test. |
| `0x00401863` | `AllocateEmptySparseSet` | H | Allocates an empty sparse-set container for the requested pool/type. |
| `0x0040189f` | `InsertSparseSetBit` | H | Inserts one bit, adding an ordered sparse word when needed. |
| `0x00401967` | `ClearSparseSet` | H | Returns every sparse word to its size-class free list and clears the container head. |
| `0x004019df` | `UnionSparseSetInto` | H | Inserts missing sparse words and ORs their payload bits into the destination. |
| `0x00401a34` | `CopySparseSet` | H | Replaces a destination with a copy of a source set. |
| `0x00401a84` | `SubtractSparseSet` | H | Removes all source bits from a destination. |
| `0x00401ae3` | `ReleaseSparseSet` | H | Clears and releases a sparse-set container back to its allocator. |
| `0x00401b72` | `RemoveSparseSetBit` | H | Removes one bit and unlinks an empty sparse word. |
| `0x0040223a` | `SparseSetsEqual` | H | Tests set equality. |
| `0x00402674` | `SparseSetCopyMinusUnion` | H | Computes `(B - C) union D` into the destination via `0x00402690` and union. |
| `0x00402690` | `SparseSetCopyMinus` | H | Copies one set and subtracts another. |
| `0x004026ed` | `ResolveCompilerHandle` | T | Normalizes handles above `DAT_0048e2e8` through a 0x0c-stride remap table, then indexes the global referent table. The handle kinds remain unnamed. |
| `0x0043fd0d` | `CopyThenUnionSparseSet` | H | Frame-analysis wrapper that copies one set and unions another. |

## Data landmarks

| VA | meaning |
|---:|---|
| `0x0048fa34` | global-optimizer visitation epoch, reset per target function; not proved to carry the observed TU-global phase |
| `0x00491080` | flag that makes register number 6 (EBP) available when frame-pointer omission permits it |
| `0x004910b8` | base of 0x14-byte frame-slot records |
| `0x00491100` | register rotation table `{1,2,3,7,8,4,6,0}` = `{EAX,ECX,EDX,ESI,EDI,EBX,EBP,stop}` |
| `0x004911a4` | register-table length, 8 including terminator |
| `0x004911a8` | rotating register-table cursor |
| `0x0049475d` | register descriptors, stride 0x50 |
| `0x00496000` | 511 keyed-table bucket heads; proved not to carry the observed `ProbeHeadSoft` phase |

## Safe continuation rule

Use these names as navigation hypotheses, then re-check callers, field use, and the first
real divergence before applying them to a new wall. A `P` entry supports reverse use in
its measured domain. An `H` entry supports navigation but may still cover more cases than
its working name. A `T` entry should be refined with a source probe or patched-C2 control
before it drives a source change.
