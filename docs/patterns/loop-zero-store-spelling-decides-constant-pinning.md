# Which constant gets pinned in a callee-saved register is decided by the ZERO-STORE spelling

**Tags:** `topic:codegen-idiom` `cpp:loop` `cpp:local` | `asm:mov` `asm:xor` `asm:test` | `topic:regalloc`

## Symptom

Retail materialises a small constant into a register and uses the register form, while
the recompile uses the immediate — or vice versa — and a *different* constant is the one
that got the register:

```
retail                                  base
mov  bl,0x1                             xor  ebx,ebx
...                                     ...
test BYTE PTR [ecx+esi*1+0x8],bl        test byte ptr [ecx+esi+0x8],0x1
xor  edx,edx          ; fresh per iter  ...
mov  DWORD PTR [esp+0x10],edx           mov  dword ptr [esp+0x18],ebx
mov  DWORD PTR [esp+0x1c],edx           mov  dword ptr [esp+0x1c],ebx
```

Or, in a blank-fill loop:

```
retail                                  base
mov  ecx,DWORD PTR ds:<buf>             mov  ecx,0x720          ; constant pinned
add  eax,0x2                            mov  edx,DWORD PTR ds:<buf>
mov  WORD PTR [ecx+eax*1-0x2],0x720     mov  WORD PTR [eax+edx-0x2],cx
```

## Mechanism

cl5 hoists any loop-invariant constant into a callee-saved register when one is free and
the constant is used more than once. There is only ONE such register up for grabs, so the
constants compete: whichever the source makes look loop-invariant *first* wins it, and
every other constant in the loop falls back to an immediate (or gets pushed into a
volatile register, which then displaces something else).

The source lever is how the loop writes its zero/constant, not the constant itself:

| you wrote | cl does |
|---|---|
| two separate field stores `x.a = 0; x.b = 0;` | hoists the 0 into the callee-saved reg for the whole loop |
| `memset(&x, 0, sizeof x)` | inlines the same two dword stores from a **volatile** `xor edx,edx` *inside* the loop |
| `i += 2;` first, then `p = buf + i - 2` | hoists the stored constant into a register (`mov ecx,K` … `mov [..],cx`) |
| `p = buf + i;` … `i += 2;` last | keeps the stored constant an **immediate** (`mov WORD PTR [..],K`) — cl still rewrites the address to `add eax,2` / `[.. - 2]`, so the bytes are otherwise identical |

## Evidence

- `src/Crypto/BlowfishCopy.cpp`, `?BitStreamBlowfishEncode@@YGXPAVistream@@PAVostream@@@Z`
  @ 0x16f6e0: **84.87% -> 100.00% EXACT**. The two-field-store record clear hoisted the
  zero into `ebx`, which denied `ostream::eof`'s mask `1` its register; `memset(rec.m_bytes,
  0, 8)` freed `bl` for retail's `mov bl,1` / `test [..],bl`. Filed for months as a
  "const-materialize-into-reg-vs-immediate regalloc wall".
- `src/Rez/DebugPrintf.cpp`, `?MonoNewline@@YAXXZ` @ 0x184d50: **90.95% -> 98.57%**. Its
  blank loop was pre-increment and hoisted `0x720` into `ecx` (pushing the page pointer
  into `edx`); `MonoClear`'s post-increment spelling of the same loop keeps `0x720` an
  immediate and the pointer in `ecx`, matching retail.
- Non-levers, measured on the same two functions: chained `a = b = 0`, store order, a
  named `u32 z = 0;` temp, `0x0720` vs `0x720`, hoisting the record out of the loop,
  `eof() == 0` vs `!eof()` — all byte-identical.

## See also

- `docs/patterns/zero-register-pinning.md` — the family this is the steerable half of.
- `docs/patterns/zero-group-loop-gives-its-own-constant.md` — the same competition read
  from the constant's side.
