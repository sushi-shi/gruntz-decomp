# A row stride recomputed in every mutually exclusive arm was not a shared local
tags: cpp:branch cpp:local cpp:expr | asm:lea asm:sub asm:shl | topic:codegen-idiom topic:correctness
symptoms: four quadrant arms repeat `lea width*8`, `sub width`, `shl 2`; base hoists one stride and is dozens of instructions short
confidence: 9/10

When retail recomputes the same row stride inside every mutually exclusive arm,
the source did not name one `stride` above the branch chain. That local gives VC5
permission to share the value and deletes each later arm's address arithmetic.

```cpp
// Wrong: asserts a source-level CSE that retail does not contain.
i32 stride = board->m_width * 7 * 4;
if (dx > 0 && dy > 0)
    Test(tgt - stride);
else if (dx < 0 && dy > 0)
    Test(tgt - stride);

// Retail shape: each arm owns the expression it uses.
if (dx > 0 && dy > 0)
    Test(tgt - board->m_width * 7 * 4);
else if (dx < 0 && dy > 0)
    Test(tgt - board->m_width * 7 * 4);
```

```asm
; repeated inside each retail quadrant
lea  ecx,[edx*8]
sub  ecx,edx
shl  ecx,2
```

STEERABLE. In both inlinings of `CGrunt::StepCompassMove` at `0x00051c00`,
removing the false shared local recovered 24 of 41 missing instructions and
raised the current-source result from 58.7547% to 60.7711%.
