# Shared inline owners can preserve their sites while moving other TU code

- **Confidence:** 9/10
- **Tags:** `cpp:inline` `cpp:template` `cpp:class` | `asm:call` `asm:jcc` | `topic:codegen-idiom` `topic:tu-state`

The activation registry had 51 separately spelled `FireActivation` overrides,
each with a retail extent of `0x102` bytes. Many used two temporary slot
pointers or nested pointer-to-member syntax. `CActRegPool`'s `ResolveEntry`
mutates registry state and may grow or report failure, so the two lookups in
each retail body are significant.

An inline `DispatchRegisteredAct(Logic*, i32)` first tested
`*ResolveEntry(id)` and then loaded the member-function pointer through a
second `ResolveEntry(id)`. The disposable local-template A/B in
`CToobSpikez::FireActivation` remained byte-exact: both sides were `0x102`
bytes, 77 instructions, eight calls, nine branches, two returns and 30
relocations. Moving that helper to `ActReg.h` and using it at all 51 overrides
left all 52 scored `FireActivation` functions exact. This supports a shared
source boundary; it does not prove whether the original spelling was a free
template or an equivalent macro or member adapter.

The coordinate pool has an analogous free-list operation. A member
`FreeNodePool::Pop()` with a single result variable in `FreeNodePoolInline.h`
replaces 20 expanded sites and preserves the rule that the final node is not
removed. At `CTriggerMgr::RebuildSelectionList` 0x7cc60, the single-exit form
preserved the 89.2241% baseline, including two calls, six branches, one return
and the ordered referents. An early-return spelling introduced a seventh
branch and fell to 79.57%. Some other sites gained a branch under the member
form; those sites retain their retail-matching expanded statements. `Push`
already has both out-of-line and expanded sites, so a shared pool method does
not imply every call site must use it.

The full build after these header and caller changes showed five fresh score
dips in functions whose own source fingerprints stayed unchanged:
`DrawBorder` 100→88.0968, `ExpandNeighbor` 82.2885→82.2016,
`FindPath` 97.4015→97.3561, `AdvanceMotion` 100→99.9535 and
`OpenActionOptionsMenu` 100→96.7910. Their source hashes and historical MAX
remain the audit boundary. These are TU-context effects of this composition,
not evidence that the unchanged function bodies should be rewritten. Keep the
retail-supported shared boundaries, bank the reviewed current state while
preserving historical MAX, and return to those functions through the derived
wall inventory.
