# A UNIFORM funclet frame shift is a local cl refused to overlay - give it a block scope

tags: cpp:local cpp:eh cpp:temporary | asm:sub asm:lea | topic:codegen-idiom
symptoms: `eh_band --census` reports a `frame-offset` group whose displacement histogram
has a SINGLE delta ("UNIFORM +0xN"); `sub esp,N` is larger than retail's by exactly that
delta; every funclet destroys the right object in the right order and only `[ebp+disp]`
moves
confidence: 9/10

## The signal

`python -m gruntz.audit.eh_band --census` splits each unwind funclet into a skeleton, its
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

Ours reserved `sub esp,0x34` with a 16-byte hole between `r` and `bounds`. Wrapping the
pair in braces closed it exactly:

```cpp
{
    RECT r;
    RECT bounds;
    CopyRect(&r, g_gameReg->GetRect(&bounds));
    m_object->m_screenX = r.left + (r.right - r.left) / 2;
    m_object->m_screenY = r.top + (r.bottom - r.top) / 2;
}
```

`sub esp,0x34 -> 0x24`, +4 exact funclets. The same edit on
`CStatusBarMgr::BuildTabzDialog` @0x10a340 (the `RECT src` / `RECT dst` / `CopyRect` head,
with `cx`/`cy` hoisted out of the block because they outlive it) took `sub esp,0x40 ->
0x30` and +12.

## Not this

* **Declaration ORDER is not the lever.** Swapping `RECT bounds; RECT r;` to `RECT r;
  RECT bounds;` is byte-identical - measured on the same function, same `sub esp,0x34`.
  Only the enclosing scope moves cl's slot assignment.
* **Do not brace speculatively.** `CMenuState::LoadGameAssetNamespaces` @0x9fe50 also
  reads UNIFORM (+0x28) but its cause is different - retail spills the `new CChatBox`
  result into the DEAD `areaArg` PARAMETER slot, which no scope reaches - and scoping its
  `RECT rc` cost 95.14 -> 92.02 for zero funclets. Confirm the surplus slot is a real hole
  in the disassembly (a gap between two named locals) before adding braces; if the shift
  is parameter-slot reuse, park it.
* A NEGATIVE uniform delta is the mirror (retail's frame is bigger, i.e. we over-merge or
  are missing a local) and the brace lever does not apply to it.
