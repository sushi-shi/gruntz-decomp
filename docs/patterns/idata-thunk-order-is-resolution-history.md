# The order INSIDE a DLL's import thunk array is resolution-history noise

**Claim.** In a VC5 (`link.exe` 5.10.7303) image, `.idata`'s layout splits into
two regimes. Everything *structural* is a link-input property and is already
reproduced: descriptor order = library search order (`LINK_LIBS`), array
placement = member-name order with deterministic incremental slack, hint/name
strings = the import lib's `.idata$6` payloads. But the order of entries
*inside* one DLL's ILT/IAT array (and the hint/name pool positions that follow
it) is a deterministic yet **chaotic function of the entire undefined-symbol
resolution history** — not of any input an honest link line can set. Do not
spend link-line or source effort trying to dial it in; it converges exactly
when the reconstruction's symbol-reference structure converges to retail's.

## Mechanism, measured with the pinned linker

`link /VERBOSE` prints the true member pull sequence. Minimal probes (one obj
referencing N imports, one import lib):

* **Pull order is a LIFO scan of the undefined worklist.** A probe calling 16
  `_AIL_*` imports pulls their members in exactly REVERSE call order; reverse
  the calls and the pull order reverses with them (probe1/probe2,
  2026-08-13). So the worklist is insertion-ordered and scanned newest-first.
* **Emitted slot order is NOT the pull order.** With the vendor
  `KERNEL32.LIB`, a 6-import probe emits the array as pull order rotated left
  by one (first-pulled lands last); with our synthesised lib it equals pull
  order exactly; in the full link, per-DLL projections show plain, rot-1,
  reversed and scrambled variants simultaneously (ADVAPI32 exact rot-1,
  smackw32/VERSION/WINSPOOL exact reversal, COMCTL32/DDRAW plain, mss32
  scrambled). A second ordering layer over the global contribution list decides
  it; no per-DLL rule exists.
* **Full-scale pull order is FIFO of first reference.** In the real link the
  16 `_AIL_*` members pull in exactly `gruntzsoundz.obj`'s symbol-table order
  (all 16 references live in that one obj), yet the emitted array matches
  neither that nor its reversal.

## The A/B evidence that bounds it

Three controlled full links, comparing all 456 import slots:

| variation | slot-order effect |
|---|---|
| alphabetical objs → retail-derived `--order` (link_line objlist) | KERNEL32/USER32/GDI32 shuffle *among themselves*; 0 move toward retail (11/456 positions equal before AND after) |
| `--engine-lib` (Dsndmgr/DDrawMgr/... archived as retail did) | **byte-identical** to the non-archived link |
| one extra obj referencing ONE `_AIL_` import early | 12 of mss32's 16 slots scramble; every other DLL byte-identical |
| hint-fix filler exports in the synthesised libs (never referenced) | **byte-identical** — unreferenced members are order-inert |

So the order is insensitive to every honest link-input lever we have, yet
hypersensitive to the reference structure of the objs themselves — which is the
reconstruction, not the link. Retail's own within-DLL order is therefore a
fossil of retail's exact obj symbol tables (plus the devs' incremental-link
history, which a fresh link cannot see at all).

## Safe reverse use

* Retail's within-DLL import order is EVIDENCE about retail's TU-internal
  first-reference order (all of a DLL's referenced imports funnel through the
  worklist in first-mention order at full scale) — usable as a weak oracle once
  the emission layer is decoded, useless as a matching target before that.
* If a future session wants the exact emission rule, RE `link.exe` itself
  (Ghidra; the pinned toolchain binary, 5.10.7303, 464,896 B, image base
  0x400000) — the black-box probes above are the calibration set any decoded
  algorithm must reproduce. Entry points already located (file offsets;
  rva = foff - 0x400 + 0x1000): the `".idata$4"/".idata$5"/".idata$6"` string
  constants at foff 0x17d8/0x17e8/0x1800 are referenced from the member-parsing
  code at foff 0x192eb-0x19bf9 and 0x1c1d5-0x1c7ee (Pass1 recognizes an import
  member's sections instead of copying them - the linker SYNTHESIZES `.idata`
  itself, which is why emission order is not contribution order), and the
  second `".idata$5"` at 0x282c has 11 code refs (0x1f2ca, 0x21b24, 0x2a07b,
  0x2a08f, 0x357d8, 0x3dc7f, 0x3dc89, 0x3fa9b, 0x4210b, 0x5e7ca, 0x62994) -
  the ilink IAT build/emission cluster. The slot-assignment enumeration (and
  the external-symbol hash it walks) is what must be decoded; with it, retail's
  within-DLL order becomes an oracle for retail's per-obj symbol-table order.
* `image_diff`'s `.idata` scorer pairs by `(dll, name)` for exactly this
  reason; with the hint fix and the completed pairing (entry pads with their
  entries, the 3,019 B of zero slack positionally after a skeleton-offset
  proof) the section measures 100.00% — all 15,169 retail bytes, 0 differing —
  while the reorder stays reported in the notes (`docs/image-diff.md`).
