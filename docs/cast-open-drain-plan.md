# Cast OPEN-drain plan — the 528 remaining, by root cause

Successor to `docs/cast-dissolution-plan.md` (whose WS-A..WS-K workstreams are spent:
the big multi-site folds — ActReg wrappers, geometry twins, container elements, manager
singletons — are done). This plan covers what is left and is sized on **root causes, not
sites**.

Measure: `python -m gruntz.audit.cast_ledger` (worklist), `--summary` (buckets),
`--max N` (ratchet). Policy: `docs/cast-metric-policy.md`.

## State (2026-07-27)

    909 reinterpret_cast total   381 accounted for   528 OPEN
    accounted =  177 explained-seam | 110 win32-abi | 42 mfc-position
               |  35 i64-halves-pun |  17 mfc-voidref-out

**OPEN is the metric that must reach 0 — the total will not.** As OPEN falls the FORCED
side grows; a named forced cast is finished work. The ledger's own rule: *an unexplained
cast is indistinguishable from un-started work.*

### The tail has arrived

528 OPEN sites sit on **339 distinct (file, operand) root causes**: 266 singletons,
74 multi-site, largest cluster 4. There is no 44-site win left. 34 OPEN are in headers,
494 in .cpp.

### By shape

| n | shape | disposition |
|---|---|---|
| 164 | pointer-to-class target | mostly CONVERT (identity/retype) |
| 141 | blob/scalar cursor (`i32*`/`u8*`/`char*` over a typed thing) | mostly REASON (format/width forced) |
|  91 | pointer → int (43 call args, 37 locals, 11 return slots) | mixed |
|  33 | other pointer | per-site |
|  27 | view-of-member (`(T*)&x`) | mixed: real union vs phantom view |
|  27 | struct → scalar cursor (`(i32*)&m_field`) | mixed |
|  18 | cast-to-value (non-pointer, non-int) | per-site |
|  16 | **offset-cast** | CONVERT — banned outright |
|  11 | MFC/list element | REASON or seam |

Within the pointer-to-class family, roughly half cast a **named variable** (fix = retype
that one declaration) and half cast an **expression** (fix = type the producer/callee).

---

## Tier 1 — must go, no judgement required (~20)

Hard-banned by CLAUDE.md (`(char*)x + N` is banned even as a C++ cast) or provably
redundant. Fully enumerated, so this tier is finishable in one pass.

The 16 offset-casts:

    ImageSet1.cpp:78 / ImageSet2.cpp:38 / ImageSet3.cpp:39   (char*)record + 8   <- ONE
        shared record struct kills all three; identical shape in three TUs
    ImagePool.cpp:845,879     (char*)buf + 8, hdr + 1  ("pixel stream at buf + 0x20")
    ImageOwned.cpp:232,234,235  ((u8*)src + m_rleLen)[i + 0x1e..0x20]  — palette tail
    Play.cpp:934,1190         (i32*)(nameBuf + 0x20)   — scratch tail of a local buffer
    WwdGameObject.cpp:467,468 (i32*)(src + 0x10 / + 0x08)  — a notify-fn + arg pair
    WwdFactoryObject.cpp:425  (i32*)((char*)&src->m_records.ElementAt(0) + N)
    DDrawSubMgr.cpp:672,677   (char**)(mgr + 0x20), (i32*)(cue + 0x78)
    NetMgr.cpp:925            (i32*)(desc + 0x18)
    NetCmdSlot.cpp:288        (char*)rec - payload + 0x410
    DirectSoundMgr.cpp:728    (char*)src + n1   (runtime offset — may be format-forced)

Method: the offset resolves to a named member of an already-typed object (today: +0xf8
was `m_0f0.GetSize()`, +0x1d0 was an embedded `SaveSlot`, +8 was `CPtrList::CNode::data`),
or to a packed on-disk record that gets a declared struct (today: `WwdTileDescTable`).
When the target is a file image, the *base* cast stays as byte-forced and only the
member reads convert.

Also in this tier: a mechanical sweep for **no-op casts** — a cast whose operand already
has the target type. Found 3 today by accident (`(CGrunt*)m_grid[i]` where `m_grid` is
`CGrunt*[]`). Worth one scripted pass over all 909.

## Tier 2 — structural retypes with multi-site payoff (74 sites)

One declaration fixed kills 2–4 casts. Ordered by cluster size:

    4  g_slotState            src/Io/SaveGame.cpp
    4  desc                   src/Image/CImage.cpp
    4  resolved               src/Gruntz/GruntVoice.cpp
    4  header                 src/DDrawMgr/DDrawSurfaceMgr.cpp
    3  obj->GetSlot           src/Io/SaveGame.cpp
    3  src                    src/Image/ImageOwned.cpp
    3  msg->m_8               src/Gruntz/Multi.cpp
    3  blk                    src/Crypto/BitStreamBlowfish.cpp
    3  s_empty                src/Bute/ButeMgr.cpp
    3  this                   src/Gruntz/GruntEntranceArrival.cpp
    3  m_cells                src/Gruntz/Grunt.cpp        (@identity-TODO — Tier 4)
    3  GetProcAddress         src/Gruntz/HeapDiag.cpp     (API-forced — Tier 3)
    + 62 two-site clusters

Cross-file shapes worth doing as one change rather than per-file:

- **`m_ownerCtx`** (DDrawSubMgr.cpp, DDrawWorker.h, Loadable.h) — the +0x0c owner-context
  handle, still untyped; `CLoadable`'s own comment already says it IS the
  `CDDrawSurfaceMgr` across the draw family. Typing it is a documented shape, and it is
  the last of the "untyped manager handle" family.
- **11 `return slot typed i32`** — the `CLoadable::Unload` / `DDrawSubMgrLeaf` residue
  carriers (`return reinterpret_cast<i32>(val)`). Memory `unload-scheme-void-slot-proof`
  says the bare-`c3` slot bodies PROVE void and these eax-residue returns are walls to
  dissolve. Needs the slot-signature method (C2561/C2555 probe), not a cast edit.
- `GetProcAddress` (HeapDiag, SFSelectDevice, WinAPIModule) — `FARPROC` → typed fn ptr.
  API-forced by Win32; three reasons, no conversion.

## Tier 3 — per-site verification, usually ends in a reason (~300)

The bulk. Each site needs its evidence read once, then either converts or gets a
ledger-visible reason. Do NOT blanket-annotate: a regex matched 152 "format walk"
candidates and hand-checking FileImage's 14 showed the shape is only right when the
format actually forces it.

- **141 blob/scalar cursors** — palette blobs (0x400 copied dword-wise), pixel rows
  (byte pitch vs 16bpp spans), file images, wire records. Verify width/format, then
  `byte-forced`. A minority hide a real record struct and convert.
- **43 call-arg pointer→int** — callback and factory payload words. **Check the callee's
  non-casting callers first.** This trap fired 10 times today: `InvokeCallback`,
  `CParseSource::Build`, `CreateA`, `SpawnVoiceDriver`, `CMoviePlayer::OpenHi` all looked
  unanimous among their casting call sites and were refuted by one caller passing a real
  pointer or a genuine integer.
- **37 local pointer→int** — often removable outright (a pointer round-tripped through an
  `i32` local, as LightFx did today).
- **27 view-of-member + 27 struct→scalar-cursor** — split real unions/overlays (keep,
  `faithful`/`overlay`) from phantom views (dissolve per
  `phantom-view-dissolution-recipe`).
- **18 cast-to-value, 11 MFC element, 33 other** — MFC element casts want a typed
  accessor at the owning class (today: `HeadRec()`, `HeadSlotNode()`, `SlotOf()`).
- 2 sentinel `(CDDrawSurfaceMgr*)0xffff` — a sentinel value, not a pointer. Reason only.

Largest OPEN files to sweep in order: NetCmdSlot(7+), ActionArea(8), GruntCombat(10),
GruntSpawnConfig(10), GruntVoice(10), TriggerMgr(10), ImagePool(10), RezSync(10),
LevelPlane(9), BattlezMapConfig(9), GruntEntranceArrival(9), CImage(9), NetMgr(9),
DDSurface(8), DDrawSubMgr(8), Projectile(8), DebugPrintf(8), WwdFactoryObject(8),
ButeMgr(7), TypeCollRuntime.h(6).

## Tier 4 — blocked on identity; must NOT be guessed

Each keeps its `@identity-TODO` with the evidence that would settle it. These are the
honest floor of the campaign, not laziness — `our-guesses-cite-themselves-as-evidence`.

- `MakeB2` → `AllocBufMakeB2` → `CreateWorker2C` chain: `CDDPalette::LoadFromFile`
  provably takes a path (`strrchr(filename,'.')` + ext dispatch), but `CreateWorker2C`
  carries a *separate* `const char* key`. Needs 0x165a10 / 0x142f40 disasm.
- `Createa58_3` → `LoadByExt`: desc-vs-path.
- `Grunt::m_cells` → the real `_zdvec::IndexToPtr`.
- `ResolveImage_163ee0`: a `CParseSource*` reaching `strrchr`.
- `CAniAdvanceCursor::SelectCue`: `m_ownerCtx + 0x20`.
- RezSync's two cross-class casts (`CSpriteRefTable*`→`CTriggerMgr*`,
  `CShadeTableCache*`→`CDDrawSurfaceMgr*`).
- Warlord's 6 ILT-thunk PMF handlers (free functions cannot enter the PMF table).

## Matcher lanes — keep 4 in flight

T4 items are blocked on a function nobody has reconstructed, or on an identity nobody has
proven. Those are matcher work, not cast work: reconstruct the function and the casts fall
out. Run **four matchers continuously**, each in a REUSED worktree
(`.claude/worktrees/matcher-N`, `classifier-1`) — never `isolation: worktree`, which mints
a cold one without `build/`, the wine prefix or the Ghidra DB.

In flight (2026-07-27): PickWeighted 0x11bee0 (+ its signature) | tree-wide inlined-MFC-
accessor sweep | CGameObject::Setup 0x150d60 + SelectCue 0x157a80 owner identities |
CDDrawPtrCollections factory signatures 0x142f40/0x142560/0x142260.

**Refill queue** — when a lane frees, dispatch the next one immediately (cherry-pick first,
then re-dispatch; the verify build gates the BLESS, not the refill):

1. `_zdvec::IndexToPtr` 0x310f0 — unblocks Grunt's `m_cells` x3 and the TypeCollRuntime
   accessor family; a leaf, so high value per byte.
2. The 11 `return reinterpret_cast<i32>(ptr)` residue carriers (`CLoadable::Unload` and the
   DDrawSubMgrLeaf slots) — settle the slot signature via the bare-`c3` void proof
   (memory: `unload-scheme-void-slot-proof`), not by editing casts.
3. `CMoviePlayer::OpenHi` — the Smacker first argument is dual-use (a HANDLE from
   `CFecFile::Lookup` at one caller); prove which.
4. RezSync's two cross-class casts (`CSpriteRefTable*`->`CTriggerMgr*`,
   `CShadeTableCache*`->`CDDrawSurfaceMgr*`) — per the no-cross-casts rule both are
   mis-models; find the real relation.
5. Warlord's 6 ILT-thunk PMF handlers — unresolved thunk targets modelled as free
   functions, which is why they can only enter the PMF table through a raw slot write.

## Method (learned this session, applies to every tier)

1. **Fix the declaration, not the cast.** The cast is the symptom.
2. **Check non-casting callers before retyping a parameter.** 10/10 refutations came
   from a caller the destination-resolver could not see.
3. **After annotating, re-run the ledger and confirm the count moved.** A reason outside
   the closed REASON vocabulary reads as done and counts as OPEN — worse than nothing.
   Vocabulary: `language-forced|API-forced|forced by|byte-forced|byte-evidenced|no reloc|
   bare imm|one seam|at one seam|the pun|overlay|faithful|PROVEN|proven|@identity-TODO`.
4. **Gate on BUILD, not %.** MAX fuzzy banks best-ever; a proven structural fix is kept
   even when its own function dips (QueryGruntSlots 94→74 today, kept, with the codegen
   bias recorded).
5. **A seam lives in the narrowest scope its users allow** — file-static first, the
   owning class's header only if genuinely cross-TU (`seams-stay-local`). Only 34 OPEN
   are in headers; batch header edits, they ripple.
6. **Nothing between an `RVA()` marker and its function** — a struct inserted there
   orphans the label and trips the ratchet.
