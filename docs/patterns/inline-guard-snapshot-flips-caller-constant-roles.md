# An inline guard snapshot can flip the caller's zero/one register roles

tags: cpp:inline cpp:local cpp:bool cpp:branch | asm:mov asm:xor asm:cmp | topic:codegen-idiom topic:regalloc
symptoms: an inlined process-global guard has the same call set, CFG semantics, stores and referents as retail, but one side pins zero in a callee-saved register before an EH temporary while retail pins one and reuses it for the unwind state, guarded store and later body fields
confidence: 9/10

`CGruntPuddle::CGruntPuddle` at `0x040490` exposed two globally different
constant lifetimes. The direct guard

```cpp
if (!g_logicTypesRegistered) {
    BuildLogicTypeTable(m_logicObject);
    g_logicTypesRegistered = true;
}
```

made VC5 keep zero in EBX from the first base construction onward. Retail
instead loads the global into EAX, creates zero in EBP after destroying the
temporary, and keeps one in EBX from the first unwind-state store through the
guarded global assignment, `Hide`, and `m_pending`.

Preserving the global read as a named snapshot in the shared inline owner is
the source lever:

```cpp
b32 registered = g_logicTypesRegistered;
if (!registered) {
    BuildLogicTypeTable(m_logicObject);
    g_logicTypesRegistered = true;
}
```

The constructor moved from **56.1653% to 97.6942%**. Its instruction, call,
branch, return, relocation, store, frame and five-action unwind topology then
all agreed with retail: 123 instructions, 11 calls, two branches, one return
and 25 ordered referents. The formerly duplicated guard arm and its extra jump
disappeared. The only residual is the independently recurring tile-snap
register colour: retail's two EAX results use `and al,0xe0`; the base uses
full-width `and r32,-0x20`, making the body two bytes longer.

Controlled negatives:

- `const b32`, split declaration/assignment, `true` versus `1`, and moving the
  inline definition were byte-identical to the snapshot result;
- a scalar inline snap helper, an owning-class helper, reference and by-value
  accessors, pointer/value locals, one reused result, per-coordinate macros,
  commuted addition, and shift-pair spelling all retained 97.6942%;
- the existing two-value snap macro reached 94.9422% and produced one
  `and al,0xe0`, but it computed both coordinates before the stores instead of
  retail's interleaved X-then-Y sequence; split in-place mask/add fell to
  90.4463%. Neither is a source fix for the two-byte residue.

This is a shared-header front-end change, so it can rotate unrelated functions
in every including TU even though its machine expansion is local to logic
constructors. The full-build A/B created ten fresh current-score dips; all ten
retained their historical MAX and were banked with the 97.6942 improvement.
That movement is not a reason to move the snapshot into one caller: doing so
would turn a real helper-local lifetime into a per-TU codegen device.

Reverse-use heuristic: when retail explicitly loads a guarded global into a
register while the base compares memory against a long-lived zero, and a
later true/false role is reversed across the caller, test a typed snapshot in
the authentic shared inline guard. Require the complete call set, unwind
actions and guarded-store placement to agree first. Keep the snapshot only as
a family-wide semantic local; never add an unused carrier or a caller-only
duplicate to steer allocation.
