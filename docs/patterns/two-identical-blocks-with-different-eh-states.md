# Two identical blocks with DIFFERENT EH state numbers were written twice in source

- **Confidence:** 9/10
- **Tags:** `cpp:eh` `cpp:loop` `cpp:branch` | `asm:mov` `asm:jmp` | `topic:codegen-idiom` `topic:eh`
- **Seen:** `CBattlezDlgCustom::DoDataExchange` 0x000180e0, 74.89 -> 94.32.

## Symptom

Retail is a large fixed amount longer than the recompile (here 134 B of a
575 B function), and reading the two side by side shows retail carrying **two
byte-for-byte copies of one loop body**: the first copy falls out of the loop
(`cmp eax,-1 / je <end>`), the second closes it (`cmp eax,-1 / jne <top>`).
The recompile has one copy and a normal `do { } while (...)` rotation.

## The discriminator

Read the **trylevel writes** inside each copy. Here:

```
copy A:   mov BYTE PTR [esp+0x54c],0x2   ... mov BYTE PTR [esp+0x53c],bl
copy B:   mov BYTE PTR [esp+0x54c],0x3   ... mov BYTE PTR [esp+0x53c],bl
```

cl5 assigns EH state numbers by a **lexical walk of the source**, before any
block duplication the optimizer performs. So:

- **same state number in both copies** => the optimizer duplicated one source
  block (peeling / rotation). Do NOT duplicate it in source; chase the loop
  spelling instead.
- **different state numbers** => the two copies are **two distinct lexical
  temporaries**, i.e. the source genuinely contains the body twice.

## The fix

Write the peeled iteration out, then the loop:

```cpp
if (h != -1) {
    <body>                              // temporaries -> state 2
    while (_findnext(h, &fd) != -1) {
        <body>                          // temporaries -> state 3
    }
}
```

not

```cpp
if (h != -1) {
    do { <body> } while (_findnext(h, &fd) != -1);
}
```

The duplicated spelling is ugly, but it is what the devs wrote, and it is the
only thing that produces two independently-numbered EH regions.

## Screening

Only applies to /GX functions whose loop body constructs a destructible
temporary (a `CString` expression, a by-value class argument). A body with no
EH state carries no evidence either way — for those, the loop-rotation patterns
([`do-while-duplicates-the-leading-call`](do-while-duplicates-the-leading-call.md))
govern instead.

## Related

- [`trylevel-bracket-is-an-raii-guard-object`](trylevel-bracket-is-an-raii-guard-object.md)
- [`do-while-duplicates-the-leading-call`](do-while-duplicates-the-leading-call.md)
