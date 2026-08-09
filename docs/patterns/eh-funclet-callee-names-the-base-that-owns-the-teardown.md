# The dtor an EH funclet CALLS names the base class that OWNS the teardown

tags: cpp:dtor cpp:eh cpp:class cpp:inline | asm:call asm:mov | topic:identity topic:eh
symptoms: an `RVA_COMPGEN` dtor pin that no obj emits, filed as an inline-budget
wall; a derived destructor in a header that hand-writes the base class's field
resets; two classes in the chain carrying the SAME reset body
confidence: 10/10

A destructor's unwind funclet destroys the **base sub-object**, so the function
it calls is *by construction* the immediate base's destructor. That makes the
funclet's callee an **identity oracle for which class owns a teardown** — a
question that the normal path cannot answer, because there cl inlines the base
dtor and the stores look like they could belong to anybody.

Read it off the relocs, not the code:

```
$ gruntz sema disasm <derived-dtor-rva> --target      # retail
  @0x0d5e83  -> 0x005de0e8   FuncInfo
  ... the unwind map at 0x5de0b8..0x5de0e0 relocates to 0xd5d70

$ llvm-objdump -dr build/objdiff/base/<unit>.obj      # ours
  00000000 <.text$x>:
       4: e8 ...   IMAGE_REL_I386_REL32  ??1CWapObj@@UAE@XZ   <-- ours
```

If the two names differ, the reset body is declared on the wrong class. It is a
**class-ownership bug**, not a budget wall: our funclet calls whatever OUR
immediate base is, so the mismatch is a direct readout of the hierarchy.

## The second oracle: the RTTI base array is complete

`.?AV<Class>@@` is emitted for **every** base of a class that has RTTI, so in a
`/GR` unit an intermediate class cannot hide:

```
$ strings -a $GRUNTZ_EXE | grep -x '.?AVCWapObj@@'     # PRESENT
$ strings -a $GRUNTZ_EXE | grep -x '.?AVCLoadable@@'   # absent
```

`CImage` has RTTI (`.?AVCImage@@` present, `cimagecomdats` is `cpp-rtti`), so
`CImage -> CWapObj -> CObject` with nothing between them is proven. **Caveat:
absence alone proves nothing** — `/GR` is per-project, and `CResolveNode`,
`CDDrawWorker`, `CGameLevel`, `CDDrawChildGroup` are all absent too. The test is
only valid when a class you KNOW has RTTI would have had to name the candidate.

## Evidence

`??1CLoadable@@UAE@XZ` pinned at 0xd5d70 was the tree's last `(unmatched)` row
and was filed as an inline-budget wall (11 objs emitted it, `cimagecomdats` did
not). It was neither.

- retail 0xd5d70, 22 B: `m_id = -1; m_flags = 0; m_ownerCtx = NULL;` + the
  `??_7CObject` stamp
- retail `??1CImage@@UAE@XZ` (0xd5e80)'s unwind map relocates to 0xd5d70 ⇒ that
  IS CImage's immediate-base destructor
- CImage's RTTI base array has CWapObj and no CLoadable

So the reset is `~CWapObj`'s. Our tree had it three times: on `CLoadable`, and
hand-transcribed into `CImage::~CImage()` (an `inline_clones` defect — the copy
is what stopped the base dtor from ever being a callee). Moving the body to
`~CWapObj`, deleting `CLoadable`'s duplicate and deleting CImage's transcription
made `cimagecomdats` emit `??1CWapObj@@UAE@XZ` at 22 bytes, **100.00 EXACT**,
with `??1CImage` unchanged byte-for-byte. `cimagecomdats` 8 -> 9 labelled
functions; `compgen_pins` 376 pins / 0 unemitted; the `(unmatched)` row is gone.

variants: [eh-funclet-band-owns-the-inline-dtor-comdat.md](eh-funclet-band-owns-the-inline-dtor-comdat.md),
[shared-inline-transcribed-once-per-call-site.md](shared-inline-transcribed-once-per-call-site.md),
[inline-budget-emits-ool-comdat.md](inline-budget-emits-ool-comdat.md)
