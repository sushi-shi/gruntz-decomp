# An out-param reset scheduled BETWEEN argument setup and the call lives INSIDE the helper

tags: cpp:inline cpp:call cpp:local | asm:mov asm:lea asm:call | topic:codegen-idiom
symptoms: a run of repeated `p = NULL; Lookup(map, "KEY", p); if (p) ...` sites where
retail's `mov [esp+N],<zeroreg>` sits AFTER the receiver-chain load (`mov ecx,[reg+0x28]`)
and the `lea`/`push` of `&p`, just before the `call` — while ours emits the store first;
diagnose says REGALLOC with byte-identical size/calls/branches/relocs
confidence: 9/10

## Symptom

`CPlay::SetEffectSpriteDurations` 0xdc060: 32 identical sites of

```cpp
d = NULL;
MapLookup(m_world->m_soundRegistry->m_cues, "GAME_PYRAMIDMOVE", d);
if (d != NULL) { d->m_replayDelay = 100; }
```

`walls diagnose` shows a PERFECT skeleton (0x51b B, 401 insns, 32 calls, 32 branches,
64 relocs on both sides) yet 67% fuzzy. Per site:

```
 ours                                   retail
 mov  [esp+0x10], edi   ; d = NULL      lea  eax, [esp+0xc]      ; &d
 lea  eax, [esp+0x10]                   push <key>
 mov  ecx, [ecx+0x28]                   mov  ecx, [ecx+0x28]     ; receiver chain
 push ...                               mov  [esp+0x18], edi     ; d = NULL - AFTER setup
 call Lookup                            call Lookup
```

Statement-order and TU-state spellings do not move the store: as a CALLER statement it
is scheduled before the call's argument setup in every state.

## Cause

The reset was never a caller statement. cl 5.0's inliner binds the helper's arguments
(the receiver chain, the key, `&out`) BEFORE substituting the body, so a reset that is
the FIRST STATEMENT OF THE INLINE BODY is emitted after the argument computations —
exactly retail's schedule. The dev shape is a tiny lookup wrapper:

```cpp
static inline void LookupCue(CMapStringToPtr& cues, const char* name, LeafCue*& out) {
    out = NULL;               // MFC Lookup does not clear out on failure
    MapLookup(cues, name, out);
}
```

and the 32 sites are `LookupCue(m_world->m_soundRegistry->m_cues, "GAME_...", d);`.
File-local `static inline`, per inline-expansion-boundary-pins-a-neighbour.md (a header
placement ripples 40+ TUs).

## Measured

`CPlay::SetEffectSpriteDurations` 0xdc060: 67.07 -> **98.95** in one change. Residue: a
3-cycle scratch-pair rotation (retail rotates ecx/eax -> edx/ecx -> eax/edx across
consecutive sites, ours pins eax/edx, so every third site matches) — regalloc class,
not reachable from this lever.

### An independent CFG wall can remain after the reset moves (2026-08-21)

`CGameObject::ResolveLinkedObject` 0x151b90 is the `CMapPtrToPtr` out-reference
control. Its wrapper returns the lookup `BOOL`, but owns the typed pointer reset:

```cpp
static inline BOOL LookupLinkedObject(
    CMapPtrToPtr& map,
    i32 id,
    CWwdGameObject*& out
) {
    out = NULL;
    // adapt the integer key and typed out-reference at the MFC boundary
    return map.Lookup(...);
}
```

Moving `out = NULL` across that inline boundary places the zero store at retail's
exact schedule, after the receiver load and both argument computations. It raises
the function from **80.5882 -> 85.8824**. It does not close the function: base and
retail still have the same one call, three branches, one relocation and ordered
referent, but base value-factors the lookup-failure and no-id tails into three
returns while retail keeps four. The remaining first register difference is the
receiver/out-address `ECX`/`EDX` rotation; the missing return is the independently
proven pre-layout over-merge described by
[over-merge-is-decided-before-layout.md](over-merge-is-decided-before-layout.md).

Measured byte-flat controls at 85.8824 were all six helper parameter orders,
key/adapter declaration order, positive versus negative lookup polarity, explicit
inner and outer `else`, and named owner, child-group, or lookup-result locals. The
named-local/explicit-arm forms also moved an unrelated function through TU state,
so they were rejected rather than retained as steering devices. The flat outer
guard does split the return count, but changes retail's block placement and remains
worse at 74.1176 with the helper. Therefore the helper schedule is reusable even
when `diagnose` must continue to classify the whole function as CFG.

### The helper may RETURN the value instead of taking an out-ref (2026-08-17)

The same mechanism fires when the wrapper's out-param is purely internal and the
wrapper returns the typed pointer. `CMapStringToOb::Lookup(LPCTSTR, CObject*&)` has
exactly this shape, because the caller wants a derived pointer, not a `CObject*`:

```cpp
static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* ob = NULL;
    map.Lookup(name, ob);
    return static_cast<CDDrawWorker*>(ob);
}
```

The `CObject* ob = 0` is still the first statement of the inline body, so it is still
emitted after the argument computations. Three `CPlay` sites went **EXACT** on this
change alone, each having been a one-instruction-position diff before it:

| function | rva | before | after |
|---|---|---|---|
| `CPlay::DrawStateMessage` | 0x000cfef0 | 95.1807 | **100.0000** |
| `CPlay::DrawMessageFrame` | 0x000d1650 | 95.6452 | **100.0000** |
| `CPlay::LoadLoadingBarSprite` | 0x000d7440 | 95.8064 | **100.0000** |
| `CWwdGameObjectA::ApplyLookupGeometry` | 0x001505b0 | 93.9394 | **100.0000** |
| `CWwdGameObjectA::ApplyName` | 0x00150540 | 94.1176 | **100.0000** |
| `CDDrawChildGroup::CreateSprite` | 0x001597b0 | 94.1176 | **100.0000** |
| `CMenuState::LoadGameAssetNamespaces` | 0x0009fe50 | 95.2863 | **100.0000** |
| `CTimer::LoadTimerSprite` | 0x0009bb00 | 96.5217 | **100.0000** |
| `CInGameText::Update` | 0x000997c0 | 93.4731 | **96.7066** |

`ApplyLookupGeometry` is the `CMapStringToPtr` counterpart: its wrapper owns a
typed `CAniElement* result = NULL`, calls `MapLookup`, and returns the pointer.
Its size, call set, CFG, and four referents already agreed before the change;
moving the reset across the inline boundary closed the sole scheduling residue.
`ApplyName` and `CreateSprite` were the `CMapStringToOb` single-site controls:
both differed only by `push name` versus the zero store, and both became exact
by returning the typed worker from the wrapper.

`LoadGameAssetNamespaces` is the three-site `LeafCue*` control. Returning a typed
pointer from one file-local helper moved both `MENU_ACTIVATE` resets and the final
`MENU_MENU` reset after argument setup. It also recovered retail's distinct final
temporary slot. The complete 0x343-byte function then matched exactly: 260
instructions, 25 calls, 16 branches, 9 returns, and 44 ordered referents.

`LoadTimerSprite` is the `CMapStringToOb` ABI control. Its caller formerly exposed a
`CObject* spr_ob = 0` solely to satisfy MFC, then cast it to `CDDrawWorker*`. Moving that
real base-class out parameter inside the established typed-return `LookupWorker` helper
and using `CObject* found = NULL` placed the reset after receiver/argument setup and made
the 0x119-byte body exact: 116 instructions, 1 call, 21 branches, 7 returns, and 3 ordered
referents. The base pointer is not a generic-erasure workaround here; `CMapStringToOb`
stores `CObject*` and its retail signature requires `CObject*&`.

`CInGameText::Update` is the bounded `CMapStringToPtr` control. Moving its `LeafCue*`
temporary inside a typed-return helper removes the adapter union's extra lifetime and
restores equal size and instruction count: 168 instructions, 7 calls, 23 branches,
2 returns, and 19 ordered referents on both sides. It does not close the function:
the remaining differences are scratch-register choices plus an independent load/store
schedule at the null tail. Thus a typed-return helper can recover the authentic lifetime
without implying that every surrounding scheduling residue belongs to the lookup.

### It is per-site, not per-idiom — measure before converting a whole family

A fourth site with the identical source idiom, `CPlay::BeginGridWalk` 0x000d0920,
**regressed 97.6191 -> 92.6786** on the same helper - but the reading "its key is a
`const char*` parameter, so there is no `push <literal>` to schedule the store behind"
is WRONG. The variable is the helper's PARAMETER, not the caller's key: retail emits
`lea ecx,[esp+8] / push ecx / mov ecx,[eax+0x10]`, i.e. it takes `&out` BEFORE the
receiver step, which is the signature of a helper that receives the already-live owner
pointer and does the chain inside its own body. `LookupWorker(CDDrawSurfaceMgr* host,
LPCTSTR name)` called as `LookupWorker(m_world, key)` puts it at **100.0000 EXACT**.
Convert sites one at a time and read the per-function score - and when a site regresses,
try the other helper shape before filing it inline (see
[out-param-null-init-belongs-to-an-inline-helper.md](out-param-null-init-belongs-to-an-inline-helper.md)).

## How to spot it

Grep the base/target pair for the zero store's position relative to the receiver-chain
load feeding the same call. Ours-before-setup + retail-inside-setup = the reset belongs
to the callee. The same read generalizes: ANY caller statement retail emits between a
call's argument setup and the call is a candidate first-statement-of-the-inline.

related: inline-expansion-boundary-pins-a-neighbour.md,
subexpression-position-names-its-statement.md, member-not-reread-after-a-call-names-a-source-local.md

### Three more productions (2026-08-22)

| function | rva | before | after | shape |
|---|---|---|---|---|
| `CActionOptionsMenuBar::LoadAssets` | 0x000090e0 | 84.87 | **100.0000** | four `CMapStringToOb` sites, typed-return helper |
| `CGruntzMgr::DelayedQuit` | 0x0008f530 | 84.84 | 94.67 | the `MENU_ACTIVATE` double lookup, typed-return `LookupCue` |
| `CSBI_Image::SerializeFields` | 0x000e6e40 | 84.97 | 89.73 | see below - the helper was INERT, the fix was the index local |

The `SerializeFields` row is the useful negative: retail shows the reset at the
deep position (`lea &out / push / push / mov ecx,[reg+0x28] / mov [esp+N],eax /
call`), yet converting the site to the typed-return helper measured **byte-flat**
and cost the exact sibling `CSBI_Image::Render` 100 -> 74 through the added
declaration. What actually moved it was a different lever in the same block:
`idx` is address-taken by `ar->Read(&idx, ...)`, so every use after the Lookup
call re-read its home slot, while retail loads it into ESI *before* the call's
argument setup. A plain `i32 frameIndex = idx;` copy is the only thing that can
hold a register there. Read the reset position AND the surrounding lifetimes
before assuming the helper is the lever.

Its own residue is a cross-jump decline: retail keeps the two `m_frame = NULL`
arms separate because its `this` is homed in one arm and in ESI in the other, so
the two epilogues are not byte-identical; ours merges them.

### Two more, on the shape parked by out-param-lea-vs-zero-store-slot.md (2026-08-22)

| function | rva | before | after |
|---|---|---|---|
| `CMenuPage::ResolveSubPage` | 0x001833f0 | 90.48 | **100.0000** |
| `CDDrawWorkerHost::RegisterNamed` | 0x00161c50 | 90.48 | **100.0000** |

Both were the residual "one insn" wall: the whole body is `CObject* v = NULL;
map.Lookup(key, v);` and retail's zero store lands AFTER BOTH `Lookup` pushes
while cl puts it between them. `out-param-lea-vs-zero-store-slot.md` records
that family as source-unreachable after four negative controls; that verdict
does NOT extend to the store's position relative to the pushes. Routing the
site through the same typed-return inline helper the sibling TU already used
(`static inline CDDrawWorker* LookupWorker(CMapStringToOb&, LPCTSTR)`) makes
the reset the helper's first statement and both go byte-exact. Detection is
unchanged: our zero store before the last push, retail's after it.
