# Padding and incomplete-layout audit

The 2026-09-07 inventory found **232 candidate fields in 72 headers**. It includes
names containing `pad`, `reserved`, or `unknown`, and hexadecimal `m_p...`
placeholders. These are search candidates, not 232 proven padding members.
The snapshot and per-field decisions are in [padding-inventory.tsv](padding-inventory.tsv).
Offsets and line numbers describe the pre-removal snapshot.

## Results

- **39 automatic-alignment members removed**, across 26 headers. Each isolated
  deletion preserved the owner's size, alignment, and every remaining direct
  field offset under the MSVC-target Clang layout. None had source member
  references in the 282-TU census. A combined proposed-header copy then passed
  **584 size/offset assertions under MSVC 5.0 SP3 `/O2 /MT`**, before applying
  the patch. The same assertions passed against the original headers.
- **3 obsolete fields removed with two obsolete class definitions**:
  `CButeTextBuf` and `CSlotHolder` were abandoned partial layout views.
- **190 candidates retained**: 120 referenced members and 70 unexplained spans. Referenced storage is not padding simply because
  its name says `reserved`. Unreferenced spans whose removal changes the layout
  remain unresolved modeling work; lack of named source uses does not prove the
  retail binary never accesses them.

The isolated probes produced 39 identical layouts, 175 changed layouts, and
18 compilation failures after deleting referenced members. The baseline census
had no parse errors. Probe failures are evidence of live uses, not baseline
header failures. The TSV includes types, offsets, widths, owner sizes/alignment,
neighboring members, reference sites, local packing directives, and disposition.
Packing directives are header-local context; they do not pretend to reconstruct
all inherited preprocessor state. The measured alignment is authoritative for
the current Clang model.

## Specifically questioned members

| Member | Evidence and decision |
|---|---|
| `CDDrawShadeBlit::m_alignmentPadding` | Two bytes at 0x2a before `b32 m_blendVariant` at 0x2c. Natural alignment supplies them. Removed; class remains 0x3c. |
| `CWwdDotObject::m_p18d` | Three tail bytes after `m_dotColor` at 0x18c. Natural class alignment supplies them. Removed; class remains 0x190. |
| `CMulti::m_p5c4` | Four bytes at **0x5c8**, between aligned integers; not automatic alignment. Retained as unexplained storage. The suffix was already misleading when 5e5907e3c renamed the old `m_pad5c8_5cc`. Earlier 57f133453 split an eight-byte hole into the proven sender latch at 0x5c4 and this remaining hole. No semantic type is established. |
| `GameInfo::m_pad10c` | 64 bytes at 0x10c, before the window-class name at 0x14c. Not alignment. Introduced in 937389ff1 with the partial 0x1d4 launch descriptor (then `src/Wap32/Wap32.h`). The complete descriptor is size-checked and copied by `CGameApp::InitInstance`; removing the span changes that contract. It could reflect an incomplete string/field extent, but there is no evidence here selecting a replacement. |

A retail `.text` displacement scan found five direct `+0x5c8` operands: two
player-color dialog reads and three status-bar accesses. Their receivers belong
to other object families, not the `CMulti` gap. The complete decoded `CMulti`
and `CGameApp` families are retained in the audit artifacts. This scan does not
exclude adjusted receivers, indexed accesses, or whole-object copies.

## Modeling leads left visible

- `CPlay::m_pad42a` is **six bytes**, yet the following `ClockInterval` has
  four-byte alignment in the current model. Only two bytes are automatically
  required. `Clock64` explicitly uses `#pragma pack(push, 4)` around an `i64`
  union. This is a concrete packing/type-recovery lead, not proof that globally
  changing that shared type is safe. All clock owners and retail offsets need
  comparison together.
- Packed network records contain explicit gaps under `pack(1)`. Some may be
  artificial substitutes for natural alignment, but others occupy meaningful
  protocol positions (for example the omitted player-preference byte). Changing
  packing or deleting these spans requires the complete packet family and
  serialization extents, not just an owner `sizeof` check.
- Repeated four-byte gaps before doubles and timer objects were often real
  automatic alignment; the proven ones are removed. Repeated larger gaps are
  not evidence of automatic alignment or an invented common base.
- Large retained spans include `CDDrawDeviceManager::m_pad300` (380 bytes),
  `CFaderShape::m_pad78` (1024), `CMulti::m_pad618` (72), and `GameInfo` (64).
  The TSV also lists typed reserved members, including containers, coordinates,
  clocks, and pointers. Those need semantic naming/type recovery, not deletion.

## Eight unused headers

No current source/header/config/tool input includes any of these headers.

| Deleted header | Provenance / replacement |
|---|---|
| `Bute/ButeTail.h` | b615ea149 restored `CCryptMgr`; only a forwarding include remained. |
| `Gruntz/RangeSet.h` | 4bbb9e1aa restored the dprintf family in `Rez/DebugPrintfInternals.h`; only a forwarding include remained. |
| `Rez/DebugConfig.h` | Same dprintf restoration; only a forwarding include remained. |
| `Bute/ButeTextBuf.h` | 88846c017 extracted the inferred padded view. d6cd75bfc replaced its consumer with real `iostream*` and direct stream operations; the unused view survived. |
| `Gruntz/SlotHolder.h` | 133aa0d7f identified `DoSwap` as the `CTileSecretTriggerLogic::Tick` override. 6390135cc later removed the stale source duplicate; the unused header survived. |
| `Gruntz/GruntCombatTimeout.h` | Untracked duplicate of retained `GruntCombatClockInline.h`; retained version also includes the frame-clock declaration. |
| `Gruntz/GruntPickup.h` | Untracked duplicate of `GruntPickupInline.h`, differing only in include guards. |
| `Gruntz/GruntMovement.h` | Untracked predecessor/duplicate of `GruntMovementInline.h`, plus an unused invalid `CGrunt::RecycleCoordHead` definition. The class has no such declaration; its helper call is undeclared in that context. |

Git cannot establish creation provenance for untracked files. Their deletion is
based on content comparison and absence of consumers, not an invented Git
history. Exact copies of all eight are retained in the ignored audit directory.

## Reproduction and limits

Run `nix develop -c python3 scripts/audit-padding.py`. It parses the actual
compilation database, supplements otherwise-unseen candidate headers, and uses
unsaved-header deletion probes. It never edits source. Its raw JSON/TSV outputs
are under `build/audits/padding/`; rerunning describes the current tree, not the
pre-cleanup snapshot committed here. Header histories, the proposed patch,
original/proposed headers, compiler assertions and logs, and the retail scans
are also preserved there for this audit.

The name-based census cannot discover a missing member whose placeholder has
an unrelated semantic-looking name. Clang layout agreement is not original
source proof. VC5 assertions verify the selected owner layouts, while the full
project build checks their actual callers, inheritance consumers, serialization,
and matching gates. This audit does not claim to have recovered the semantic
identity of all 190 retained candidates.

## Final validation

The full `nix develop -c gruntz build` passed, including the MAX gate and all
normal verification gates. All **293 COFF objects** retained identical non-debug
section payloads, section layouts, symbol offsets, and typed relocation targets
after applying the explicit identifier rename map. The final 282-TU naming
census contains 41,345 declarations, zero prefix/capitalization violations, zero
parse errors, zero uncovered files, and no supplemental unused headers.
The capitalization correction covers 216 data identifiers, including the
previous uppercase members/constants and nine asset-suffix names. Functions
remain exempt from the data-prefix rule.
