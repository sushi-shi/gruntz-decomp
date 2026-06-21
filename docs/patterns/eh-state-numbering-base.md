# Multi-member ctor/dtor: EH funcinfo state-numbering base shifts when the TU's fn-set differs
tags: cpp:ctor cpp:eh | asm:mov | topic:wall topic:eh
symptoms: body+offsets+EH-frame byte-identical, residue only in __ehfuncinfo state ids / the `[ebp-N]=state` writes, ~89-95%
confidence: 7/10

A ctor (or dtor) with several DESTRUCTIBLE sub-objects emits a `/GX` EH frame whose state-table
entries are NUMBERED relative to the translation unit's funcinfo. When our `.cpp` holds a
different SET of functions than the retail TU, that numbering base shifts (e.g. retail tags the
array ctors 0/1/2; ours uses the -1 entry-state for the first, then 0/1) — the function BODY,
member offsets and EH-frame structure are byte-identical, only the state ids differ. This is a
symptom of an INCOMPLETE TU (see tu-completeness-rva-order); it resolves when the TU is complete.

WALL until the TU is complete. Evidence: CGameLevel ctor 89.5%, CGruntzMgr ctor (deferred), the CUserBase trigger ctors 95%.
