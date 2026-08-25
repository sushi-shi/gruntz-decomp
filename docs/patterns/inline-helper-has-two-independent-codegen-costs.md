# An inline helper has two independent costs: its caller boundary and its declaration

- **confidence** c9
- **tags** `cpp:inline` `cpp:macro` `cpp:include` | `asm:mov` `asm:push` | `topic:regalloc` `topic:front-end-state` `topic:codegen-idiom`

## Symptom

A repeated block is folded into a real inline helper. Either the edited caller changes
register allocation despite an equal call set and CFG, or an unedited later function in
the TU changes. These are separate mechanisms and need separate fallbacks.

## Mechanism A: the inline-body boundary changes the caller

Inlining does not make the helper body identical to hand-written caller statements in
VC5's front end. The inlined IL retains ownership and lifetime information which can
recolor a caller even when the emitted function has the same size, instruction count,
call set, branch skeleton, and referents.

`CImageSet3::Parse` is the controlled example. Its shared parser prefix was:

```cpp
i32* p = &record->m_width;
m_width = *p++;
m_height = *p++;
```

A free inline returning `p`, and then an implicit-`this` member inline, both left the
target and base at 0x8d bytes, 59 instructions, one call, four branches, and three
returns. `gruntz walls diagnose 0x166d70` classified the first divergence at +0x5 as
REGALLOC/SCHEDULING. The score nevertheless moved from exact to 70.3051%. A
token-preserving `READ_TILE_IMAGE_DIMENSIONS(record, p)` macro restored exact.

## Mechanism B: the declaration changes later TU state

Even if every helper call expands acceptably, adding the declaration can recolor
unrelated later functions. Moving the definition to a narrow header does not help if a
new member declaration still lives in a widely included class header.

Two measured controls:

- Adding `SoundCueRegistry::TickSoundVolumeRamps` to the class header rebuilt roughly half
  the program and produced eleven fresh regressions (3,737 exact, 95.11%). Removing the
  member declaration and defining `TickSoundVolumeRamps(SoundCueRegistry*)` only in the eight
  caller TUs restored 3,741 exact and zero fresh regressions while retaining all thirteen
  conversions.
- A member `CTileImageSet::ReadDimensions` declaration produced thirteen fresh
  regressions, including exact renderers and accessors in unrelated TUs. Removing the
  declaration restored them; the caller-local regalloc cost above remained, proving the
  two mechanisms are independent.

The four-site status-bar initializer supplies a smaller declaration-state control. A
narrow real inline left the edited setup bodies usable but moved the unedited exact
`CSBI_ImageSetAni::Render` to 99.8667%. Replacing the helper with
`INITIALIZE_STATUS_BAR_ITEM` restored the later renderer and the MAX gate.

## Detection and reverse-use rule

After a real-inline A/B, compare two populations:

1. If only converted callers move and `walls diagnose` reports equal calls and CFG,
   treat the helper boundary as a caller regalloc lever. Try authentic parameter/return
   shapes; if none reaches retail, retain a token-preserving macro.
2. If unedited later functions move, treat the helper declaration or definition boundary
   as TU state. First move a free helper to the narrowest real owner header. If a class
   declaration is still required and the gate remains red, use a narrow free helper or
   macro instead of retaining an inert declaration.

Do not add or retain unused declarations to steer either mechanism. A macro fallback is
valid only when it names a repeated semantic operation and expands to the caller's
evidence-backed source shape.
