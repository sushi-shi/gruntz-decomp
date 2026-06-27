# Unsigned `strlen(s) > N` reuses the inline-strlen `dec` flags for a free `js`
tags: cpp:builtin cpp:branch cpp:type | asm:js asm:cmp asm:jle | topic:wall
symptoms: after `repne scasb; not ecx; dec ecx` the target does `js Lfail; cmp ecx,N; jle Lok`; your recompile emits only `cmp;jle` (signed) or a single `jbe` (unsigned)
confidence: 7/10

A length-bound check `strlen(name) > N` where retail lowers the *unsigned* compare
by splitting it across the inline-strlen's already-live `dec ecx` flags: `js` (catch
the sign bit = unsigned-huge) then a signed `cmp ecx,N; jle Lok`. No clean C spelling
reproduces it — the `js` is a cl scheduling choice off the live `dec` flags, not a
source-level branch.

```asm
repne scasb ; not ecx ; dec ecx
js   Lfail            ; <- free negative/huge half, reuses dec's SF
cmp  ecx, 0x10
jle  Lok             ; signed low half
```
WALL. `(i32)strlen(s) > N` emits the signed `cmp;jle` WITHOUT the `js` (closest, target
minus 2 bytes/check); `(u32)`/no-cast emits a single `cmp;jbe` (a different opcode at
that slot, worse); an explicit `n<0||n>N` reschedules (much worse). Evidence:
CNameRecord::SetNames 0x118040 — `(i32)` cast plateaus ~96.6%, the only residual being
the two missing `js` + the downstream branch-displacement cascade.
