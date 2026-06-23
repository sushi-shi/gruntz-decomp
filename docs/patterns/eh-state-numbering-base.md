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

CAVEAT — "complete the TU" is not always reachable, and the funcinfo is PER-FUNCTION
(its own COMDAT), so adding sibling EH ctors to the same .cpp does NOT renumber another
ctor's states. The CProjectile no-arg ctor (0x126e0, 99.05%) residue is its OWN funcinfo:
the EH prologue pushes `funcinfo+0x0` vs retail's `funcinfo+0xe`, and the CObList state id
is `mov [esp+0x18],1` vs retail `2`. Its sibling out-of-line `CMovingLogic::CMovingLogic()`
(0x13940) CANNOT be emitted from the projectile TU: the no-arg ctor INLINES CMovingLogic()
(it must stay `inline` in the header), and there is no out-of-line CMovingLogic caller in
this TU — making the ctor non-inline to force the standalone drops the no-arg ctor 99.05%
→ 19.7% (it then `call`s instead of inlining; the standalone alone matches 98.4%). The
1-arg CProjectile ctor (0xdec60) does NOT call CMovingLogic() either — it chains
`CUserLogic(obj)` (0x58cd0) out-of-line then builds the +0x38 motion band via a separate
field-init helper (0x136d0), so it would not force 0x13940 either. So 0x13940 belongs to
whatever TU directly constructs a standalone CMovingLogic, and the no-arg ctor's funcinfo
wall is NOT resolved by completing the projectile family. Leave it at 99.05%.
