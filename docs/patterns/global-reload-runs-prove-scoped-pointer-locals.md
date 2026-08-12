# Global reload runs prove scoped pointer locals

tags: cpp:local cpp:pointer cpp:scope | asm:mov asm:idiv | topic:codegen-idiom topic:correctness
symptoms: repeated member chains through one global produce more DIR32 relocations than retail; retail reloads the global once per logical region or branch arm; the candidate frame is one dword too small
confidence: 9/10

When retail repeatedly accesses `g->member->...`, count the relocations to `g`
before treating different register choices as noise. A source local caches the
pointer value and turns a run of member chains into one global load. Its scope is
recoverable from where retail reloads the global: a reload after a call or at the
start of each sibling arm names a new local lifetime.

`CGrunt::StepArrivalDrop` at `0x4b370` was the calibration case. Its candidate
referenced `g_gameReg` ten times while retail referenced it seven times. Three
scoped `CMapMgr*` entities explain the exact difference:

- one nudge-region local spans the rock-neighbour reads, 3x3 save, `SearchEdge`,
  and 3x3 restore, reducing four candidate loads to retail's two;
- one local in each Bresenham arm is initialized immediately after that arm's
  `idiv`, preserving retail's separate two loads instead of letting cl hoist one
  common load above the branch.

The reconstruction changed the frame from `0x4c` to retail's `0x50`, reduced the
global relocation count from 10 to exactly 7, and reduced the function from 2908
to 2856 bytes against retail's 2864. Declaring both Bresenham locals before their
division was a negative control: cl common-hoisted them, emitted only six global
relocations, and contradicted retail. Thus the relocation positions prove both
the pointer entities and their scopes; the frame agreement is corroboration, not
the sole argument.

Use `gruntz audit reloc_multiset <unit>` for the count and the unmasked base/target
disassemblies for the positions. Do not create one function-scope cache merely to
make the total smaller: it can erase reloads that retail retains and incorrectly
claim the global is stable across calls.
