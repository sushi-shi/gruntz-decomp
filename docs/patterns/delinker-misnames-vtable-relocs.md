# The delinker mis-names vtable + pooled-string relocs; objdiff scores the NAME

tags: topic:tooling topic:scoring-artifact | asm:mov | cpp:vtable
symptoms: a vtable-heavy TU caps well below its byte-exact potential; `llvm-objdump -r` on the TARGET obj shows dozens of `??_7` relocs collapsed onto one or two wrong symbols
confidence: 9/10 (measured), cause localized to the delinker

## Symptom

In `build/objdiff/target/serialobjectfactory.c.obj` all 77 retail `??_7C<Class>@@6B@`
stores resolve to two wrong names — `??_7?$zDArray@P8CUserLogic@@AEHXZ@@6B@` (x59) and
`?messageMap@CBattlezDlg@@1UAFX_MSGMAP@@B` (x35) — while our base obj emits the correct
per-class names (`??_7CUserBase@@6B@`, `??_7CUserLogic@@6B@`, ...). Same in `gruntzmgr`
(every state vtable -> `??_7CRgn@@6B@`) and in the string pool (`gruntcombat`,
`gruntassetloaders`).

**objdiff scores reloc target NAMES**, so each mis-named reloc is a scored mismatch even
when the address is right. This is a standing free-percent defect across every
vtable-heavy TU, not a reconstruction defect — do not "fix" the source for it.

## What it is NOT (measured 2026-08-07)

The obvious hypothesis — that the catalogued game vtables reach the Model with an
EMPTY size column (`labels.py`: `size = row["size"] if row["kind"] == "template" else None`),
leaving the synth PDB unable to bound the symbol — is **wrong**. Carrying the real catalog
size through (and skipping the cosmetic `vtables` unit in `data_manifest.py` so the rows
still stay out of the manifest, since the COFF emits that section itself) changes the CSV
as intended, re-fires the delink, and leaves the target relocs **bit-for-bit unchanged**
(Overall 87.59% before and after). Reverted.

`??_7CUserLogic@@6B@` and its siblings ARE present in `build/pdb/gruntz_named.yaml`, so the
delinker HAS the correct name and still picks another. The defect is in vostok-delinker's
own address->symbol selection for `.rdata`, not in the data we feed it.

## Next step for whoever picks this up

Instrument or read vostok-delinker's `.rdata` symbol resolution: with several symbols
covering (or adjacent to) one address, which does it choose, and why does a sized template
vtable and an unrelated `messageMap` win over the catalogued entries? Fixing it should be
worth real percent across many TUs at once, with zero source change.

Related: `reloc-typing-vptr-global.md` (objdiff scores names, not addresses),
`objdiff-reloc-scoring.md`.
