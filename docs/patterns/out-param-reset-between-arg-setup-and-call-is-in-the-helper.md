# An out-param reset scheduled BETWEEN argument setup and the call lives INSIDE the helper

tags: cpp:inline cpp:call cpp:local | asm:mov asm:lea asm:call | topic:codegen-idiom
symptoms: a run of repeated `p = NULL; Lookup(map, "KEY", p); if (p) ...` sites where
retail's `mov [esp+N],<zeroreg>` sits AFTER the receiver-chain load (`mov ecx,[reg+0x28]`)
and the `lea`/`push` of `&p`, just before the `call` — while ours emits the store first;
diagnose says REGALLOC with byte-identical size/calls/branches/relocs
confidence: 8/10

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

### The helper may RETURN the value instead of taking an out-ref (2026-08-17)

The same mechanism fires when the wrapper's out-param is purely internal and the
wrapper returns the typed pointer. `CMapStringToOb::Lookup(LPCTSTR, CObject*&)` has
exactly this shape, because the caller wants a derived pointer, not a `CObject*`:

```cpp
static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* ob = 0;
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

`ApplyLookupGeometry` is the `CMapStringToPtr` counterpart: its wrapper owns a
typed `CAniElement* result = NULL`, calls `MapLookup`, and returns the pointer.
Its size, call set, CFG, and four referents already agreed before the change;
moving the reset across the inline boundary closed the sole scheduling residue.

### It is per-site, not per-idiom — measure before converting a whole family

A fourth site with the identical source idiom, `CPlay::BeginGridWalk` 0x000d0920,
**regressed 97.6191 -> 92.6786** on the same helper and was left inline. Its key is a
`const char*` PARAMETER rather than a pooled literal, so there is no `push <literal>`
to schedule the store behind; retail instead emits `lea ecx,[esp+8] / push ecx / mov
ecx,[eax+0x10]` and the helper form reorders that lea/push pair. Convert sites one at a
time and read the per-function score, not the family.

## How to spot it

Grep the base/target pair for the zero store's position relative to the receiver-chain
load feeding the same call. Ours-before-setup + retail-inside-setup = the reset belongs
to the callee. The same read generalizes: ANY caller statement retail emits between a
call's argument setup and the call is a candidate first-statement-of-the-inline.

related: inline-expansion-boundary-pins-a-neighbour.md,
subexpression-position-names-its-statement.md, member-not-reread-after-a-call-names-a-source-local.md
