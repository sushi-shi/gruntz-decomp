# `new T[n]` (T non-trivial) → array-cookie alloc + `__ehvec_ctor` + the /GX frame — model it as a real array-new, not a manual alloc

tags: cpp:new cpp:ctor cpp:eh | asm:push asm:call asm:lea | topic:codegen-idiom topic:eh
confidence: 8/10
variants: gx-frame-destructible-local.md, eh-state-numbering-base.md, newd-class-real-size.md

## Symptom

A ctor allocates an array of small structs and zero-inits each. Retail shows:

```asm
imul edi,ebp                       ; n = rows*cols
lea  eax,[edi*8+4]                 ; alloc size = n*elemSize + 4   (4-byte array cookie)
push eax
call operator new
add  esp,4
test eax,eax; je <fail>
push <elem-dtor>                   ; ??1T  (reloc-masked)
push <elem-ctor>                   ; ??0T  (reloc-masked)
push edi                           ; n
push 8                             ; sizeof(T)
lea  ebx,[eax+4]                   ; array starts AFTER the cookie
push ebx
mov  [eax],edi                     ; store count in the cookie
call __ehvec_ctor                  ; ??_L  (callee-clean __stdcall)
```
…all of it wrapped in a `/GX` SEH frame (`push -1 / push <handler> / mov fs:0,esp`)
so a partially-constructed array unwinds on throw. A hand-rolled
`operator new(n*8+4)` + a manual `extern __ehvec_ctor(...)` call reproduces the
*body* but **NOT the frame** (the compiler doesn't know the array is destructible),
capping it ~60%.

## Fix — write the real array-new and give the element a non-trivial ctor+dtor

```cpp
struct BucketHead {              // the 8-byte element
    void* m_head;
    void* m_tail;
    BucketHead() { m_head = 0; m_tail = 0; }  // non-trivial -> ??_L element ctor
    ~BucketHead() {}                          // user dtor -> array-cookie + frame
};
...
m_buckets = new BucketHead[m_1c]; // -> the whole sequence above, frame included
```

MSVC5 lowers `new BucketHead[n]` (T has a user dtor) to exactly the retail shape:
`operator new(n*8 + 4)`, store `n` in the 4-byte cookie, return `ptr+4`, call
`??_L`(`__ehvec_ctor`) over the array, all inside the `/GX` frame. The element
`??0BucketHead`/`??1BucketHead` are fresh COMDATs (the retail element ctor/dtor at
their own RVAs are reloc-masked at the `??_L` call site — only the address operand
differs). `__ehvec_ctor`/`__ehvec_dtor` are **`__stdcall`** (callee-clean: NO
`add esp` after the call) — if you ever DO model them by hand, declare
`extern "C" void __stdcall ...`, or the spurious caller cleanup caps you ~95%.

Evidence: `CWwdGrid::Setup` (0x1915c0) 61%→79% on the switch to `new[]` (the
frame + alloc + `??_L` region all snap to retail); the matching teardown
`delete[]`/`__ehvec_dtor` path is the FreeBuckets sibling (0x191800, 100%).

WALL residue after this fix: the log2/pow x87 schedule
(x87-fp-stack-schedule.md) and local/register ordering keep the initializer
below exact even though the allocation and frame are structurally correct.
