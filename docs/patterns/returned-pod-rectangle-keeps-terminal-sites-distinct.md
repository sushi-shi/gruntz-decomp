# A returned POD rectangle keeps terminal inline-helper sites distinct

tags: cpp:inline cpp:struct cpp:return | asm:call asm:jmp asm:ret | topic:codegen-idiom topic:regalloc
symptoms: retail contains one direct callee and one return per terminal branch,
while the recompile builds the same rectangle in each branch but jumps the last
two branches into one shared call and epilogue; call, branch and return counts
are each short by one
confidence: 9/10

When an inline member helper builds a plain `RECT` and immediately passes it to a
call, cl 5.0 may factor source-identical terminal expansions even when every arm
has its own block-scoped local. A separate inline POD value builder supplies the
front end with an aggregate-return/copy expression and keeps the sites distinct:

```cpp
static __inline RECT MakeRect(i32 left, i32 top, i32 right, i32 bottom) {
    RECT rc;
    rc.left = left;
    rc.top = top;
    rc.right = right;
    rc.bottom = bottom;
    return rc;
}

inline void Worker::BlitDirtyRect(SurfacePair* src, i32* pos, i32* size) {
    RECT rc;
    rc = MakeRect(pos[0], pos[1], pos[0] + size[0], pos[1] + size[1]);
    surface->BltEx(&rc, src->surface, &rc, FLAGS, NULL);
}
```

`CWwdGameObjectA::BltDirtyEx` at 0x1506b0 and
`CWwdGameObjectC::BltDirtyEx` at 0x1662a0 are the controlled pair. Their exact
`BltDirtyRegions` siblings prove the caller CFG and arguments; the static receiver
type selects the base inline helper for `CDrawSubWorker*` and the derived
out-of-line empty helper for `CDDrawSurfacePair*`. The dead retail
`CDDrawSubMgrPages::BltDirtyChildrenEx` caller passes its
`CDDrawSurfaceChildA*` front pair, independently proving that the first argument
uses the shared `CDrawSubWorker*` base rather than `CDDrawSurfacePair*`; names in
the generated target COFF are reconstructed labels and are not ABI evidence.
With direct field stores,
aggregate initialization, direct MFC `CRect` construction, explicit early returns,
or per-arm `RECT` locals, both recompiles retain only four `BltEx` calls. With the
returned plain `RECT`, A has retail's 7 calls / 5 branches / 4 returns / 7
relocations, and C has retail's 5 / 8 / 4 / 5 plus its exact 0x1fa-byte extent.

Two apparently cleaner models are negative controls. Replacing each adjacent
coordinate pair with `POINT`/`SIZE` changes no evidenced complete-object use and
loses retail arithmetic. Passing four scalar values improves fuzzy alignment but
makes A four instructions short and C two instructions short; a returned
size-based rectangle builder is byte-identical to that scalar form. The pointer
pair form is retained because it preserves retail's instruction counts and C's
exact extent, not because its fuzzy score happens to be higher or lower.

The remaining differences are operand scheduling, not evidence against the
helper. Bounded 32-variant campaigns for both functions found one compiler island
and no improvement. The reverse-use rule is therefore narrow: when terminal
inline-helper arms are short by one whole call and epilogue, test a real
return-by-value POD builder before inventing branch-specific work. Do not use a
class wrapper, volatile carrier, or dummy local to defeat factoring.
