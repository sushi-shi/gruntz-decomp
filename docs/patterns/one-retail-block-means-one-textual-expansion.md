# One shared retail block under two guards means ONE textual expansion, not two
tags: cpp:macro cpp:goto cpp:inline | asm:jmp asm:x87 | topic:codegen-idiom
symptoms: two mutually exclusive guards whose bodies are the same computation;
retail reaches ONE block from both arms (`jmp` from the first, fallthrough from
the second) while ours emits two byte-identical copies; `walls diagnose` reports
extra base instructions with the same call/branch counts
confidence: 7/10

Under `/O2` (`/Ot`) cl 5.0 does NOT cross-jump two byte-identical copies of a
macro/inline expansion - the merge that still runs is the global optimizer's
value-based factoring, and it does not fire on two separate expansions. So when
retail shows ONE block reached from both arms, the era source wrote the update
ONCE. In a macro the collision-free way to skip it is a nested `do { } while (0)`
with `break` (a label would collide across expansions).

```cpp
    do {
        double c;
        if (t > vmax) { c = vmax; }
        else if (t < -vmax) { c = -vmax; }
        else { break; }        /* no clamp: leave the inner do/while */
        scr = c;
        ARRIVAL_V(v, a, s, c + s);   /* written ONCE for both arms */
    } while (0);
```
```asm
  16ed31: fstp st(0)
  16ed33: fld  QWORD PTR [ecx+0xd8]
  16ed39: jmp  0x16ed56          ; arm 1 jumps into the shared block
  16ed3b: fld  QWORD PTR [ecx+0xd8]
  16ed41: fchs                   ; arm 2 falls through into it
  ...
  16ed56: <the single arrival-velocity block>
```
STEERABLE. `CMotionState::Step` 0x16ecd0: two `ARRIVAL_V(v, a, s, c + s)` call
sites in the `t > vmax` / `t < -vmax` arms produced two copies; folding them to
one textual site reproduced retail's block layout byte-for-byte from 0x67 to
0x86 and moved 81.28 -> 81.80. Composing a `double target = c + s;` hoist on top
of it (retail computes `fadd [ecx+0x40]` above the `a == 0` fcomp) re-split the
block and cost 2.7 - the hoist is real but is not reachable while the two
spellings interact.
