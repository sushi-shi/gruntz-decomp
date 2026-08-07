# Two copies of the same struct: retail RELOADS every field, so it was a struct assignment

- **confidence** c9
- **tags** `cpp:local` `cpp:struct` `cpp:expr` | `asm:mov` | `topic:codegen-idiom`

## Symptom

Retail seeds two local `RECT`/`POINT`/small-struct locals from one pointer and emits
**eight** load/store pairs through **two different base registers**; we emit four loads
and eight stores because cl common-subexpressioned the second read of each field.

```
 base                              target (retail)
 movl (%eax), %ecx                 movl %eax, %edx          # second base register
 movl 0x4(%eax), %edx              movl (%edx), %ebx
 movl 0x8(%eax), %ebx              movl %ebx, A(%esp)
 movl 0xc(%eax), %eax              movl 0x4(%edx), %ebx
 movl %ecx, A(%esp)                movl %ebx, B(%esp)
 ...                               ... (two more)
 movl %ecx, E(%esp)   <- reused    movl (%eax), %edx        # RELOADED
 ...                               movl %edx, E(%esp)
```

## Cause

`RECT cur = *rect;` is a **struct assignment**: cl expands it as its own four-pair
block and does not CSE the loads against a *second* expansion, so a `mov <reg>,<ptr>`
copy of the source pointer shows up between them. Field-by-field assignment
(`cur.left = rect->left; ... work.left = rect->left; ...`) is one basic block of
ordinary loads, and cl folds the second read of each field away. The two spellings are
semantically identical, so nothing but the instruction count distinguishes them.

Measured on `CFontConfig::DrawTextLines` 0x22360, 85.40 -> 89.51 (delta -25 -> -18);
the whole difference was `RECT cur = *rect; RECT work = *rect;` vs eight field
assignments.

## Rule

When retail reloads a field it already has in a register and the destination is a
*whole* small struct, write the struct assignment. Reading the reloads as a register
allocation artifact is the trap - the reload count is a source fact here.

related: [instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md),
[cse-defeat-uncached-global-rewalk.md](cse-defeat-uncached-global-rewalk.md)
