# A uniform frame shift is a local cl refused to overlay - give it a block scope

tags: cpp:local cpp:scope cpp:struct cpp:eh cpp:temporary | asm:sub asm:lea | topic:codegen-idiom
symptoms: `sub esp,N` is larger than retail's by one aggregate slot and later escaped
locals occupy distinct homes where retail reuses an earlier dead aggregate; for EH
functions, `eh_band --census` reports the same fact as a `frame-offset` group with one
displacement delta ("UNIFORM +0xN")
confidence: 9/10

## The signal

In an ordinary function, list every address passed for a stack aggregate. If a later
local gets a fresh home while retail reuses an earlier aggregate's home, the earlier
local's lexical scope is too wide. In an EH function,
`gruntz.delink.eh_band --census` makes the same error visible across all unwind
funclets: it splits each funclet into a skeleton, its
`[ebp+disp]` displacements and its relocation targets. A group in the `frame-offset`
bucket already agrees on WHAT is destroyed and in WHAT ORDER - only the frame slots move -
and the census prints `retail - ours` over those displacements. **One delta for the whole
group means one frame-SIZE difference**, and every funclet in the group lands the moment
it is closed. Twelve records in one group, one edit.

```
[eh-band]     12  sbi_tabzdialog_eh:?BuildTabzDialog@CStatusBarMgr@@QAEHXZ
[eh-band]         UNIFORM +0x10 - one frame-SIZE fix
```

Positive delta = retail's slot sits closer to `ebp` = **our frame carries a local retail
does not**. It is usually not a surplus variable; it is a variable cl declined to OVERLAY.

## The cause: cl 5 overlays by SCOPE, not by liveness

cl 5.0 gives every function-scope local its own slot for the whole function. It reuses a
slot only between objects whose SCOPES are disjoint - and a compiler temporary counts as a
scope of its own. So a local that retail's source declared inside a block can share the
slot of a temp that died above it, while the same local at function scope cannot.

`CSingleFrameMessage::CSingleFrameMessage` @0xab310 is the clean calibration. Its base
`CUserLogic(obj, INLINE_BASE)` expands `AttachToObject`, which builds and destroys a
16-byte `zBitVec` temp. Retail then puts `bounds` in exactly that slot:

```asm
; retail: temp and bounds are the SAME 16 bytes at esp+0x24
sub  esp,0x24
lea  ecx,[esp+0x2c]      ; the zBitVec temp (2 pushes live) = esp+0x24
call ??0zBitVec ... call ??1zBitVec
lea  ecx,[esp+0x24]      ; &bounds - the temp is dead, the slot is reused
```

Ours reserved `sub esp,0x34` with a 16-byte hole between `r` and `bounds`.
Wrapping the pair in braces recovered the retail primary-frame reservation:

```cpp
{
    RECT r;
    RECT bounds;
    CopyRect(&r, g_gameReg->GetRect(&bounds));
    m_object->m_screenX = r.left + (r.right - r.left) / 2;
    m_object->m_screenY = r.top + (r.bottom - r.top) / 2;
}
```

`sub esp,0x34 -> 0x24`. It did **not** make every unwind record exact: the
remaining uniform `+0x10` action displacement comes from retail preserving EBX
as a zero carrier while candidate does not, despite both primary bodies now
reserving `0x24`. The same scope mechanism on
`CStatusBarMgr::BuildTabzDialog` @0x10a340 (the `RECT src` / `RECT dst` / `CopyRect` head,
with `cx`/`cy` hoisted out of the block because they outlive it) took `sub esp,0x40 ->
0x30` and +12.

The non-EH control is `CGrunt::ScanNearestTarget` @0xf42f0. Four `Coord` outputs are
simultaneously live while a `RECT` is assembled, but all four are dead before a fifth
`Coord bp` is passed to `GetScreenPos`. With the four construction outputs at function
scope, base reserved `0x44`, put `bp` at `[esp+0x1c..0x20]`, and left the first output's
`[esp+0x38..0x3c]` home unavailable. Retail reserves `0x40` and reuses that first home
for `bp`. Keeping `RECT box` outside while bracing only its four construction outputs
produced the retail allocation:

```cpp
RECT box;
{
    Coord p1;
    // ... four escaped Coord outputs build box ...
}
if (best != NULL) {
    Coord bp; // reuses p1's dead home
    best->GetScreenPos(&bp);
}
```

The controlled A/B changed `sub esp,0x44 -> 0x40`, object size `0x153c -> 0x1538`,
and instruction count `1223 -> 1222`. The fuzzy score moved slightly down because the
whole-function register schedule changed; the exact stack-home identity, not that
navigation metric, proves the scope.

## The overlaid object does not have to be an aggregate

`CGiantRockLogic::BuildRockBreakInGameText` @0x1122a0 **99.96 -> 100.00 EXACT**.
Its whole body was already byte-identical under masking; the only residue was
`sub esp,0x18` against retail's `0x14`. The entity is a four-byte ESCAPED SCALAR:
a `LeafCue*` out-parameter passed by reference to an inlined map lookup at the
very end of the function. At function scope it got a sixth frame word; retail
reuses the dead outer loop counter's home for it. Nesting the tail — which the
common `return 0` already implied — puts it in a scope disjoint from the loops:

```cpp
// NO - `found` is function-scope, so it cannot reuse the loop counter's home
if (sreg->m_emitGate != 0) { return 0; }
LeafCue* found = NULL;
MapLookup(sreg->m_cues, "LEVEL_ROCKBREAK", found);
...
return 0;

// YES - sub esp,0x18 -> 0x14
if (sreg->m_emitGate == 0) {
    LeafCue* found = NULL;
    MapLookup(sreg->m_cues, "LEVEL_ROCKBREAK", found);
    ...
}
return 0;
```

So the sieve is not "look for an aggregate": it is `walls framescan` d > 0, then
**every address that escapes to a call** — `lea <reg>,[esp+N]` for an out-param
counts exactly like a `RECT`.

## Not this

* **Declaration ORDER is not the lever.** Swapping `RECT bounds; RECT r;` to `RECT r;
  RECT bounds;` is byte-identical - measured on the same function, same `sub esp,0x34`.
  Only the enclosing scope moves cl's slot assignment.
* **Do not brace speculatively.** `CMenuState::LoadGameAssetNamespaces` @0x9fe50 also
  read UNIFORM (+0x28). Scoping its `RECT rc` cost 95.14 -> 92.02 for zero funclets because
  the rectangle was not the overlapping entity. The decisive A/B scoped only the earlier
  `LeafCue* e` lookup region: cl then overlaid the construction spill and `e` in the dead
  `areaArg` home, removed the surplus frame word, and closed all four funclets. Declaring
  `e` at function scope instead rotated the same entities into three homes and left a
  UNIFORM +0x4 row. Confirm the exact entities and their disjoint scopes in disassembly;
  a uniform delta identifies the allocation problem, not which pair deserves braces.
* A NEGATIVE uniform delta is the mirror (retail's frame is bigger, i.e. we over-merge or
  are missing a local) and the brace lever does not apply to it.
* **A local already inside a macro's `do { } while (0)` is already scoped.** Bracing
  `CGameLevel::ResolveFloorCollision`'s @0x15ede0 first `PROBE_TILE` result (its only
  function-scope-looking name) was byte-flat at 96.54 - the macro body had given it a
  scope already. Read the escaped addresses out of the disassembly before choosing what
  to brace; a name at the top of the source is not necessarily a name at function scope.
