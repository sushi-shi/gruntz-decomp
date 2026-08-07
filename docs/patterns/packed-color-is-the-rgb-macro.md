# `mov ch,al / mov cl,bl / shl ecx,8 / or ecx,esi` IS the Win32 `RGB()` macro

- **confidence**: 9/10
- **tags**: `cpp:const` `cpp:local` | `asm:shl` `asm:or` `asm:mov` | `topic:codegen-idiom`
- **measured**: `CCreditsState::FlashColor` 0x39d00 84.90 -> **100.00 EXACT**

## Symptoms

Retail packs three byte-sized values with **partial registers**:

```
xor  ecx,ecx
...
mov  ch,al        ; high channel
mov  cl,bl        ; middle channel
shl  ecx,0x8
or   ecx,esi      ; low channel (this one carries an explicit `and esi,0xff`)
```

and the recompile emits a plain shift chain instead:

```
shl eax,0x8 / or eax,esi / shl eax,0x8 / or eax,edi
```

## Reading

cl5 only reaches for `ch`/`cl` when it can prove each channel is `< 256`, which needs
the mask on **every** channel - so a hand-written `(b << 16) | (r << 8) | g` with a
mask on only some of them falls back to the shift chain. The masks-on-all form is
exactly the Win32 `RGB()` macro (`wingdi.h`):

```cpp
color = RGB(r, g, b);      // instead of (b << 16) | (g << 8) | r
```

Only the LOWEST channel keeps a visible `and reg,0xff` - the other two are masked
implicitly by the byte-register moves.

## The bug this exposes

The partial-register form also **names the channel order**: `ch` is the high byte
(<<16), `cl` the middle (<<8), the `or` operand the low one. Read against the order
of the `rand()` calls that produced them, that caught a real transcription bug -
the reconstruction had the middle and low channels swapped, which the flat diff had
shown only as "different shift scheduling".

## Related

- [[cse-partial-term-is-not-a-separate-constant]] - the other packed-channel trap.
