# Three retail oracles pin a class's layout - and they say our layouts are right
tags: cpp:class cpp:ctor cpp:member cpp:new | asm:push asm:lea asm:mov | topic:identity topic:wall
symptoms: a diff row that looks like a member is at the wrong offset (`lea ebx,[esi+0x18]` vs `lea edi,[esi+0x38]`); "the class must be short"; wanting to move a member to close a diff; a method reading `[this+0x17c]` out of a 0x34-byte class
confidence: 10/10

Retail states class layout out loud in three independent places, and all three are
sweepable across the whole image. Read them BEFORE changing a member offset - a
layout edit is not byte-neutral and a wrong one corrupts every function that
touches the class.

| oracle | what retail writes | audit |
|---|---|---|
| total size | `push <n>; call ??2@YAPAXI@Z` at a `new C` site | `gruntz verify alloc-size` |
| sub-object placement | `lea ecx,[this+N]; call ??0Y` in `??0C`/`??1C`; `mov [this+N],??_7C@@6BY@@@` | `gruntz verify layout --var <name>` |
| field offsets | every `[this+N]` in any `?M@C@@...AE...` body | `gruntz verify data-access --symbol <name>` |

Measured 2026-08-08 over the whole EXE: **430 allocation sites (253 classes), 818
sub-object placements and 11 332 field accesses, with ZERO disagreements** against
`build/gen/structs.json`. Our class layouts are not where the remaining points
are - so a plateau is almost never a layout bug, and "the member must be
elsewhere" is the wrong hypothesis to spend a build on.

Both apparent contradictions the sweep produced along the way were TOOL bugs,
caught before any header moved, and both are worth knowing if you extend it:
fencing the forward scan at the next `new` named an object after whichever BASE
vptr landed first (CWwdGameObjectC read as 0x17c, which is the `AnimWorkerObj`
allocated between it and its own stamp), and dropping that fence without ranking
register-OWNED evidence first let `CGruntzMgr::Run` claim all eleven of its
allocation sizes for one class. Proximity is not ownership; `mov <reg>,eax` is.

## The trap that looks exactly like a layout bug

`--diff` masks addresses, so it aligns two `lea`s that address DIFFERENT
sub-objects and prints them as one changed row:

```asm
-lea ebx,[esi+0x18]     ; base:   CUserBaseLink m_link, from the INLINED CUserLogic ctor
+lea edi,[esi+0x38]     ; target: CMotionState m_motion, after an out-of-line CALL
```

That 0x20 "discrepancy" in `CProjectile::CProjectile` (0xdec60) is not an offset
error: **retail's own bodies name both offsets**. `??0CProjectile@@QAE@XZ`
(0x126e0) constructs `CUserBaseLink` at +0x18, `??1CProjectile@@UAE@XZ`
(0xdef60) destroys the `zBitVec` at +0x18, and 0xdec60 constructs `CMotionState`
at +0x38 - exactly our header. The real divergence is inline-vs-out-of-line on
`CUserLogic::CUserLogic(CGameObject*)`; see
[base-ctor-pinned-out-of-line-costs-every-derived-ctor.md](base-ctor-pinned-out-of-line-costs-every-derived-ctor.md).
`subobject_offsets --class C --all` settles this in one command.

## What the oracles DO find

`this_offsets` PAST-SIZEOF is a misattributed METHOD, not a short class: a body
reading past `sizeof(C)` belongs to a bigger derived class, and it is usually
wearing a `static_cast<CDerived*>(this)` that hides the lie.
`?IsAtSavedScreenPos@CUserLogic@@QAEHXZ` read +0x17c/+0x180 out of a 0x34-byte
CUserLogic - those are `CGrunt::m_lastTilePx`, and every call site already passed
a `CGrunt*`. Rehoming it to CGrunt is a mangled-name change only (reloc-masked,
classed REMOVED not LOST by the MAX gate).

## structs.json blind spots - absence there is not evidence

Three shapes clang's `-fdump-record-layouts` cannot show as fields, so do not
read them as holes: a **vptr** (primary at 0, and each MI base's at its own
offset - take those from the vtable catalog), and a member of an **empty class**
(clang tags both the record and the member `(empty)`; `ghidra_metadata_generate`
dropped both until 2026-08-08, which hid `CButeMgr::m_crypt` at +0x10f).
