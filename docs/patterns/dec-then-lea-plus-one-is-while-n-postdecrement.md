# `mov edx,eax / dec eax / test edx,edx / lea ebp,[eax+1]` is `while (n--)`, not a guarded do-while

tags: cpp:loop cpp:while cpp:do-while | asm:dec asm:lea asm:test | topic:codegen-idiom
symptoms: retail computes the trip count, copies it, decrements the copy, tests the
ORIGINAL, and then immediately adds the 1 back with a `lea reg,[reg+1]` to seed the loop
counter; our recompile emits a plain `test n,n / je / ... / dec n / jne` and is 4-6
instructions short
confidence: 9/10

MSVC 5.0 `/O2` compiles

```cpp
i32 n = coll.m_grown;
while (n--) {
    if (p != NULL) {
        p->CString::CString();
    }
    p++;
}
```

as

```asm
  mov  eax,DWORD PTR ds:<m_grown>
  mov  edx,eax          ; the value of the post-decrement expression
  dec  eax              ; the side effect
  test edx,edx          ; ...which is what the loop tests
  je   <after>
  lea  ebp,[eax+0x1]    ; and cl immediately rebuilds n as the trip count
  test edi,edi          ; loop body
  ...
  add  edi,0x4
  dec  ebp
  jne  <loop>
```

The `dec`/`lea +1` pair looks like dead arithmetic and invites a "cl is being silly"
reading. It is not: it is the literal lowering of a **post-decrement in the condition**.
cl evaluates `n--` (value in `edx`, side effect in `eax`), tests the value, then
strength-reduces the loop into a counted `dec`/`jne` and has to reconstruct the count.

The guarded do-while a reconstruction naturally reaches for is a **different** program
and compiles to a different, shorter prologue:

```cpp
// NOT this - no dec, no lea, and cl tests the live counter directly
if (n != 0) {
    do { ...; } while (--n != 0);
}
```

So when the target carries the `dec`+`lea +1` pair, write the post-decrement `while`.

## Measured

`CTriggerMgr::ClearCell` 0x6e800 **76.83 -> 86.36** and `CTriggerMgr::UseToyAt`
0x6e120 **81.43 -> 82.99**, both on the `g_typeColl.Slots()` / `m_grown` CString
re-construction walk, by that single rewrite. After it the two prologues are
register-for-register identical to retail.

Both sites are the same three-statement shape and it very likely recurs wherever the
`z`-library scratch collections are re-initialised:

```cpp
CString* p = g_typeColl.Slots();
i32 n = g_typeColl.m_grown;
while (n--) { ... }
```

`rg -n 'while \(--' src` is the worklist for the inverse mistake.
