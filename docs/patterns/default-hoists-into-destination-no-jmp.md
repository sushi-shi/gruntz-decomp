# The two-way select whose low arm has NO `jmp`: the default is hoisted INTO the destination

tags: cpp:branch cpp:local cpp:ternary | asm:mov asm:jmp asm:jle | topic:codegen-idiom topic:regalloc
symptoms: retail's select is `mov <dst>,<default> / cmp .. / jcc J / mov <dst>,[other] / J:` — the
  low arm is pure fall-through with no `jmp` and no block of its own; the recompile emits the
  textbook two-arm shape `cmp / jcc L1 / mov <dst>,[other] / jmp J / L1: mov <dst>,<default> / J:`,
  i.e. TWO extra instructions per site; the register colouring downstream of the join differs
confidence: 9/10
variants: default-then-override-flag.md, clamp-ternary-keeps-literal-in-compare.md,
  map-lookup-ternary-ifconverts.md

## Shape

A value is one thing normally and another under a condition. Retail puts the DEFAULT into the
destination register **before** the compare, so the common path just falls through:

```asm
    mov     edx,DWORD PTR [edi+0x170]     ; level
    cmp     edx,0x16
    mov     eax,edx                       ; cap = level      <- default, hoisted
    jle     J                             ; low arm: nothing at all
    mov     eax,DWORD PTR [edi+0x19c]     ; cap = m_19c
J:  test    eax,eax
```

A `?:` — and any if/else that assigns in **both** arms — gives cl two real arms plus a join jump:

```asm
    cmp     edx,0x16
    jle     L1
    mov     eax,DWORD PTR [edi+0x19c]
    jmp     J
L1: mov     eax,edx
J:  test    eax,eax
```

## The fix

Write it as **assign-then-override** — one statement seeding the destination, then a one-armed
`if` that overwrites it:

```cpp
// NO  - two arms + a jmp
i32 cap = (level > 0x16) ? unit->m_19c : level;

// YES - retail's shape
i32 cap = level;
if (level > 0x16) {
    cap = unit->m_19c;
}
```

The same rule governs an `if/else` whose else-arm only exists to seed the default. Zeroing the
value **inside** the else costs you the hoist AND lets cl reuse that freshly-zeroed register for
an unrelated member store in the same arm, where retail writes an immediate:

```cpp
// NO  - `flags = 0` in the else: cl emits `xor eax,eax; mov [esi+0x538],eax`
u32 flags;
if (useDS == 1) { m_useDS = useDS; flags = 0x100000; } else { m_useDS = 0; flags = 0; }

// YES - retail: `xor eax,eax` above the cmp, then `mov DWORD PTR [esi+0x538],0x0`
u32 flags = 0;
if (useDS == 1) { m_useDS = useDS; flags = 0x100000; } else { m_useDS = 0; }
```

## The assign-then-CANCEL direction

When the override is the *default* and the interesting value is the loaded one, invert it the same
way: **assign the load first, cancel it after.** Any test-first spelling lets cl constant-propagate
the equality into an immediate and fold the load into a memory compare:

```cpp
// NO  -> cmp DWORD PTR [esi+0x19c],0x1 / jne / mov eax,0x1     (all three spellings:
//        `if (m_19c == 1) v = m_19c;`, `i32 t = m_19c; if (t == 1) v = t;`, and the `&&` form)
// YES -> mov eax,[esi+0x19c] / cmp eax,0x1 / jne J / mov ecx,eax
i32 v = m_19c;
if (v != 1) {
    v = 0;
}
```

The cancel arm usually emits nothing at all: the destination register is already 0 from the
function's shared `xor`, so cl just branches over the copy.

## The dual: block polarity

Which arm retail keeps **inline** tells you which side the source's `if` tested. A `jcc` that
jumps *over* a block means that block is the fall-through arm — so it is the `if` body, not the
`else` body. Three shapes from the same session:

| retail | source shape |
|---|---|
| `test eax,eax / jne <SetAtGrow>` (the drop is inline) | the FAILURE arm is the `if`: `if (Refresh(..) == 0) { delete } else { insert }` |
| `cmp eax,edi / je <lost-source>` (the body is inline) | the POSITIVE gate: `if (resolved != 0) { ...; return 0; }`, lost-source after |
| `cmp ecx,ebp / jne <call>` (the zero arm is inline) | the explicit else: `if (obj == 0) kind = 0; else kind = obj->GetClassId();` — `&& obj != 0` gives `je` and swaps both blocks |

## Measured

| function | before -> after | site |
|---|---|---|
| `CSBI_StatzTabGruntBar::Update` @0x0ea6c0 | 91.69 -> **branch sequence AGREES** | two `?:` selects on `m_entranceReason`/`m_19c` |
| `CMoviePlayer::OpenLo` @0x017c570 | 70.73 -> **82.32** | `flags` zero hoisted out of the else |
| `CMoviePlayer::OpenHi` @0x017c630 | 70.73 -> **82.32** | same |
| `CGrunt::CommitNeighbor` @0x005b050 | 94.72 -> **95.14** | assign-then-cancel on `m_19c == 1` |
| `EnumSurfacesCallback` @0x013e9a0 | 87.45 -> **99.08** | block polarity; note had read "/GX ctor-in-flight EH state" |
| `CGruntVoice::Update` @0x011a8e0 | 87.08 -> **100.00 EXACT** | block polarity on the last gate |

STEERABLE. Every one of these had been filed as a regalloc / scheduling / EH wall.

## Where it does NOT apply — measure before reaching for it

**1. The default IS the literal the condition compares against.** `i32 v = K; if (x >= K) v = x;`
gives cl one live range seeded with `K`, so it CSEs the second `K` into that register and compares
register-to-register (`cmp edx,esi`) where retail keeps the immediate. There the *ternary* is
right — see [clamp-ternary-keeps-literal-in-compare.md](clamp-ternary-keeps-literal-in-compare.md)
(`CMulti::AutoTuneCmdDelay` 97.87 -> 100 EXACT).

**2. The value's only consumer is a null test, and the override is a memory load.** cl then
**if-converts** the whole select into the branchless mask and spends a callee-saved register on
the destination — the opposite of the hoist. `CDDrawChildGroup::PruneOrphans` @0x15b1d0 is the
worked counter-example: retail is unambiguously the value form
(`test eax,eax / je L / mov eax,[found] / L: cmp eax,esi`), and cl will not emit it from any
spelling:

| spelling | % |
|---|---|
| `void* owner = 0; if (Lookup(k, found)) owner = found;` | 85.93 |
| `void* owner = Lookup(k, found) ? found : 0;` | 85.93 |
| explicit `if (Lookup(k, found)) owner = found; else owner = 0;` | 88.18 |
| `if (Lookup(k, found) == 0 \|\| found == 0) { ... }` — the WRONG shape | **93.75** |

All three value spellings produce `neg eax / sbb eax,eax / and eax,[found]`. Keep the `||` chain
and say so in the note. This is the boundary of
[map-lookup-ternary-ifconverts.md](map-lookup-ternary-ifconverts.md): its statement form beats the
*ternary*, but when the result feeds only a branch, the two-compare `||` beats both.

**Screen first:** apply when retail's low arm is a bare fall-through with no `jmp` of its own.
If retail has two real arms joined by a `jmp`, the `?:`/if-else form is already right.
