# Retail re-reads a field the guard just proved zero; cl copy-propagates it

- **confidence** c7
- **tags** `cpp:branch` `cpp:member` | `asm:cmp` `asm:mov` | `topic:wall`

## Symptom

A guarded block whose first statement stores the very field the guard tested:

```cpp
if (obj->m_field == NULL) {
    m_cache = obj->m_field;          // provably NULL here
    obj->DoSomething("str", 0);
}
```

Retail emits a **fresh load** of the field, through the call's `this` register:

```
mov edx,dword ptr [esi+0x38]
cmp dword ptr [edx+0x1b4],ebp      ; ebp == 0
jne  skip
mov ecx,edx                        ; this for the call
push ebp
push <str>
mov eax,dword ptr [ecx+0x1b4]      ; <-- RE-READ, not folded to ebp
mov dword ptr [esi+0x40],eax
call <DoSomething>
```

cl 5.0 in this tree instead substitutes the guard's zero register and drops the load:

```
mov ecx,dword ptr [esi+0x38]
cmp dword ptr [ecx+0x1b4],ebx      ; ebx == 0
jne  skip
push ebx
push <str>
mov dword ptr [esi+0x40],ebx       ; <-- copy-propagated
call <DoSomething>
```

## Why it matters beyond the two instructions

The propagated store is one MORE use of the zero constant. On a function with a spare
callee-saved register that extra use tips cl's constant hoisting from "materialise zero
late, in a register that just died" to "hold zero for the whole function in a fresh
register" - so cl claims a 4th callee-save, and **every `[esp+N]` frame offset in the
function shifts by 4**. A 3-instruction divergence therefore reads as a 40-point drop.

## Not steerable from source

Seven spellings were measured on `CFrontCandyAni::CFrontCandyAni` (0xacf40): plain, a
local for the object, a pointer local for the sub-struct, mixed receivers between the
guard/store/call, and both statement orders. Every assign-BEFORE-call form propagates
and lands at 49.03%; the assign-AFTER-call form (`call` then read) blocks propagation,
restores retail's register allocation and reaches 90.95%, at the cost of one reload
emitted after the call instead of before it.

Same residue, unchanged, in `CEyeCandyAni` (0xac870, 93.58%) and `CBehindCandyAni`
(0xad540, 92.63%) - so it is the compiler, not the reconstruction.

## Rule of thumb

When a guarded block re-reads its own guard expression and your base is ~40 points below
retail, look at the **saved-register count in the prologue** before anything else: one
extra `push` there is the whole diff.
