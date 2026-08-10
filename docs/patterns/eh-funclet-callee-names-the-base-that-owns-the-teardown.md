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

## The whole intermediate class was the defect (2026-08-10)

The 2026-08-08 fix moved the *body*; it left the intermediate `CLoadable` standing, so
`eh_band --census` still read **31 groups** of `??1CLoadable@@UAE@XZ` (ours) against
`??1CWapObj@@UAE@XZ` (retail) - one for every derived class in `include/DDrawMgr/**`.
The census count is the tell: a body-placement mistake shows up once, a class that does
not exist shows up once per subclass.

Three readings converge on ONE class where we modelled two:

* **the deleting destructor.** `??_GCLoadable@@UAEPAXI@Z` (0x155720) is `push esi / mov
  esi,ecx / call 0x429b` and 0x429b is a five-byte ILT `jmp 0xd5d70`. A `??_G` calls its
  OWN class's destructor, so the class at that vtable HAS no teardown between it and
  0xd5d70 - it *is* 0xd5d70's class. (Its own vptr store is dead-eliminated by the three
  field stores that follow.)
* **the vtable.** `??_7CLoadable@@6B@` at 0x1efc30 is `0x24` = **9 slots**: CObject's five
  plus `IsLoaded / IsReady / Unload / GetClassId`. That is exactly the union of what we
  had split across an abstract `CWapObj` (IsLoaded, IsReady) and a concrete `CLoadable`
  (Unload, GetClassId) - and slot 6 already resolved to `?IsReady@CWapObj@@UAEHXZ`, i.e.
  the "two" classes were already sharing one vtable.
* **RTTI.** `.?AVCWapObj@@` is present and `.?AVCLoadable@@` is not, and `CImage`'s base
  array is `CImage -> CWapObj -> CObject`. `CImage`'s slot 7 (which we had named
  `FreeAll`) is the same slot as `CLoadable::Unload`, which is only possible if one class
  declares it.

Merging `CLoadable` into `CWapObj` (delete `include/Gruntz/Loadable.h`, move
`LoadableClassId` + `Unload` + `GetClassId` + the ctors onto `CWapObj`, rename
`CImage::FreeAll` -> `CImage::Unload OVERRIDE`, rebind `??_7CLoadable@@6B@` ->
`??_7CWapObj@@6B@`) took the funclet band **2706 -> 2737 exact** in one build, census
`different-targets` 54 -> 29 and `identical` 86.4% -> 89.7%, with every renamed pin still
100.00 and `vtable_hierarchy --audit` / `vtable_owner --audit` / `class_sizes` clean.
`VTBL_ABSENT(CWapObj)` was removed - the class does have a `??_7`, we had just hung it on
the wrong name.

variants: [eh-funclet-band-owns-the-inline-dtor-comdat.md](eh-funclet-band-owns-the-inline-dtor-comdat.md),
[shared-inline-transcribed-once-per-call-site.md](shared-inline-transcribed-once-per-call-site.md),
[inline-budget-emits-ool-comdat.md](inline-budget-emits-ool-comdat.md)
