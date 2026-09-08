# Typed container use replaces manual compiler methods

tags: cpp:template cpp:array cpp:constructor cpp:destructor cpp:inline | asm:call asm:coff asm:mov | topic:source-model topic:compiler-artifact

Audit the complete declaration/use family when a concrete container owns
methods that also look like emitted template or member-lifetime operations.
Use-site types select template arguments even where several arguments would
produce identical erased code. Names used for a recovered primary remain
reconstruction names unless surviving source establishes their spelling.

The action registry already used the recovered `zDArray<CString>` and
`zDArray<CActHandler>` declarations, but callers still expanded raw growth,
error handling and CString construction. Replacing those regions with the
sourced `T& operator[]` and exposing its definition removed twelve redundant
helpers/accessors. Both emitted indexers and the erased accessor remain exact.
In the controlled array-only build, `RegisterGruntActions` rose from 5.06812%
to 97.792915%; overall current fuzzy moved from 94.77745% to 93.98467%.
A per-caller raw/typed/report variant was a reconstruction of an inline cut,
not evidence for another authored API.

The input array provides a second control: its complete layout and users give
`CFixedPtrArray<CInputDevBase,32>`. The emitted `Clear` and `Add` are exact;
`FillFrom` expands the formerly external `Add` and changes from 126 bytes,
one call and six branches to 122 bytes, zero calls and seven branches. This
is expected template visibility evidence, not a reason to erase its element
type. The compiler rejects passing `Coord*` to this specialization's `Add`.

Four list heads share the existing erased `CLTBaseList` implementation but
recover different element types from insertion and traversal: `WwdRegion`,
`SoundSample`, `SoundBufferNode` and `SoundTask`. Their typed getters preserve
the original erased insertion/deletion boundary. Their implicit teardown
emits the same one-byte methods. The grid head's implicit constructor and
vector deleting destructor are exact too. The task list retains its authored
buffer/tag filtering owner.

An empty destructor is **not sufficient evidence** for omission. Removing
`SoundBufferInstance::~SoundBufferInstance` emits only a five-byte jump to
its base destructor. Retail first writes the derived vptr, then jumps: eleven
bytes and two ordered relocations. Restoring the explicit empty destructor
restores that source layer. Inlining the base destructor is a separate change
and also changes this body. Keep the authored base and derived declarations;
never transcribe the vptr store manually. `SoundTask` likewise has an authored
nonvirtual destructor that resets a polymorphic vptr. Conversely, ordinary
member-array lifetime can be implicit: `CGruntCellRec`'s CString array needs
no source-written empty constructor or destructor.

The two Brickz pools are a structural negative control. Their element types
are known, but their allocation/free routines use opposite storage/free-list
member offsets. Wrapping both in invented layout specializations would retain
the duplicated model. Recover the owner/node policy before unifying them.

For reverse use:

1. Join the pylibclang definition, owner, callers and allocation sites to the
   actual COFF symbols and source RVA claims. Empty bodies, template-looking
   names and equal sizes are candidate signals.
2. Recover typed storage and its complete constructor/destructor/accessor
   family; replace caller expansions with that API. Compile with real VC5.
3. Compare vptr transitions, vector-helper arguments, raw bytes and ordered
   relocations. Keep authored empty special members where omission loses a
   retail operation. Preserve real virtual declarations and owner behavior.
4. Bind the actual emitted specialization with `RVA_COMPGEN`. Explicit
   instantiation of the used primary/member is sufficient; no unused
   realization functions or volatile compiler ordinals are needed.

The compiler-artifact verifier now rejects `p->CString::CString()` through
its public gate. A negative control exercises that previously missed spelling;
a normal qualified method call passes. Typed placement construction remains
only in the shared array constructor/index implementation. The audit and
per-record dispositions are in [the template audit](../template-model-audit.md).
