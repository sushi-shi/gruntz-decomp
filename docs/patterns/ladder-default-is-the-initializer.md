# A constant ladder's default arm is the INITIALIZER, not a trailing `else`

**Tags:** `cpp:branch` `cpp:if` `cpp:local` | `asm:setcc` `asm:jmp` `asm:cmp` |
`topic:codegen-idiom` `topic:layout`
**Confidence:** 10/10 (A/B'd both directions on the same function)

## Symptom

An `if / else if / … / else` chain that assigns a different **constant** to one
local per arm, and:

- the tail of the chain in our compile is `xor r,r / cmp / setg r8 / add r,K`
  where retail has two plain `mov r,K` stores separated by a `jmp`;
- a range test on the result **downstream** of the join (`cmp eax,0x22 / jl`)
  is present in retail and **absent** in ours;
- retail keeps `cmp eax,0x14` / `cmp eax,0x3` after the join even though no arm
  can produce those values, while ours has threaded them away and each arm
  branches straight to its destination;
- our arms 2..N are **sunk to the end of the function** while retail emits the
  whole ladder inline, each arm ending in its own `jmp` to the join;
- and — the expensive consequence — the register allocator spills `this` to a
  pushed stack slot where retail keeps it in a callee-saved register.

## Mechanism

Written with a trailing `else`, the last two arms are `if (c) x = A; else x =
A+1;`. cl 5.0 lowers **adjacent** constants to `setcc` + `add`, and that gives
it a VALUE RANGE for the joined variable. With a range in hand it then folds the
downstream range guard, constant-propagates each arm past the join's equality
tests (jump threading), and — because the join now has a single hot predecessor
— re-lays the chain out with the join adjacent to arm 1 and everything else
sunk. The layout change is what moves the register allocator.

Written as an initializer with **no** trailing `else`, the default is a single
store cl cannot pair with anything, so none of that fires: the ladder stays
inline, every arm jumps to a real join, the guard survives, and the allocation
follows retail's.

## Steer

```cpp
// NO - trailing else
PickupType mode;
if (roll <= m_bombzPct)        { mode = PICKUP_BOMB; }
else if (roll <= m_welderzPct) { mode = PICKUP_WELDER; }
else                           { mode = PICKUP_WINGZ; }

// YES - the default is the initializer
PickupType mode = PICKUP_WINGZ;
if (roll <= m_bombzPct)        { mode = PICKUP_BOMB; }
else if (roll <= m_welderzPct) { mode = PICKUP_WELDER; }
```

## Evidence

`CBattlezMapConfig::ChooseIdleBehavior` 0x2f620, a 21-arm tool ladder and a
4-arm brick ladder: **53.62 -> 94.54**. Putting the trailing `else` back on the
tool ladder alone returns it to **53.26**, so the two forms are not
interchangeable and the effect is the ladder's, not the surrounding code's.

The prologue is the tell that the effect reaches the allocator: retail
`push ebx/ebp/esi/edi; mov edi,[esp+0x14]; mov ebp,ecx`, ours before the change
`push ecx; push ebx; push ebp; mov ebp,[esp+0x10]; push esi; push edi;
mov [esp+0x10],ecx` — one extra push, one extra pop per exit, and every member
access going through a reloaded `this`. After the change ours matches, including
retail's per-site `mov edx,[esp+0x14]` reloads of the parameter.

## Not this pattern

A ladder whose last pair retail really does emit as `setcc` + `add` — the same
function's 9-arm toy ladder ends `mode = roll > m_squeakToyzPct ? PICKUP_YOYO :
PICKUP_SQUEAKTOY;` and retail emits `xor eax,eax / cmp / setg al / add eax,0x1f`.
Read the tail of the target's chain before converting: two `mov r,K` separated by
a `jmp` means initializer, `setcc`+`add` means ternary.
