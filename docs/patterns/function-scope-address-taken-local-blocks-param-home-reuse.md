# A function-scope escaped local can block reuse of a dead parameter home

tags: cpp:local cpp:scope cpp:param cpp:temporary | asm:sub asm:lea | topic:codegen-idiom topic:regalloc
symptoms: base and retail have the same instructions, calls, CFG and relocations,
  but retail's frame is one dword larger; an earlier four-byte object lives in a
  fresh retail slot while base puts it in an incoming parameter home
confidence: 10/10

cl 5.0 packs stack homes from source scopes, not just from the final machine
live ranges. A real address-taken aggregate declared inside a late loop can
share the function's packing pool differently from that same declaration at
function scope. The consequence can appear much earlier: an EH-tracked
temporary is allowed to occupy a dead incoming parameter home in one spelling
and receives a dedicated local slot in the other.

## Controlled A/B: `CFecFile::AddFile` @0x17b950

After restoring the name-fill loop, AddFile was 99.84892%. Its only normalized
residual was the scale-one address

```
base:   lea esi,[ebp+eax+0x1e]
retail: lea esi,[eax+ebp+0x1e]
```

Caching the already reused string length in a sized local,
`i32 length = base.GetLength()`, removed that SIB difference and reached
99.884895%. At that point the instruction stream, call set, branch skeleton,
relocations and normalized residual were all identical. The remaining exact
difference was the frame:

```
base:   sub esp,0x38
retail: sub esp,0x3c
```

The full stack map named the displaced object. The `CString` temporary produced
by `base.Right(...)` used the dead `name` parameter home in base
(`[esp+0x58]` at that point), while retail used a dedicated local
(`[esp+0x1c]`). The function already had one later escaped aggregate:
`MSG msg`, whose address is passed to `PeekMessageA`, `TranslateMessage` and
`DispatchMessageA`.

Moving only that existing declaration from the loop body to function scope
made the frame `0x3c`, preserved all 283 instructions, 33 calls, 19 branches and
27 relocations, and made AddFile plus all three unwind helpers and its EH
registration helper 100% exact.

## Negative controls

The following did not create the retail home:

- naming the `Right()` result as an explicit block-local `CString`;
- separating declaration from assignment of the slash index;
- spelling the path branch as a shared-tail `goto`;
- an inlined whole path mutator or one-message pump helper;
- a trivial const-reference adapter around the `name` argument.

Return-by-value filename and `Right()` wrappers were not controls for this wall:
VC5 declined them under `/Ob1`, leaving wrapper calls and changing the retail
call set. An explicit result local inside an expanded setter added an
instruction but still left the frame at `0x38`.

## Reverse-use rule

When `gruntz walls framescan --folded` shows retail larger by one dword and the
normalized residual is zero:

1. map every local and incoming parameter home in both disassemblies;
2. identify the real temporary that base places in the parameter home;
3. look later in the function for an existing address-taken local declared in
   a narrow block or loop;
4. test its authentic function-scope declaration, then require identical
   calls, CFG, instructions and relocations.

Do not add a dummy address-taken scalar or widen a random scope. The lever is
valid only when both stack roles are already evidenced.

related: frame-size-counts-the-locals.md,
early-return-kills-the-param-home-coalesce.md,
dead-parameter-home-pool-order.md,
escaped-local-scope-decides-frame-slot-packing.md
