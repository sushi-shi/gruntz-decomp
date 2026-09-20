# Shared template ownership

PR #79 incorporates the typed-container follow-up from #80 and the method/use-site
census from #81. It also imports the census coverage controls from `6fc68fb0d`
and `a6cb7147b`, without their later constructor/lifetime application work.
The broader constructor-family and SDK-adapter branches are not included.

## Public headers

| Family | Canonical shared header | Consumer-specific declarations |
| --- | --- | --- |
| `zDArray<T>`, `_zvec`, `_zdvec` | `ZTools/ZDArray.h`, `ZTools/ZVec.h` | `Gruntz/ActReg.h` and `Gruntz/TypeKeyColl.h` |
| `zSymTab<T>`, `zPTree`, `zPtrColl` | `ZTools/PTree.h` | `Gruntz/ActRegistry.h` for the integer action-ID registry; Bute retains its own typed consumers |
| Error handling and bit vector | `ZTools/Error.h`, `ZTools/BitVec.h` | No game registry declaration in the library headers |
| `CFixedPtrArray<T, Capacity>` | `Utils/FixedPtrArray.h` | `DinMgr2/InputDeviceGroup.h` for the alias; `Gruntz/InputDeviceGroup.h` for the game global |
| `FreeNodePool<T>` and nested node | `Utils/FreeNodePool.h` | `Gruntz/CoordPool.h` for the coordinate specialization/node alias and global |
| `CLTList<T>` over the erased list | `Lith/TypedList.h` | WWD region, sound sample/node/task, and stream-voice consumers; task filtering remains with the sound owner |

`ZTools` follows the surviving library family, not the TU that happened to emit a
COMDAT. The pinned [NOLF ztools.h](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/LT2/lithshared/incs/ztools.h)
owns the error, bit-set, vector and symbol-table layers together and names
`ztools.lib`. Revision-specific adoption decisions remain solely in the
`nolf-zdarray-*` and `nolf-zsymtab-*` lineage-ledger rows.

The public array header now provides its complete declaration, lifetime and
indexing definitions. Consumers do not select an implementation fragment or
include a game registry to instantiate an array. Generic headers are independent
of `Gruntz`, `Bute` and `Wap32`. The recovered `Utils` names are reconstruction
names, not a claim about an original library path.

Out-of-line definitions, global storage, and `RVA_COMPGEN`/`RVA_DYNINIT` bindings
stay in their evidence-backed emitting TUs. Header ownership alone does not
license moving retail storage or changing contribution boundaries.

## Coverage and remaining questions

The expanded audit visits free-function templates, conversion operators, unions
and unused template bodies with delayed parsing disabled. It records implicit
special-member declaration candidates independently of actual COFF emission.
Record-name heuristics rank candidates; they do not prove completeness.

The two Brickz pools still have opposite storage/free-list field roles, and no
shared owner policy has been established. They remain open, not fabricated
specializations. Typed MFC collection recovery is also a separate complete-family
question; see [the queued audit](todos/recover-typed-mfc-pointer-collections.md).
Typed pointer use alone does not prove a different concrete MFC container or
justify changing its vptr/lifetime behavior. Existing pool recycling helper
factoring and broader constructor visibility reviews are not closed by this PR.

## Reproducible controls

Run in the pinned environment:

```sh
nix develop -c python3 -m unittest discover -s scripts -p test_audit_template_models.py -v
nix develop -c python3 -m unittest discover -s scripts -p test_container_headers.py -v
nix develop -c gruntz build
nix develop -c python3 scripts/audit-template-models.py --workers 4
```

The header tests compile the public interfaces with actual VC5, verify the array,
pool/node, list and symbol-table extents, and reject an unrelated pointer passed
to the input array. A transitive dependency control rejects importing consumer
headers into the shared families. The census tests include unused-template and
parser-snapshot negative controls. This is coverage of the audit mechanism, not
a claim that every row in the derived review queue is resolved.

## Integrated validation (2026-09-21)

The final full build passes MAX and every fast/normal gate. All 4,429 historical
RVA maxima from main and the three input PR heads are preserved; none is lost or
lowered. Current started-unit results are 3,790/4,426 exact and 93.64% fuzzy.
`RegisterGruntActions` is 97.7929% current, with its historical 100% retained.
The twelve fresh ownership-stage gate deltas all have unchanged function source
fingerprints; their call/CFG and operand/referent comparisons were inspected
before banking the new declaration context. No caller body was respelled to
recover a score, and no new bounded-wall claim is made.

All 17 census controls, eight public-header controls and six compiler-artifact
controls pass. Twenty-two core methods pass direct raw-instruction and ordered
relocation/addend comparison with retail, including both array indexers, handler
lifetimes, the erased accessor, pool Push, input Clear/Add, list lifetimes,
cell-record lifetimes and symbol-table teardown. This count does not certify
all caller or exception-handling code as exact.

The fresh census covers 282 TUs, 4,784 owned definitions and 31,558 use sites,
with zero parse errors or uncovered files. Its derived queue has 5,689 rows and
no orphan/invalid review entries. Thirty-three older method reviews are correctly
marked stale after source/macro-context changes; they are not re-keyed into
fresh certifications. The remaining unreviewed and stale rows stay visible.
