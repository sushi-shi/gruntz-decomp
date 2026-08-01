# A min/max clamp of two MEMBERS is an if-WIDEN statement, not a ternary — the far edge loads FIRST
tags: cpp:branch cpp:ternary cpp:member | asm:cmp asm:jle asm:jge asm:jl | topic:codegen-idiom
symptoms: `cmp edx,ecx` + a 2-byte `jle`/`jge` skip over `mov ecx,edx`; jcc_sieve POLARITY rows tagged `=dest`; a ternary spelling that scores identically no matter how the operands and arms are permuted
confidence: 9/10

## Shape

Retail clamps a pair of ints (screen coord vs a stored extent) like this:

    mov  eax,[esi+0x10]        ; the object
    mov  ecx,[eax+0x164]       ; <- the FAR EDGE, loaded FIRST, into the RESULT register
    mov  edx,[eax+0x5c]        ; <- the candidate, loaded SECOND
    cmp  edx,ecx
    jle  +2                    ; (or jge for the min)
    mov  ecx,edx
    mov  [eax+0x13c],ecx

Two loads, a compare, a **2-byte conditional skip** over a single `mov`, then one
store. The register that ends up stored (`ecx`) is loaded first and holds the
"default" side of the clamp.

## Why a ternary cannot produce it

For `a <= b ? b : a`, cl5 loads the **condition's LEFT operand** first, into the
result register, and then has to invert the branch to keep it:

    mov  ecx,[eax+0x5c]        ; screenX - the condition's left operand
    mov  edx,[eax+0x164]
    cmp  ecx,edx
    jg   +2                    ; INVERTED vs retail's jle
    mov  ecx,edx

Same value, same length, opposite mnemonic and opposite load order. Permuting the
ternary (`>=`-swapped, `<`-with-else-first, `>`-with-then-first) only permutes
*which* operand is "left", so every spelling lands on one of the two forms —
which is why an exhaustive ternary matrix ties.

## The fix

Write the statement form, naming the far edge first:

    i32 exRight = m_object->m_164;
    if (m_object->m_screenX > exRight) {
        exRight = m_object->m_screenX;
    }
    m_object->m_extent.right = exRight;

    i32 exTop = m_object->m_168;
    if (m_object->m_screenY < exTop) {         // min: retail's `jge` skip
        exTop = m_object->m_screenY;
    }
    m_object->m_extent.top = exTop;

The local's initializer is the first load and the result register; the `if`'s
comparison is emitted in source order, so the guard keeps retail's polarity.

## Exception: a clamp that reuses ALREADY-LOADED registers

When the two values are still live in registers from a preceding test, cl reuses
them and the ternary form matches on its own — do not touch those. In
`CKitchenSlime::CKitchenSlime` the `left` clamp follows the
`screenX == m_164 && screenY == m_168` equality test, so screenX/m_164 are
already in ecx/edx and the ternary spelling is byte-correct; only the three
clamps that RE-load (`right`, `top`, `bottom`) needed the statement form.

## Evidence

`CKitchenSlime::CKitchenSlime` 0x0b23a0 — three `=dest` POLARITY rows
(`jg`→`jle`, `jl`→`jge`, `jg`→`jle`), 96.97 → 97.60 with the branch sequences
now AGREEING. `config/axes/kitchenslime-clamp.json` had already spent 64 cells on
min/max **ternary** spellings and recorded NEGATIVE ("not condition-spelling
steerable"); the statement form was the class it never enumerated.

Distinct from [`default-hoists-into-destination-no-jmp`](default-hoists-into-destination-no-jmp.md),
whose "assign-then-override" applies to a two-way select with a `jmp`-joined arm
and which explicitly excludes the clamp CSE. This is the clamp CSE, and the
resolution is the same assign-then-override *shape* with the far edge as the
default.
