# A farther cursor declared first preserves retail's address recomputation

tags: cpp:local cpp:pointer cpp:order | asm:lea asm:add | topic:codegen-idiom
symptoms: two loop cursors share a base and stride; retail forms the farther
cursor through a temporary nearer address, then recomputes the nearer cursor,
while the reconstruction computes the nearer cursor once and derives the
farther cursor from it
confidence: 8/10

When two walking cursors address adjacent planes, algebraically sharing the
nearer cursor can erase a source-level local creation boundary.  This source:

```cpp
u8* green = scan + width * 2;
u8* blue = green + width;
```

lets cl 5.0 retain `green` as the common expression.  Retail instead showed:

```asm
lea edx,[scan+width*2]  ; temporary used to form blue
mov esi,width
add esi,edx             ; blue
lea edi,[scan+width*2]  ; green recomputed independently
```

That sequence is selected by declaring the farther cursor first and spelling
both addresses from their common semantic base:

```cpp
u8* blue = scan + width * 3;
u8* green = scan + width * 2;
```

The multiplication by three may still lower as `(scan + 2*width) + width`;
the important evidence is that `blue` is created before `green`, forcing the
second `2*width` address formation rather than sharing the named local.

Measured on `CRezImage::DecodePcxData` at 0x176000: 93.8608% to 97.7215%.
The change made the base and target both 158 instructions with identical call,
branch, return, and relocation counts.  The remaining residue is a consistent
whole-function register-role swap: width/scan buffer use EBP/EDI in the base
and EDI/EBP in retail.

Negative control: naming a `u8* red = scan` cursor and using `red[x - 1]` was
byte-flat.  Add a local only when retail's address-formation order proves that
entity; an alias optimized away does not steer this case.
