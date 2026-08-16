# REFUTED: `&= ~ENUMERATOR` does NOT complement at 16 bits - the truncation is always a width slip
tags: cpp:enum cpp:type cpp:struct | asm:and asm:or | topic:scoring-artifact topic:correctness
symptoms: and dword ptr [x],0xfffe, andl $0xfffe, 0x0000ffe0, andl $0xdf vs andb $0xdf, `&= ~FLAG`, GZ_ENUM_FLAGS_BEGIN, truncated masks
confidence: 10/10
variants: enum-domains.md, enum-switch-selector-lowers-signed.md

**The hypothesis, and why it is wrong.** It is natural to suspect that MSVC 5.0 gives an
enum whose enumerators all fit in 16 bits a 16-bit underlying type, so that `flags &=
~SPRITE_STATE_HIDDEN` would assemble to `and dword ptr [..], 0xfffe` and silently clear
bits 16..31 of the flag word on every clear (`|=` unaffected, which would explain why such
a bug survives every read-through). **Measured on cl 5.0 SP3 (11.00), it does not happen.**
cl promotes every enumerator to `int` before `~`, whatever the enum's range and whether or
not the enum is named:

```cpp
enum E_TINY   { T_ONE = 1 };        void p1(int* p) { *p &= ~T_ONE; }   // andl $-0x2
enum E_WORDHI { W_HI = 0x8000 };    void p3(int* p) { *p &= ~W_HI; }    // andb $0x7f, %ch
enum E_WORDMAX{ WM = 0xffff };      void p4(int* p) { *p &= ~WM; }      // andl $0xffff0000
enum         { U_ONE = 1 };         void p5(int* p) { *p &= ~U_ONE; }   // andl $-0x2
```

The tree agrees. `SpriteStateFlags m_stateFlags` is `typedef i32` (the retail branch of
`GZ_ENUM_FLAGS_END`), and every one of the 45 `&= ~SPRITE_STATE_*` sites already emits the
32-bit form that retail has - `src/Gruntz/Play.cpp` byte-for-byte:

```asm
83 60 40 fe        and    DWORD PTR [eax+0x40],0xfffffffe   ; base AND target
8b 50 40           mov    edx,DWORD PTR [eax+0x40]          ; wormhole, base AND target
83 e2 fe           and    edx,0xfffffffe
```

**So `static_cast<i32>(FLAG)` at a `~` site fixes nothing and should not be written.** It is
a cast against a defect that cannot occur, and the cast metrics are ratcheted to 0.

## What the symptom really means when you do see it

A 32-bit word masked with a narrow constant is real, and it is a **correctness** bug, but
the cause is a WIDTH or UNION-MEMBER slip in the source, never the enum. The signature is a
32-bit mask in the base against the byte/word form in the target:

```asm
base    83 e0 df           andl   $0xdf, %eax                 ; clears bit 5 AND bits 8..31
target  80 64 08 03 df     andb   $0xdf, 0x3(%eax,%ecx,1)     ; clears bit 5 of byte three
```

`CGrunt::ClaimSwitchTile` (0x00052c70) had exactly this. The cell array is a union of
`BrickzCell** m_rows` / `i32** m_rowInts` / `char** m_rowBytes`, and the body reached for
the `i32` member with BYTE arithmetic:

```cpp
gb->m_rowInts[oldTy][oldTx * 7 * 4 + 3] &= 0xdf;   // WRONG: i32 index 28*x+3
gb->m_rows[oldTy][oldTx].m_flagBytes[3] &= 0xdf;   // RIGHT: byte 3 of cell x
```

The `* 4` is the tell that the author meant bytes. Through `m_rowInts` the write landed at
byte offset `112*oldTx + 12` - a **different cell** - and clobbered its upper 24 bits.
60.30% -> 62.12% and the memory corruption is gone.

**Sieve:** `the immediate-mask sieve (retired) compares the base-vs-target multiset of
`and`/`or`/`xor`/`test` immediates per function, width-normalised (a sub-32-bit `and` extends
with ones, `or`/`xor`/`test` with zeros), so only genuine constant disagreements survive. The
narrow-complement subset is the `truncated masks` cleanliness metric, ratcheted at **0**.
Tree-wide 2026-08-08: 99 functions differ on some mask constant, 1 was truncated (the above),
now 0. objdiff cannot show this class - its diff masks operands.

## Site census (2026-08-08, `src/` + `include/`, comments and strings stripped)

602 `~` tokens: 345 destructors, 108 numeric literals, and 149 applied to a named constant -
96 `~TILE_MASK_PX`, 41 `~SPRITE_STATE_HIDDEN`, 4 `~SPRITE_STATE_FLASHING`, 3 already-cast
`~static_cast<u32>(bit)` in DinMgr2's key-latch macro, 2 `~bit` (a variable), 1
`~WS_EX_TOPMOST`, 1 `~rd->m_flags`, 1 the strict-branch `operator~` in `<Enums.h>`. **None is
a bug.** All four flag domains (`SpriteStateFlags`, `MapCellFlags`, `PidFlags`,
`RockNeighborMask`) and the `TileGeometry` constant bag have enumerators inside 16 bits, and
all of them complement correctly.

The one thing that looks like the defect and is not: `andl $0xffe0` in `play.obj`. That is
`(x & 0xffff) & ~TILE_MASK_PX` folded into one mask, and the target has the identical
instruction.
