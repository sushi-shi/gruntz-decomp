# `fild;fild / cond fmul,fxch,fmul,fxch / fstp;fstp` is a DUPLICATED-STORE if/else, not a scaled local

- **confidence**: 9/10
- **tags**: `cpp:branch` `cpp:local` `cpp:float` | `asm:fild` `asm:fmul` `asm:fxch` `asm:fstp` | `topic:codegen-idiom`
- **measured**: `DispatchDemoMoverLogic` 0x3c300 72.68 -> **97.21** (two sites)

## Symptoms

Retail keeps two x87 values live ACROSS a branch with no spill at all:

```
fild DWORD PTR [esp+0x1c]      ; y
fild DWORD PTR [esp+0x10]      ; x
test al,0x1
jne  skip
fmul DWORD PTR [ecx+0x18]      ; x *= scaleX
fxch st(1)
fmul DWORD PTR [ecx+0x1c]      ; y *= scaleY
fxch st(1)
skip:
fstp DWORD PTR [ecx+0x10]
fstp DWORD PTR [ecx+0x14]
```

The recompile spills: `fst DWORD PTR [esp+N]` / `fstp QWORD PTR [esp+N]` /
`fld DWORD PTR [ecx+0x1c]` / `fmulp st(2),st`, and a `double` local additionally
forces `mov ebp,esp; and esp,-0x8` (8-byte frame alignment) that retail does not have.

## Reading

Do **not** write "compute into a local, then conditionally scale it":

```cpp
float fx = (float)x;   // spills to a dword slot (float rounding)
double fx = x;         // spills to a qword slot + aligns the frame
if (!(p->m_flags & 1)) { fx *= p->m_scaleX; ... }
p->m_scaledX = fx;
```

Write the **if/else with the stores duplicated in both arms**:

```cpp
if (!(p->m_flags & 1)) {
    p->m_scaledX = static_cast<float>(x * p->m_scaleX);
    p->m_scaledY = static_cast<float>(y * p->m_scaleY);
} else {
    p->m_scaledX = static_cast<float>(x);
    p->m_scaledY = static_cast<float>(y);
}
```

cl5 hoists the two identical `fild`s out of the arm tops and cross-jumps the two
identical `fstp` pairs at the arm bottoms, leaving exactly retail's shape - with no
named FP local there is nothing to round or spill. The two int uses (one per arm)
are also what forces retail's `mov reg,[mem]; mov [esp+N],reg; fild [esp+N]`
GP-register round trip, which a single-use spelling does not produce.

Corollary: an `x87` value that survives a branch with NO `fst`/`fld` around it is
never a named local.

## Related

- [[named-double-local-blocks-ftol-negation]] - the other "don't name the FP local" case.
