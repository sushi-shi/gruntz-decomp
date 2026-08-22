# Function-scope locals used in disjoint `switch` arms each keep their OWN slot — scope them to the arm

tags: cpp:local cpp:switch cpp:scope | asm:sub-esp asm:mov | topic:codegen-idiom topic:regalloc
symptoms: `sub esp,N` is exactly 4 (or 8) bytes larger than retail's, every
`[esp+K]` in the body is shifted by that amount, and the two sides otherwise
agree instruction-for-instruction; `walls semdiff` reports identical `disp`,
`store`, `imm` and mnemonic multisets while the score sits in the low 90s
confidence: 9/10 (broken with objdiff on CSBI_StatzTabGruntBar::SerializeFields 0xea990, 94.03 -> EXACT)
variants: temp-slot-overlay-reveals-scope-structure.md

cl 5.0 overlays two stack locals only when their SCOPES are disjoint, not when
their live ranges are. A `switch` whose arms are mutually exclusive therefore
does *not* let a function-scope temp share a slot with a temp from another arm —
declare each arm's temps inside the arm and the frame collapses to retail's.

```cpp
// NO - v, out and idx each get their own dword; frame 0x90
char buf[SERIAL_NAME_LEN];
CObject* out;
i32 idx;
i32 v;
switch (mode) {
    case SERIAL_SAVE: /* uses buf, v      */ break;
    case SERIAL_LOAD: /* uses buf, out, idx */ break;
}

// YES - v overlays the out/idx pair; frame 0x8c, retail's
char buf[SERIAL_NAME_LEN];
switch (mode) {
    case SERIAL_SAVE: {
        i32 v;
        ...
        break;
    }
    case SERIAL_LOAD: {
        CObject* out;
        i32 idx;
        ...
        break;
    }
}
```
```asm
sub esp,0x8c            ; retail: buf(0x80) + reg-spill + max(save-arm, load-arm)
mov DWORD PTR [esp+0x14],eax
lea eax,[esp+0x20]      ; buf, at a push depth of 4
```

The general rule is the one
[temp-slot-overlay-reveals-scope-structure](temp-slot-overlay-reveals-scope-structure.md)
states — cl 5.0 assigns slots by SCOPE, siblings overlay — read from the other
direction: a `switch` is the commonest place where mutually exclusive code is
written at ONE scope, so it is where the frame silently grows.

Steerable, and the whole score is in it: the shifted displacements make a
byte-identical body read as a 900-line diff. Reach for this whenever the frame
delta is one slot and `semdiff` says every multiset agrees — the arms' temps are
what to look at first, and macro-expanded arm bodies (which force a shared
declaration) are where it hides.
