# A call result can coalesce onto dead `this` and remove one callee-save

tags: cpp:local cpp:return cpp:method | asm:push asm:mov asm:pop | topic:regalloc topic:wall
symptoms: calls, branches, returns, and relocations agree; the recompile is two bytes
shorter because it omits `push ebp`/`pop ebp`; retail keeps one call-crossing result in
EBP while the recompile reuses the now-dead `this` register for a later boolean
confidence: 9/10 for recognizing the residue, 3/10 for steering it from source

`CMulti::CreateHostSessionAndPlayer` (`0x000b8b10`) is the calibration case. Both sides
emit 11 calls, four branches, one return, and 22 relocations. Retail is 113
instructions/`0x175` bytes and the recompile is 111 instructions/`0x173` bytes.

The values account for the whole delta:

| role | retail | recompile |
|---|---|---|
| `this` | ESI | ESI |
| group-enumeration result, live across later calls | EBP | EBX |
| player color, live across `GetName` | EDI | EDI |
| `RegisterLocalPlayer(...) == 0` | EBX | ESI after `this` dies |

Retail therefore saves EBP; the recompile coalesces the final failure flag onto ESI
immediately after its last `this` use and needs no EBP. The remaining instructions are
the same computation with those roles recolored.

Two source controls bound the local levers:

- declaring `failed` at function entry and assigning it at the call is byte-identical;
- copying `enumResult` into a result local and conditionally nulling it changes the
  skeleton to five branches and 108 instructions, moving away from retail.

Detection rule: first verify equal calls/CFG/relocations, then map the apparently extra
callee-save to a real value. If the shorter side merely coalesces a post-call result
onto a dead receiver and declaration splitting is flat, do not invent a live local to
force the extra save. Park it as allocation closure. This is distinct from an invented
member-pointer local, where the recompile has the extra save and removing the local
improves both the model and the code.
