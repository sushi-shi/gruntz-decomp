# SDK members explain an apparently authored constructor helper

tags: cpp:mfc cpp:templates cpp:constructor | topic:identity topic:source-shape

A hand-transcribed container can conceal both its SDK template identity and the
SDK members of its elements. The reverse signature here is an MFC growth and
serialization family, a 0x28-byte element containing two rectangles, and a
three-byte `mov eax,ecx; ret` function called from one placement-construction
loop but absent from a stack declaration.

The old model called that function `InitRezElem`, typed the rectangles as
`RECT`, and copied `CArray` into `CRezBufferObject`. An out-of-line authored
constructor added a call in `CFaderMesh::ApplyInit`, so the old review concluded
that the element must remain POD. That conclusion tested one declaration
boundary, not the complete SDK family.

Controlled VC5 `/O2 /MT /GX` probes used the actual `AFXTEMPL.H`:

| Element declaration | Serializer result | Constructor result |
| --- | --- | --- |
| Two `RECT` members, POD | 0x1c0-byte padded section; eight relocations | No constructor emitted |
| Two `RECT` members, explicit inline empty constructor | Same serializer result | Empty constructor expanded away |
| Two `CRect` members, implicit constructor | Exact 0x1ce-byte retail body (0x1d0 padded section), ten relocations | Exact three-byte implicit element constructor |

For the final row the payload matches the previously exact serializer byte for
byte. Every ordered relocation agrees after the two justified identity changes:
`InitRezElem` becomes `RezElem40::RezElem40()` and `ZeroRecords` becomes MFC's
`ConstructElements(RezElem40*, int)`. The latter emits the same 0x23-byte byte-fill
body. The element still occupies 0x28 bytes; both rectangle members are real MFC
objects. The complete serializer proves more than an isolated empty constructor
or coincidental layout agreement.

The surrounding rectangle locals matter too. Assigning a Win32 `RECT` to a
`CRect` selects its SDK assignment overload and introduces `CopyRect`; using
`CRect` for the assembled/dispersed local rectangles selects the compiler's
ordinary same-type assignment. Restore declarations and uses together. The
caller and standalone `SetSize` can still vary with real template visibility
and inlining; retaining the copied container to preserve that earlier source
hash would restore the modeling defect.

Use this signature to reopen an old "constructor-shaped helper is not a
constructor" conclusion. Inspect SDK member constructors, implicit special
members, placement construction, and the complete template body. Absence of an
out-of-line call at one site is not proof that its constructor was unavailable
or that the element was POD. Do not force emission with fake references or keep
hand-expanded SDK source to imitate one inlining decision.

## Compose the caller's SDK boundaries

A later real-TU comparison at `1cbca7964` separated the individual boundaries
in `CFaderMesh::ApplyInit`. The existing SDK definitions provide the two
four-int `CRect` constructors, three `CRect::OffsetRect(int,int)` adapters,
and `CArray::RemoveAll()`, which delegates to `SetSize(0, -1)`.

| Cumulative source state | Fuzzy | Bytes / instructions | Calls / branches / returns |
| --- | ---: | --- | --- |
| Baseline | 62.55696% | 1351 / 435 | 13 / 38 / 2 |
| Required BDefs include and six Mesh `SQR` uses | 62.55696% | 1351 / 435 | 13 / 38 / 2 |
| Both SDK rectangle constructors | 66.95696% | 1212 / 379 | 13 / 30 / 2 |
| Three SDK offset adapters | 65.207596% | 1251 / 395 | 13 / 32 / 2 |
| SDK `RemoveAll` adapter | 86.741776% | 1270 / 398 | 10 / 30 / 2 |
| Retail | 100% | 1276 / 399 | 10 / 30 / 2 |

The last composition restores the initially missing out-of-line `SetSize`
call, eliminates the extra element-construction calls and extra delete, and
restores the retail `0x9c` frame. Its historical best was 82.3038%, so this is
new headroom, not recovery of an already exact source. The offset-only dip
was a useful base for the next independently supported boundary, not evidence
against the SDK API. No inlining pragma, fabricated statement cost, SDK body
copy, or guessed compiler budget was used.

The two production TUs contain 85 scored bodies. The complete sixteen-site
square substitution is body/reference-flat after the required include control.
That include alone changes allocation in Flat and Shape `RenderFrame`; the
three subsequent SDK steps change only Mesh. Across baseline to final, 82 of
the 85 bodies remain identical. The existing nested Light helpers and their
inline/out-of-line call split are retained.

All 48 ordered references in Mesh, Radial `ApplyInit`, Light `RenderFrame`,
`Render`, and `GetFrameCount` agree with raw retail operands. This includes
independent PE import-directory resolution of `OffsetRect`, and original
payload checks for the named FP constants. Integer products and sums still
precede signed DWORD-to-FP conversion; the four original `CIpow` calls remain.
These are scoped object/reference comparisons, not a whole linked-image proof.

The final first divergence is receiver allocation at `+0xf`. Equal call,
branch and return counts do not prove complete CFG equivalence or exhaust
local/FP lifetime hypotheses; the function is not declared exact or bounded.
Compilation/comparison used `gruntz build base compare`; test suites and the
default gated build are deferred to authorized squash merge.

Reverse-use signature: a real SDK container and element family are already
modeled, but a caller expands the reset while making extra element-constructor
calls. Inspect the caller's complete constructor/member-adapter use layer
before changing the container or forcing an inline decision. The canonical
adoptions and separately recheckable alternatives are the `fader-*` and
`reassess-sqr-fader*` rows in `config/lithtech_lineage.tsv`.
