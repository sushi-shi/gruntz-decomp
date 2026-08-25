# The archive-slot I/O sieve: prove a file FORMAT faithful without running anything

**Symptom it answers.** "Loading a retail-written save crashes / saving fails" — and the
fault lands in a function that is already byte-exact, so no per-function % points anywhere.
The first question is always *is our reader consuming exactly the bytes retail's reader
consumes?* Until that is settled every other hypothesis is speculation.

## The sieve

`CFileMemBase`'s vtable puts `Read(void*,int)` at **+0x2c** and `Write(const void*,int)` at
**+0x30**. Every serialised field in the game therefore lowers to

```asm
push <size>          ; the byte count      <- the FORMAT
push <buf>           ; lea / imm / reg
mov  ecx, <archive>
call DWORD PTR [reg+0x2c]      ; or +0x30
```

So for each function present in both `build/objdiff/base/<u>.obj` and
`build/objdiff/target/<u>.c.obj`, extract the **ordered sequence of `(RD|WR, size)`** —
the size is the push **two** positions before the call, not the last one (the buffer
address is pushed last, and when the buffer is a global that push is also an immediate) —
and diff the sequences. Ours-vs-retail, same function name, per unit.

**Two parsing traps, both fatal if missed:**

1. `llvm-objdump` prints cl's internal labels (`$L29876`, `$tail$29882`) as if they were
   symbols, which silently re-attributes the calls inside a switch arm away from the
   enclosing function. Skip any symbol name starting with `$`.
2. A size that arrives in a register (`RD:eax`) is not a format fact — those rows are
   noise, and most of them are `+0x2c` on some *other* class's vtable, not the archive.

## The measured result (2026-08-10)

**256 functions carry archive-slot calls; the sequences are identical to retail in every
one of them.** The five printed rows were all register-sized calls on non-archive vtables.
The save-file field protocol is therefore byte-faithful end to end — reader and writer —
and a format desync is *excluded* as the cause of a save/restore defect.

Companion checks that close the rest of the format surface, all also clean:

* **`CSnapshotHeader`** — retail's `SnapshotChildren` (0x156020) stores `g_wwdObjIdCounter`
  at `header+0x114` and `CountActive()` at `header+0x110`, then `push 0x120` to `Write`;
  `RestoreChildren` (0x156530) reads the same 0x120. Layout confirmed field-by-field.
* **`WwdSnapshot`** — retail's `LoadObjects` (0x15ad30) reads `push 0xa0`, then uses
  `desc+4` as the id key, `desc+8` as the class id (`add eax,-5; cmp eax,0x17` + jump
  table), `desc+0x14` as the worker name and `desc+0` as `m_id`. Layout confirmed.
* **The factory type-id map** — `GameSerializationCallback`'s arm-9 switch is
  `lea eax,[ecx-0x3e8]; cmp eax,0x44; ja default; jmp [eax*4+0x40eb10]`. Read the 69
  dwords at RVA 0xeb10, take each arm's **last** `mov [esi],<imm>` before its `mov eax,1`
  epilogue, and look the address up in `config/retail/data_vtables.tsv`: the vptr NAMES
  the class the arm constructs. All 69 arms agree with our `case LOGIC_*: new C*;` list,
  including the two ids (0x3f9, 0x40e) whose table entries point at the default arm and
  which our source correctly omits. Two arms (0x42b, 0x42c) `call` an out-of-line ctor
  instead of inlining it, so scan for the `call` when no vptr store is found.

## Tools

    the archive-slot I/O sieve (retired)          # the sieve above
    the LogicTypeId factory map (retired)        # the 69-arm LogicTypeId -> class map
    the savegame decoder (retired) <game-dir>   # decode the shipped .sav files

## READ THE SHIPPED FILES FIRST — they refute inferences the code cannot

Everything above is about *our* bytes. The **saves on disk** are independent evidence and
they overturned two conclusions that pure code reading had reached:

* **"SnapshotChildren returned 0" was wrong.** Each `SlotN.sav` ends with a *complete*
  `0x3843a`-byte preview BMP (320x240x24, `bfSize` = 0x3843a = `SAVE_PREVIEW_BYTES`,
  `bfOffBits` = 0x3a), appended by `SaveOverlayBufferShot` -> `SaveScreenshot` ->
  `CDDSurface::SaveFile`
  *after* `SaveGame` has already written the snapshot. A complete preview therefore proves the
  whole `CSaveGame::Save` chain ran to the end and **that save succeeded**. An "ERROR - Cannot
  Save Game" alongside complete files means the error came from a *different* attempt.
* **"the slot record was never marked used" was wrong.** `Gruntz.sav` decodes to a valid index:
  `m_header[2]` equals the `ComputeAll()` sum over the de-obfuscated slots, and the used slots
  carry the right `m_type`, name and `SlotN.sav` path. `TempFileExists` had everything it needed.

The decoder also locates the `CTriggerMgr` grid record by its own invariant — 60 object ids
followed by `m_unitCountByPlayer[4]` where `rowCount[r]` equals the number of non-zero ids in row `r`
(the neighbouring offsets pass a weaker sum-only test, so check per row) — and resolves each id
through the `WwdSnapshot` table. That turns a register dump into a named cell: the
`RemovePlayerUnitsImmediately` fault at row 1 col 0 resolves to a `Grunt` record with `logicTypeId` 0x3e8 that
IS present in the object table, which rules out type confusion and leaves only "created but
never chained".

## Why it matters even when it finds nothing

A clean sieve converts "the save subsystem is broken somewhere" into "the bytes are right;
the failure is a runtime-state guard". On the restore path those guards are few and each is
a *lookup* predicate, not a format one — `LoadObjects` (worker name not in the cache,
duplicate object id, `m_logicRecord == NULL`, factory declined), `Deserialize` (id not in
`m_map48`), `CTriggerMgr::Load` (grid id not in `m_map48`, or the object has no logic).

### Worked narrowing: from a register dump to three named guards

The `RemovePlayerUnitsImmediately` fault register set (`eax=0`, `ecx`, `esi`, `edi`) reconstructs the loop
induction exactly — `edi = row*15 + 0x47`, `esi = this + 0x20c + 4*row`, `ecx = &m_units[row*15
+ col]` — giving **row 1, col 0**. `savegame_dump` resolves that cell to object id 178, and the
table order (which is the `LoadObjects`/`Deserialize` order, since `WriteObjectSnapshots` and
`SerializeObjects` walk the same `m_map48`) puts the five grid grunts at indices 132..136.

Row 0 completed before the fault, so ids 185 (idx 132) and 181 (idx 133) HAD a valid
`m_wwdObject`; id 178 (idx 136) did not. `CWapX::SerializeAnimationState` runs *before* `LoadStateRecord`, so the
abort is at index 133..136 and after 181's Chain.

Then walk the per-object `SERIAL_LOAD` path and keep only the guards that can return 0 for a
reason other than `ar == NULL`:

    CWwdSpriteObject::SerializeDispatch CAniAdvanceCursor::SerializeDispatch/Deserialize -- ar only
                               ReadSpriteState                   -- ar only
    CGameObject::SerializeDispatch SerializeObjectState              -- THREE real guards
                               m_logicRecord == NULL
    CLogicRecord::SerializeDispatch    Load                                  -- ar only
    CGrunt::SerializeDispatch      CUserLogic::SerializeDispatch / CWapX::SerializeAnimationState -- ar only
                               LoadStateRecord -> SERIALREF x7       -- checked, all resolve

The seven `SERIALREF` sprite ids were read straight out of the file for all five grunts (anchor:
the block sits at key+0x6ce, followed by the three `READCSTR` name fields
`NORMALGRUNT`/``/`GRUNTZ_NORMALGRUNT_DEATH`) and every non-zero id resolves to a real table
entry, so `LoadStateRecord` is excluded.

**What survives is `CGameObject::SerializeObjectState`'s `EnsureHitLogic` /
`EnsureAttackLogic` / `EnsureBumpLogic`** — the three `LogicHit`/`LogicAttack`/`LogicBump`
name resolutions — and they run *before* `CWapX::SerializeAnimationState`, which is precisely why the victim's
`m_wwdObject` is still the zero `operator new` left. Each `Ensure*Worker` returns 0 exactly
when its `m_workerCache->m_workers.Lookup(name, found)` missed, so the question is whether
those three workers exist in the CURRENT world at restore time.

**REFUTED, and recorded so nobody re-derives it:** "our source wrongly restores
`g_logicTypesRegistered`" is wrong. Retail's `CUserLogic::SerializeDispatch` @0x16e7f0 reads it
too — `push 0x4; push 0x6bf674; call [edx+0x2c]`, between `m_reserved2c` and
`m_previousAnimationActId` — and RVA 0x2bf674 is exactly our `DATA(0x002bf674) i32
g_logicTypesRegistered` (`src/Wwd/WwdGameObject.cpp:45`). Same global, same position, same
width. The restore-time value of that flag is retail's own behaviour.

**The real asymmetry is the SCOPE MISMATCH, and it is structural.** `sema xref` on
`BuildLogicTypeTable` (0x8a40) gives ~55 retail callers and they are ALL
`CFoo::CFoo(CGameObject*)` ctors reached from the `_DispatchXxxLogic` functions — i.e. the
single `RegisterLogicTypesOnce()` inside `USERLOGIC_ATTACH_TO_OBJECT`. Our tree has that same
one call site and every one of those ctors, so **no caller is missing**. But the save-restore
path does not use those ctors at all: `GameSerializationCallback`'s arm 9 builds logics with the
DEFAULT ctors (`new CGrunt()` -> `CMovingLogic(CUserLogic::INLINE_BASE)` ->
`CUserLogic(EInlineBase) {}`, an empty body), which never run the macro and therefore never
call `BuildLogicTypeTable`. A restore inherits those three workers from the preceding level
load or does without them.

And the two things it depends on have different lifetimes: `g_logicTypesRegistered` is a
**process-global latch that is never reset**, while the workers it guards live in the
**per-world** `obj->OwnerMgr()->m_logicRegistry->m_templatesByName` — the same map
`SerializeObjectState` looks them up in. Any path that builds a fresh `CDDrawSurfaceMgr` (or
clears its worker cache) after the latch is set leaves the three workers permanently absent.
`CMD_LOAD_SAVED_GAME` calls `PassClickToPlayState(si->m_levelId, 0, 1)` *before*
`RestoreGameFromFile`, so that ordering is what to audit next — not the flag, and not the factory
table (`RegisterGameObjectLogicTypes` @0xa3b0 is 100.00% EXACT, and the three callback
dispatchers are not in it anyway; their templates are registered lazily only by
`BuildLogicTypeTable`).

And it explains the crash *shape*: `CTriggerMgr::Load` fills `m_units` from object ids during
`SerializeGameState(SERIAL_LOAD)`, which `RestoreChildren` invokes **before**
`m_childGroup->Deserialize`, and it is `Deserialize` that finally runs `CWapX::SerializeAnimationState` and
assigns `m_wwdObject`. So between those two calls the grid legitimately holds grunts whose
`m_wwdObject` is still the zero left by `operator new` — retail's own `CGrunt()` writes
nothing in the 0x150 band either (its inlined copy in `GameSerializationCallback` stores 0x268..0x8cc
only). If anything in that window returns 0, `CGruntzMgr`'s `CMD_LOAD_SAVED_GAME` arm merely
`ReportError`s and leaves the half-built grid in place, and the next
`CPlay::FreeListTeardown` / `LeaveState` faults in `RemovePlayerUnitsImmediately+0x5d` on
`c->m_wwdObject->m_flags |= 0x10000`. **The fault site is the failure path, not the bug** —
adding a NULL guard there would hide it and move away from retail, which has no such guard.
