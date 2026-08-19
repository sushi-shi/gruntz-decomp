# Deferred switch outputs let duplicated case bodies fold

tags: cpp:switch cpp:local cpp:branch | asm:jmp asm:mov | topic:codegen-idiom topic:correctness
symptoms: two retail jump tables target the same cardinal case bodies, but a reconstruction
either duplicates every body and its data relocations or inserts small value-selection arms
before a second dispatch
confidence: 9/10

## Signal

An outer command switch and a nested direction switch may author the same cardinal
movement bodies twice. Retail can still contain only one copy: both jump tables have
DIR32 entries naming the same four body offsets. That is stronger evidence than a
matching relocation count because it proves the sharing happens at the case-body level.

The incoming lifetime of the output variables decides whether cl 5.0 can perform this
fold. Initialising the output pair before the outer switch makes those values live into
every arm and prevents the duplicated bodies from becoming identical. Leave the outputs
uninitialised at declaration and assign them in every case, including the explicit
default paths proved by retail:

```cpp
i32 nextX;
i32 nextY;

switch (command) {
    case COMMAND_NORTH:
        nextX = x;
        nextY = y - step;
        cell = northCell;
        break;
    case COMMAND_CURRENT:
        switch (direction) {
            case DIR_NORTH:
                nextX = x;
                nextY = y - step;
                cell = northCell;
                break;
            default:
                nextX = x;
                nextY = y;
                break;
        }
        break;
    default:
        nextX = x;
        nextY = y;
        break;
}
```

This is not an uninitialised-value trick. Every path reaching a use assigns both outputs;
the default block is part of the recovered semantics. The retail default entries target
the two-move fallback block, independently proving those assignments.

## Calibration

`CGrunt::StepCompassMove` has an outer tile-command table and an inner current-direction
table. Retail points both tables' north/east/south/west entries at the same movement
bodies and references the nine `g_gruntMoveDir*` fields 151 times.

- Duplicated nested bodies with entry-initialised outputs emitted 163 relocations.
- Replacing the nested source with an intermediate `GruntDirection` emitted 151
  relocations and 129 branches, but generated four value-setting arms followed by a
  second dispatch that retail does not contain.
- Direct source `goto` sharing also emitted 151 relocations but collapsed the CFG to 125
  branches.
- Keeping the nested switches and deferring the output pair made cl fold the duplicated
  cardinal bodies. Both jump tables then target the same arms and the relocation count is
  retail's 151. The function moved 57.62% to 62.98%; the first retained state was one
  branch short because later collision-search tails factored differently.
- A fresh 462-variant structural/state campaign found ten instruction islands. Its only
  higher source shapes reversed the two independent output assignments in one fixed
  cardinal arm. Writing the north arm's `nextY` before `nextX` restored retail's complete
  129-branch skeleton and moved the live report to 63.30%; calls (22), returns (2), and
  relocations (151) also agree. The remaining 860-versus-884 instruction residue is
  register/scheduling closure rather than a missing arrow branch.

Two negative lifetime controls isolate the mechanism. Deferring the pair while keeping
the intermediate direction stage produced 133 branches; deferring it only across the
arrow block produced 131. Neither represents retail's nested switch topology.

## Reverse use

When retail jump-table relocations alias two semantic case sets, reconstruct both source
switches first. If cl duplicates the bodies, inspect whether an eager output initializer
is carrying a value into the arms that retail's explicit defaults assign locally. Do not
replace the switches with labels, a helper, a volatile carrier, or an invented selector
solely to force sharing.
