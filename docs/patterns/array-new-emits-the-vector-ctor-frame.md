# `new T[n]` — the /GX frame, the trylevel 0/-1 bracket and the `p ? (ctor loop, p) : 0` merge

tags: cpp:new cpp:ctor cpp:eh cpp:loop | asm:call asm:jmp asm:xor | topic:codegen-idiom topic:eh
symptoms: retail has a `/GX` prologue (`push -1 / push handler / mov fs:0,esp`), a
`mov [esp+trylevel],0` right after an allocator call and a `mov [esp+trylevel],-1` after a
ctor loop, plus a `jmp` / `xor eax,eax` merge on the alloc-failure edge — and the recompile
has NONE of them and is ~70 bytes shorter
confidence: 10/10

## Symptom

A block of records is allocated and initialised. Retail:

```asm
mov  esi,[this+count]
lea  eax,[esi+esi*2] / lea eax,[eax+eax*4] / shl eax,2   ; n * sizeof(T)
push eax
call ::operator new
add  esp,4
mov  [esp+0x14],eax
test eax,eax
mov  DWORD PTR [esp+0x20],0x0        ; <-- EH trylevel 0
je   .empty
dec  esi / mov edi,eax / test esi,esi / jl .done
inc  esi
.loop: mov ecx,edi / call T::T() / add edi,sizeof(T) / dec esi / jne .loop
mov  eax,[esp+0x14]
jmp  .done
.empty: xor eax,eax                  ; <-- the null merge
.done:  test eax,eax
mov  DWORD PTR [esp+0x20],0xffffffff ; <-- EH trylevel -1
```

and the whole function carries a `/GX` frame it otherwise has no reason to.

Written as a hand-rolled `RezAlloc(n * sizeof(T))` + an explicit ctor loop, **none** of
that appears: no EH frame, no trylevel stores, no null merge — and the missing frame also
moves every callee-saved assignment (this/node landed in the swapped `ebx`/`ebp` pair).

## The fix

It is `new T[n]`. MSVC 5.0 lowers array-new to exactly the sequence above: the
element-count multiply as a `lea/lea/shl` chain, the allocator call, an EH state
bracketing the inline vector-ctor loop (so a throwing element ctor unwinds the built
prefix), and the `p ? (construct, p) : 0` merge that gives the alloc-failure edge its own
`xor eax,eax`.

Corollary that usually comes with it: **a method whose whole job is object initialization
and `return this;` may be a constructor, including a parameterized constructor.** A
constructor and a pointer-returning `__thiscall` initializer have the same ABI return
shape; only the mangled name distinguishes them. Require independent construction
evidence: all callers should occur at complete-object, base, or member construction
sites, and the stores/allocations should initialize the receiver rather than mutate an
already-live object.

For a default constructor, declare it as the ctor and drop any explicit member-ctor call
from the body — members then construct implicitly ahead of it (emitting one by hand emits
it twice). For a parameterized base constructor, move derived wrappers from
`Construct(n);` bodies to `: Base(n)` initializer lists.

## Evidence

`CSymParser::PopParseSlot` @0x13c0c0 — **77.13 % → 98.22 %** in one change, having been
filed as an "EH-state + regalloc wall (the node/array allocations land in a swapped
callee-saved register, the operator-new trylevel transitions and the slot-block
down-counter init loop idiom diverge)". Every one of those symptoms was the missing
array-new. `??0CParseSource@@QAE@XZ` stayed EXACT across the rename.

`CHashBase::Construct(i32)` @0x184960 was already 112-byte exact and carried an identical
1/1 EH map, but all of its callers were derived-member construction sites. Remodeling it
as `CHashBase(i32)` and using four derived base-initializer lists preserved the exact body,
all caller bytes, and all caller EH maps after relabeling. This is the negative control:
exact output did not prove the old semantic name or source structure.

## Related

* [[ob1-inline-budget-divergence]] — the other reason a ctor is `call`ed at one site and
  expanded at another.
* [[eh-state-numbering-base]] — when the trylevel NUMBER (not its presence) is the residue.
