# 16-bit color pack via runtime RGB shift globals (`(c>>sar)<<off` per channel)
tags: cpp:bitop | asm:sar asm:shl | topic:codegen-idiom topic:wall
symptoms: `mov ecx,DAT_00683eac; sar ebp,cl; mov ecx,DAT_00683ea0; shl ebp,cl; or ...`, screen-format globals 0x683ea0..0x683eb4, packed 565/555 color into a u16
confidence: 7/10

The renderer packs an 8-bit (R,G,B) constant triple into the screen's native
16-bit pixel using six runtime globals at `0x683ea0..0x683eb4` (the live
display-format shift table): per channel, right-shift the 8-bit value down to the
channel's bit width, then left-shift it into the channel's position, then OR the
three channels. The shift counts are *variable* (held in globals), so each term
is `mov ecx,<global>; sar reg,cl` / `mov ecx,<global>; shl reg,cl`. The globals:
`0x683eac`=R down-shift, `0x683ea0`=R up-shift; `0x683eb0`=G down, `0x683ea4`=G up;
`0x683eb4`=B down (B has no up-shift — it sits at bit 0).

```cpp
extern i32 g_rDown, g_rUp, g_gDown, g_gUp, g_bDown; // 0x683eac/ea0/eb0/ea4/eb4
// each channel: (8-bit const >> downShift) << upShift ; B has no upShift
static inline u16 Pack(i32 r, i32 g, i32 b) {
    return (u16)(((r >> g_rDown) << g_rUp) | ((g >> g_gDown) << g_gUp) | (b >> g_bDown));
}
```
```asm
mov ebp,0x4f ; mov ecx,ds:0x683eac ; sar ebp,cl ; mov ecx,ds:0x683ea0 ; shl ebp,cl   ; R
mov ebp,0x14 ; mov ecx,ds:0x683eb0 ; sar ebp,cl ; mov ecx,ds:0x683ea4 ; shl ebp,cl   ; G
or  edi,ebp                                                                            ; R|G (u16 temp)
mov ebp,0x01 ; mov ecx,ds:0x683eb4 ; sar ebp,cl ; or ecx,ebp                           ; |B
```
**Per-channel term is steerable** (the `(c>>down)<<up` spelling reproduces it). But
in the 8 CLightFxRender shape generators (0xa3dc0…0xa8900, ~2KB each) ~22 such
packs share the 5 globals and the optimizer heavily CSEs the global loads and
schedules the 22 expressions + their u16-temp spills across the frame — that
inter-color scheduling/regalloc is a **wall** no uniform `Pack()` source
reproduces byte-for-byte (~70% plateau on a complete, correct body). Evidence:
CLightFxRender::Shape1..Shape8; the standalone DrawBorder/FillSpan u16 fills match.
