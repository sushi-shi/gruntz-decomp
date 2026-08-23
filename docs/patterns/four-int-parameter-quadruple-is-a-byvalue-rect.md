# A `sub esp,0x10` + four in-order stores at a call site means the callee takes a rect BY VALUE

tags: cpp:call cpp:struct cpp:param msvc5:mfc | asm:sub asm:mov asm:push | topic:codegen-idiom

symptoms: a helper declared `f(CString s, i32 x0, i32 top, i32 right, i32 bottom)`
  matches its own body well but its CALLER plateaus; the caller's argument setup is
  `sub esp,0x10 / mov eax,esp / mov [eax],..  / mov [eax+4],.. / mov [eax+8],.. /
  mov [eax+0xc],..` where our base emits four `push`es. Separately, the CALLEE's frame
  is 4 bytes short of retail's and cl homes a local in what it thinks is a dead
  argument slot.

confidence: 10/10 (measured both directions on `font`)

## The tell is the ARGUMENT SETUP, not the callee

A callee that takes four `int`s and one that takes a 16-byte aggregate by value have
the **identical** parameter layout, so reading the callee's own prologue cannot tell
them apart - it loads `[esp+0x7c]`, `[esp+0x80]`, ... either way. The caller is where
they differ, and the difference is unambiguous:

| source | cl 5.0 emits |
|---|---|
| `f(text, rc.left, rc.top, rc.right, rc.bottom)` | `push bottom / push right / push top / push left` (reverse order) |
| `f(text, rc)` with `CRect rc` by value | `sub esp,0x10 / mov eax,esp` then four stores **in field order** `[eax] [eax+4] [eax+8] [eax+0xc]` |

The aggregate form is a field copy, so the stores run FORWARD; four separate int
arguments are pushed BACKWARD. That is the whole test.

## Why it is worth more than the call site

The four-int spelling makes each rect field a **separate parameter with its own
lifetime**. Read once and never again, a parameter slot goes dead and cl reuses it -
typically to home a `CString` or another local. Retail cannot do that, because a
by-value struct parameter is one addressable object that stays live as long as any
field is. The visible consequence is a **uniform +4 shift of every `[esp+N]`** in the
callee plus a 4-byte frame difference, which reads as an unfixable regalloc wall.

`FontRenderer::LayoutWrapped` 0x17b120 carried exactly that note ("cl homes `line` in
the dead `begin` arg slot, retail gives it a frame slot (uniform +4 shift); depth-2
variants + state-trials-48 exhausted"). Retyping the quadruple as `CRect rc` removed
the shift outright.

## Measured (2026-08-16, `src/Font/Font.cpp`)

| function | before | after | change |
|---|---|---|---|
| `FontRenderer::LayoutWrapped` 0x17b120 | 99.6397 | **100.0000 EXACT** | `(CString, i32,i32,i32,i32, i32*)` -> `(CString, CRect, i32*)`, plus the two items below |
| `FontRenderer::MeasureWrapped` 0x17ad10 | 95.7843 | **99.7745** | same retype; instruction stream now byte-identical to retail |
| `FontRenderer::DrawWrapped` 0x17a460 | 74.3265 | 75.7026 | its `MeasureWrapped(text, rc.left, rc.top, rc.right, rc.bottom)` became `MeasureWrapped(text, rc)` |

Retyping a parameter to `CRect` **does** change the mangled name
(`VCString@@HHHH@Z` -> `VCString@@VCRect@@@Z`), so it is a deliberate signature change:
verify the `RVA()` binding still resolves. Both of these are project methods bound by
address, and `status update` reported them as `RENAMED in place (same retail rva,
high-water carried)` - no MAX was lost.

## Two companions that landed with it, both re-usable

**1. `return T(a, b);` is not the same as `r.a = ..; r.b = ..; return r;`.** cl 5.0
evaluates constructor arguments **right to left**, so the ctor form emits the second
argument's work - including a `call` - BEFORE the first argument's arithmetic, then
stores the members in declaration order. The named-local form computes in statement
order. Retail's tell is a tail whose `call` comes first and whose two stores come last:

```asm
mov  ecx,[esi]        ; m_font
call ?GetMaxHeight@Font@@QAEH        ; ...the SECOND argument
mov  ecx,[esp+0x2c]   ; maxWidth      the FIRST argument's operands
mov  edi,[esp+0x88]   ; rc.left
sub  ecx,edi
inc  ecx
mov  [esi],ecx        ; ext.width
mov  [esi+4],eax      ; ext.height
```

`MeasureWrapped` 95.78 -> 98.96 on that line alone. The file already spelled it the
right way three lines earlier (`return TextExtent(0, 0);`) - the idiom was the devs'.

**2. `T v = f();` reloads from the RVO slot; `f().field` reads through the returned
pointer.** Retail `mov eax,[esp+N]` where we emit `mov eax,(%eax)` after a
struct-returning call means the source named the temporary:

```cpp
i32 w = MeasureText(line).width;              // mov eax,[eax]     - 99.00
TextExtent lw = MeasureText(line);            // mov eax,[esp+N]   - 99.77
i32 w = lw.width;
```

Copy-INITIALIZATION only. `TextExtent lw; lw = MeasureText(line);` is a different
thing (an assignment through a second temporary) and measured **91.72** - worse than
either.

**3. Declaration order is the init-store order.** With the above applied, the last
divergence was two adjacent zero/`ebx` stores swapped; the source declared
`i32 maxWidth = 0;` before `i32 y = rc.top;` and retail stores `y` first. Swapping the
two declarations took `LayoutWrapped` to EXACT and `MeasureWrapped` 98.96 -> 99.00.

## A second live instance, and the sieve that found it (2026-08-23)

`CTileTriggerContainer::AddToList3` was declared `(BrickTileId, i32 tileX, i32 tileY,
i32 cellKey, i32 player0, i32 player1, i32 player2, i32 player3)` and its one caller
passed `obj->m_extent.left/top/right/bottom`. Retail's caller shows the forward-store
shape through a `lea` rather than `mov eax,esp`, which is the same idiom:

```asm
lea  edi,[esi+0x134]        ; &obj->m_extent
sub  esp,0x10
mov  ebx,esp
mov  ebp,[edi];    mov [ebx],ebp
mov  ebp,[edi+0x4]; mov [ebx+0x4],ebp
mov  ebp,[edi+0x8]; mov [ebx+0x8],ebp
mov  edi,[edi+0xc]; mov [ebx+0xc],edi
```

`AddToList3(..., RECT playerFlags)` with `AddToList3(..., obj->m_extent)` reproduces
that block byte for byte. `CPlay::ValidateLevelTiles` 0xd2dd0 90.3583 -> 90.4351;
the callee's own body is unchanged (80.1184, `RENAMED in place`, high-water carried).

The parameter names were the other half of the finding: they were `player0..player3`
because the callee spreads them into `m_playerFlags[0..3]`, and nobody had connected
that to the caller reading one rect. A brick's four per-player flags reach the trigger
event in the level record's extent rect.

FOUND WITHOUT THE CALLER-SETUP READING, by a whole-function displacement census (base
register ignored) of a `walls offsetscan` row: our side touched +0x134/+0x138/+0x13c/
+0x140 and retail touched **none of them**, because retail reached all four through
the `lea`. That is a cheap tree-wide screen for this pattern - a contiguous run of
2-4 dword displacements one side uses and the other never does, with the LOW one
appearing in a `lea` on the other side.

COST, and it is intrinsic: the declaration loses three type handles, which moves C1's
state for every TU that parses the header. Three untouched functions in two other TUs
dipped in the same build (`CTriggerMgr::ScrollToActiveRecord` 100.00 -> 99.14,
`CPlay::LoadScrollSpeedOptions` 98.75 -> 98.62, `CTriggerMgr::ApplyTriggerA` 87.45 ->
87.44). Every banked MAX and hist survived - `bank` only raises `best` when the
per-function fingerprint is unchanged - but expect the ledger to need an adjudicated
re-bank when you apply this to a widely-included header.

## Where to look for more - and the answer, which is NOWHERE

Any `(..., i32 x0, i32 top, i32 right, i32 bottom, ...)` or `(..., i32 l, i32 t,
i32 r, i32 b)` signature in the tree is a candidate; check the caller's argument setup
for the forward-store shape before retyping. The same reading applies to `Coord`/
`POINT` pairs, where the aggregate form is `sub esp,0x8` plus two forward stores.
The four values need not be spelled as a rect at either end - `AddToList3` reads them
as per-player flags and still takes the rect.

That search is now mechanical and it is **drained**. `gruntz walls aggscan`
aggregates {callee -> hole sizes} over the whole image on both sides and asks the
question of every call site at once; measured 2026-08-23 it reads 179 argument holes
ours against 182 retail over 20 callees, with **zero callees retail hands a block and
we never do, and zero the reverse**. Every named callee's hole-SIZE multiset matches
exactly; the only two functions that differ (`CGrunt::StepGruntMovement`,
`CStatusBarMgr::BuildTabzDialog`) differ in the COUNT of holes at one size, which is
a tail-merge or inlining divergence rather than a signature. `AddToList3` was the last
live instance. Re-run the sieve after any signature change; do not re-derive the
screen by hand.

Two attribution rules the sieve had to learn, both of which a hand screen will
repeat: cl interleaves the copies of consecutive holes, so the `mov reg,esp` can sit
22 bytes after its `sub esp,N` (a fixed byte window read one hole where the
disassembly has four), and cl TAIL-MERGES the argument build, so several
predecessors each fill a hole and `jmp` to one shared call - stopping at the first
`call` byte named `CTriggerMgr::CellDispatch` for two blocks that jump to
`CGrunt::PlaySound`.
