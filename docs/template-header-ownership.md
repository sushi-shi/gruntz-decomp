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

The coordinate-pool recycling family now has one inline template member. Its
duplicate free helper/header, expanded recycle macro, local push macro and
manual splices are removed from 51 callers. The remaining iterator APIs are
not collapsed merely because some expansions look alike. Caller matching stays
open; the [controlled source-family correction](patterns/comdat-home-adjudicates-inline-spelling.md#template-family-control-canonical-coordinate-recycling)
supersedes the old missing-emitter and score-based reasons to keep duplication.

The remaining candidate decisions are deliberately narrower than a claim that
the missing original source has been recovered:

| Question | Evidence-backed decision |
| --- | --- |
| Brickz pools as one ordinary primary | Reject: allocation/free and independent pop/recycle consumers prove opposite storage/head roles. Node pool storage/head are +4/+0; cell pool storage/head are +0/+4. Both are 12 bytes, unlike the 16-byte coordinate pool. The cell allocator also initializes its search pointer. |
| Brickz specialization or layout-policy template | Unproven: such a policy can be invented to fit the layout, but no surviving declaration, second use family or debug/mangled witness selects it. Keep the two concrete declarations with `CMapMgr`; do not fabricate a generic `Utils` owner. |
| SDK typed MFC wrappers | Reject for the audited native construction sites: actual VC5 adds derived-vptr stores absent from retail. The status-bar reward queue's stable `Coord*` payload does not turn its native `CPtrArray` into `CTypedPtrArray`. Keep append-via-`Add` and interior `InsertAt` as distinct SDK operations. |
| Custom MFC composition/accessor layers | Original spelling remains unknown. Keep demonstrated owner accessors and native bulk/serialization calls; do not introduce a cast-hiding wrapper without source-family evidence. |
| `PlayerLatency` as a generic accumulator | Unproven: only one eight-byte integer average/count family is observed. Its single sample-combine site and reset/read uses do not establish a template parameter or original generic owner. Preserve its explicit empty destructor and visible open identity review. |

The pinned public game/source search supplies no matching pool, accumulator or
typed-MFC owner declaration for these candidates. That absence is not negative
proof of templates. New source/debug/type evidence can reopen the identities;
layout-compatible specializations alone cannot settle them.

The MFC map follow-up removes `MapOutRef<T>` and the unused concrete lookup
overload, retaining the native `void*&` conversion in the existing generic
forwarders. It also corrects the duplicate SDK `Lookup` identity to `LookupKey`.
See [the constructor and key/value controls](patterns/native-mfc-map-identity-precedes-typed-adapters.md).
The broader constructor/lifetime application work is reserved for a separate PR.

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

## Consolidation checkpoint (2026-09-21, `f5c14f6d5`)

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

## Focused container follow-up (2026-09-21)

The full build passes MAX and every fast/normal gate. All 4,429 historical RVA
maxima from main and the input PR heads survive; none is lowered or lost, and
the same three absent rows remain. `BuildCellAttributes` rises to 90.2473 and
`RebuildSelectionList` to 89.3103 historical MAX. Current started-unit results
are 3,789/4,426 exact and 93.65% fuzzy. The two fresh gate deltas belong to
unchanged `CBoomerang::AdvanceMotion` and `CSpotLight::Update`; both were
diagnosed and their ordered referents checked before banking the new context.
Fourteen edited fingerprints reset their source-specific best values while
retaining historical peaks. Correct source boundaries, not current aggregate
scores, decide this integration.

All 35 controls pass: 18 census tests, 11 real-VC5/header/retail-identity tests,
and six compiler-artifact tests. Twenty-two core functions still pass the direct
raw instruction and ordered-relocation audit. The 33 stale method reviews were
revalidated, not blindly re-keyed: their complete definitions are unchanged and
all normalized instruction/referent pairs match. Twenty-six also pass the simple
raw resolver; five use canonical EH identities, and two one-byte destructors
have fifteen bytes of retail NOP padding outside their decoded bodies.

The refreshed 282-TU census has 4,782 definitions, 31,553 use sites and 22 template
declarations, with no parse errors or uncovered files. Its 5,687-row queue has
no stale, orphan or invalid reviews: 36 retained-authored, six recovered-implicit
and thirteen explicit open-model-conflict decisions. The other rows remain
unreviewed or automatically classified, not certified complete. The new raw-MFC
member inventory contains 49 declarations in 30 owners, representing 68 native
subobjects including arrays; its records are candidates, not type verdicts.
Thirty-five nonexact pool-caller reviews are explicitly open after this change.
