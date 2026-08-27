# A repeated scalar byte copy can be an inline helper, not `memcpy` or a macro

- **confidence** c10
- **tags** `cpp:inline` `cpp:loop` `cpp:pointer` `cpp:macro` `cpp:builtin` | `asm:mov` `asm:dec` `asm:rep` | `topic:codegen-idiom` `topic:regalloc` `topic:negative-control`

## Symptom

Several callers contain the same forward byte-copy loop. A hand-expanded loop has the
right scalar `mov`/`inc`/`dec` texture and can be within a few bytes of retail, but its
frame homes or register roles differ. Replacing it with `memcpy` is semantically tempting
and structurally wrong: cl 5.0 recognizes the intrinsic and emits `rep movsd`/`rep movsb`.

The missing source boundary can be a small inline function:

```cpp
static inline void CopyBytes(u8* dst, const u8* src, i32 count) {
    while (count-- > 0) {
        *dst++ = *src++;
    }
}
```

The function is fully expanded, so no `CopyBytes` COFF symbol or call survives. Its
parameter entities still give VC5's front end different ownership and lifetime input
from caller-local cursors.

## FaderEffects evidence

`CFaderShape::RenderTile` 0x00182610 was the decisive control. The hand-expanded loops
were already 747 bytes with the retail 32 branches, one return, and no relocations, but
stopped at 99.9831% because two frame homes were reversed. Across 128 mixed declaration
forests plus the baseline, all 129 builds emitted one state. Replacing the two repeated
copies with the TU-local inline helper changed the function-local optimized graph and
made the complete 747-byte body exactly 100%.

The alternatives separate source abstraction from mere behavior:

| source form | `RenderTile` result | discriminating output |
|---|---:|---|
| hand-expanded caller loops | 99.9831% | same extent and topology; two frame homes reversed |
| TU-local inline function | 100.0000% | complete instruction and relocation identity |
| direct CRT `memcpy` at the exact site | 93.0085% | 744 bytes; intrinsic `rep movsd`/`rep movsb` |
| block macro with private cursors, used at all sites | 97.12% | caller-owned locals; different schedules |

The repeated-family controls agree. The helper raises `CFaderSine::RenderFrame` from
92.4279% to 94.6626%. In `CFaderShape::RenderWarpTile`, converting only the first mirror
arm reaches 87.6353% but creates 83 branches against retail's 82; converting both arms
restores the correct 82-branch skeleton. That asymmetric high score is a local maximum,
not evidence that only one source site used the operation.

Ownership was tested separately. Putting the helper on the shared `CFader` class emits
the same FaderEffects bodies but exposes a new declaration to unrelated consumers in
three translation units. A TU-local free inline preserves the caller results without
inventing a public class API. `static inline` and external `inline` are byte-identical
here and neither emits an out-of-line symbol, so the narrower internal linkage is the
supported model.

The macro control also exposes a TU-state trap. It restores the previously exact
`CFaderRadial::ApplyInit` compiler state while making every converted copy caller worse;
the real inline declaration moves Radial to 99.8070%. The retained source follows the
repeated caller evidence and preserves Radial's already-banked MAX rather than choosing
the wrong abstraction to recover an unrelated current score.

## Reverse-use rule

1. Require a repeated semantic copy operation and a retail scalar loop. A `rep movs` or
   real `_memcpy` call belongs to a different pattern.
2. Compare a direct loop, a narrow inline helper, the CRT intrinsic, and a block macro.
   The helper is supported when it preserves scalar topology and moves multiple callers
   coherently; behavior alone is insufficient.
3. Check both mirror arms and the full branch skeleton. A one-arm score spike can be a
   CFG-local maximum.
4. Keep the helper in the narrowest real owner. A broadly included class declaration is
   not an acceptable compiler-state carrier.
5. Treat unrelated exact-function movement as TU state. Preserve its MAX; do not select
   a refuted abstraction to restore the current score.
