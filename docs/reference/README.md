# docs/reference — third-party material, mirrored or transcribed

Community documentation about Gruntz and its formats, kept offline so it can be
cited and audited. **Nothing here is evidence.** These are other people's
readings of the same artefacts; a claim only becomes evidence once it has been
checked against `GRUNTZ.EXE`'s disassembly or the shipped bytes.

The project's rule is **distilled over mirrored**: read the distilled layer,
which says which claims survived that check.

| Mirrored / transcribed here | Distilled into |
|---|---|
| [`gooroosgruntz/`](gooroosgruntz/) — GooRoo's Gruntz level-editor docs, the community reference for **game semantics** (what the WWD object fields *mean*) | [`docs/domain/`](../domain/) |
| [`wwd-spec-datashenanigans.md`](wwd-spec-datashenanigans.md) — a third-party **WWD binary-format** specification | [`docs/formats/wwd-v1.md`](../formats/wwd-v1.md) |

## Why the split matters

The two directories answer different questions, and mixing them is how a
plausible reading gets promoted to a fact:

* `docs/reference/` — *what someone else says the format is.*
* `docs/formats/` — *what retail's reader actually does with the bytes*, per
  field, tiered proven / proven-unread / inferred / contradicted.
* `docs/domain/` — *what the values mean to the game*, mapped onto `src/`.

The WWD spec is the cautionary case. `tools/gruntz-codec/src/wwd.rs` and
`include/Wwd/WwdFile.h` both carry field names that match it almost exactly, so
for a while the tree contained three copies of one unverified claim and no
independent check. Verifying it found a field the spec names `signature` that
is really a header *size*, twenty-one header slots the game never reads, and a
checksum formula that reproduced none of the 63 shipped levels until its
operand was corrected.

A third-party spec also cannot tell you what retail **ignores**, and for a
decompilation that is often the more useful half.
