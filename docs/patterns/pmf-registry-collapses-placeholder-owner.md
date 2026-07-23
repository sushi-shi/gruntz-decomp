# A PMF registry plus receiver offsets collapses a placeholder method owner
tags: cpp:class cpp:member cpp:pmf | asm:call | topic:identity topic:mis-model
symptoms: several handlers were placed on a layout-padded placeholder class because their
  bodies access high offsets; the handlers are cast into one real class's member-function
  registry and every accessed offset names an existing member of that class
confidence: 10/10

A method body that only exposes `this+N` offsets does not prove a new class.
Before creating or retaining a padded "view" class, inspect how the method
address is stored and how it is invoked. A registry whose dispatcher evaluates
`(realThis->*entry)()` is strong owner evidence: every registered body receives
that concrete object, even when the PMF storage type is declared on a
single-inheritance base and a placeholder cast temporarily made the source
compile.

Use four independent checks:

1. Follow the registration store and dispatch load. Recover both the
   member-pointer representation/type and the concrete receiver passed at the
   indirect call; they need not name the same class.
2. Map every `this+N` access in the candidate bodies onto the real class. The
   mapping must be complete, including nested pointers and timer pairs; a few
   coincident offsets are not enough.
3. Resolve direct and ILT-mediated callees. Existing methods on the candidate
   real class are additional identity evidence; fake same-offset helper names
   are not.
4. Search RTTI, vtables, constructors, allocation sites, and storage for the
   placeholder. If none exist, do not preserve it merely because the padded
   layout compiled.

For the Grunt action registry at `0x644af0`, `CGrunt::FireActivation` invokes
each entry on its own receiver. The current reconstructed entry type is
`i32 (CGrunt::*)()`; retail RTTI suggests the storage abstraction may ultimately
be a `CUserLogic` PMF instead, but neither model supplies a separate leaf
receiver. The former `CGruntBehaviorLeaf` handlers map completely onto
`CGrunt`: `+0x154` is `m_38`, `+0x170` is `m_entranceReason`, `+0x1c0` is
`m_animSetName`, `+0x260` is `m_tileMgr`, `+0x360` is `m_deathType`,
`+0x830/+0x838` are the idle timer/window pairs, and `+0x860/+0x868` are the
attack timer/downtime pairs. Their three supposed leaf helpers resolve through
the ILT to `CGrunt::CreateStaminaSprite`,
`CGrunt::LoadGruntAbilityTuning`, and
`CGrunt::ResetEntranceAnimation`. No independent leaf RTTI, vtable,
constructor, allocation, or storage exists. The placeholder class was
therefore deleted and the three bodies became ordinary `CGrunt` methods.

The reverse audit is useful: search declared-only method owners with no class
evidence, then group their call sites by a typed PMF table or a stable receiver.
When all offsets and callees fit one existing class, fold the placeholder
instead of adding more padding, casts, or fake helper declarations.
