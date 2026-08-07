# `lea <reg>,[this+off]` + one register per field = a by-VALUE struct rvalue

tags: cpp:struct cpp:inline cpp:local | asm:lea asm:mov | topic:codegen-idiom

symptoms: a member `RECT`/`POINT`-shaped field is filled with four `mov`s and the
  function plateaus 10+ points below its neighbours. Retail addresses the field
  through a `lea`d base register and materializes each value in its OWN register
  (`xor eax,eax / xor ecx,ecx / xor edx,edx / xor edi,edi` for four zeros); our
  recompile writes `mov [esi+0x290],ebx` four times reusing one zero register.

confidence: 10/10

## The three spellings are NOT interchangeable

For `m_rect.left = -r; m_rect.top = -r; m_rect.right = r; m_rect.bottom = r;`
(plus a second field set to all zeros) cl5 /O2 emits three different shapes:

| source | codegen |
|---|---|
| four scalar member assignments | direct `mov [esi+0xNNN],<reg>` per field, disp32 each, ONE zero register reused |
| an inline helper taking `RECT*` | **identical to the scalar form** - the pointer parameter is folded away |
| a by-VALUE struct rvalue assigned to the member | `lea <reg>,[esi+0xNNN]` once, then `mov [<reg>]`, `[<reg>+4]`, `[<reg>+8]`, `[<reg>+0xc]`, with a SEPARATE register per field value |

Only the third reproduces retail. The rvalue can be

```cpp
m_reachRect = CRect(-r, -r, r, r);       // TU with MFC inlines ON
m_reachRect = MakeRect(-r, -r, r, r);    // TU with MFC inlines OFF
```

where `MakeRect` is a TU-local `static __inline RECT MakeRect(i32,i32,i32,i32)`
that fills a local and returns it by value. `CRect`'s 4-int ctor is only inline
when `_AFX_ENABLE_INLINES` is live, so in a `<MfcNoInline.h>` TU the `CRect`
spelling becomes 43 `call ??0CRect@@QAE@HHHH@Z` -
[out-of-line-crect-ctor-means-mfcnoinline-tu](out-of-line-crect-ctor-means-mfcnoinline-tu.md).

## Why the pointer-taking helper fails

After inlining, a `RECT*` parameter whose argument is `&this->m_field` is a
constant offset from `this`; cl folds it into every store's displacement and the
base register never materializes. A by-value return has no pointer to fold: the
returned aggregate is scalarized into pseudo-registers, and the ASSIGNMENT then
needs a real base for the destination. That is also why the four zeros of
`= MakeRect(0,0,0,0)` cost four `xor`s - they are four independent SSA values,
not one CSE'd constant.

## Evidence

`CGrunt::LoadGruntTypeTable` @0x4dd50 - ~25 sites in one 10 KB switch.

| spelling | insn delta vs retail | frame | fuzzy |
|---|---|---|---|
| four scalar assignments | +106 | `sub esp,0x10` | 47.67 |
| `SetRectValues(&m_reachRect, ...)` (RECT* inline) | +120 | `sub esp,0x10` | 47.10 |
| `m_reachRect = MakeRect(...)` (by value) | **-21** | **`sub esp,0xc`** (retail's) | **58.50** |

The `CRect` half of the pair is corroborated by `CGrunt::SetupTubeAnim` @0x50a50
(97.4%, a `<Mfc.h>`-inlines-ON TU), whose `m_reachRect = CRect(-1,-1,1,1);
m_reachExclusionRect = CRect(0,0,0,0);` compiles to exactly retail's
`lea ebx,[esi+0x290]` / `or eax,-1 / or ecx,-1 / mov edx,1 / mov edi,edx` and
then `xor eax,eax / xor ecx,ecx / xor edx,edx / xor edi,edi`.

## Corollary for reading the target

A run of member stores that goes through a `lea`d base is telling you the source
assigned a whole struct there. Counting the distinct registers holding the stored
values gives the arity of the rvalue's constructor/return - four separately
zeroed registers is four literal `0` arguments, not one memset.
