# The vtable-slot DISPLACEMENT of an indirect call names the receiver's static type

tags: cpp:virtual cpp:call cpp:class | asm:call | topic:codegen-idiom topic:correctness
symptoms: `call DWORD PTR [edx+0x2c]` on one side and `call DWORD PTR [edx+0x8]` on the
other at the same construction site; `gruntz walls diagnose` says INLINE/CALL-SET but the
call COUNTS agree and no named referent differs (both sides are indirect, so the masked
diff shows the same mnemonic); a `new CDerived` stored in a base-typed local; one extra
`push` in the argument list
confidence: 10/10 (2026-08-16; the slot map is RTTI ground truth, so this is a decision,
not an inference)

An indirect `call [reg+N]` carries no relocation, so the reloc-sequence diff cannot see
it and the masked instruction diff prints the same text on both sides. But `N` is the
vtable slot, and the slot is fixed by the **static type the source wrote at the call
site**. A different `N` therefore proves the receiver's declared type is wrong - it is a
referent defect that hides inside the "regalloc" bucket.

```cpp
// WRONG - `it` is CSBI_Image*, so `SetupImage` resolves to CSBI_Image's slot 11 (+0x2c)
CSBI_Image* it;
it = new CSBI_WellGoo;
if (!it->SetupImage(this, code, cmd, tab, rc, key, m_gauge, 0)) { ... }
m_gaugeSink = static_cast<CSBI_WellGoo*>(it);
```
```cpp
// RIGHT - CSBI_WellGoo overrides the 7-arg BASE `Setup`, slot 2 (+0x8), and takes no
// `extra` argument; the downcast falls out with the type
CSBI_WellGoo* goo = new CSBI_WellGoo;
if (!goo->Setup(this, code, cmd, tab, rc, key, m_gauge)) { ... }
m_gaugeSink = goo;
```
```asm
; retail CStatusBarMgr::LoadTabSprites, the GAME_..._WELLGOO site
call DWORD PTR [ebp+0x8]        ; slot 2  -> CSBI_WellGoo::Setup
; ours, before the fix
call DWORD PTR [edx+0x2c]       ; slot 11 -> CSBI_Image::SetupImage
```

`gruntz sema class CSBI_WellGoo` prints the whole map and settles it in one command -
slot 2 `CSBI_WellGoo::Setup`, slot 11 `CSBI_Image::SetupImage` (inherited). A base-typed
local that is `new`-ed with several different derived classes is the shape that produces
this bug: every site then dispatches through whichever override the BASE declares.

## The sieve

Split each side's call stream at the `??2@YAPAXI@Z` (operator new) calls, so each group
is one constructed object, and compare the groups pairwise. Reading raw `call [reg+N]`
displacements per group takes one pass over `llvm-objdump -dr` on the compare pair's
base and target objects and finds the wrong receiver immediately; the same grouping also
lines up the constructor-chain cut depth per site, which is what the /Ob1 budget work
needs (repeated-container-call-is-an-inline-member.md).

The fix is worth real score: it took `CStatusBarMgr::LoadTabSprites` 89.58 -> 90.08 and
turned that site's ctor chain from "fully inlined" into retail's `call ??0CStatusBarItem`,
because dropping the extra argument also changed what the front end charged there.

variants: [reloc-sequence-diff-finds-wrong-referents](reloc-sequence-diff-finds-wrong-referents.md),
[masked-diff-hides-branch-target](masked-diff-hides-branch-target.md)
