# The vptr stamp transposed with the ctor body's FIRST member load — a scheduler wall, not a mem-init-list bug

tags: cpp:ctor cpp:vtable cpp:member | asm:mov | topic:wall topic:regalloc
symptoms: a `CUserLogic`/`CWapX`-derived ctor at 95-99.6% whose ONLY code difference is that retail has `mov [esi],??_7Class@@6B@` immediately after the last base-ctor store and ours has it one instruction later, after the body's first `mov r,[esi+0x38]`; instruction counts agree exactly
confidence: 9/10

This looks exactly like [vptr-stamp-splits-meminit-from-body.md](vptr-stamp-splits-meminit-from-body.md)
— the stamp is the divider between the member-init list and the body — but it is
NOT that bug. The divider is in the *same place relative to the stores* on both
sides. What moved is the body's first LOAD, hoisted one slot by the scheduler.

**The control that proves it is a scheduler decision, not a source shape:**
`CTileTriggerSwitch::CTileTriggerSwitch` (0x0010dc40) is **100% EXACT** and its
base obj hoists the body's first load above the stamp too —

```asm
mov  [esi+0x3c],eax        ; last CWapX base-ctor store
mov  ecx,[esi+0x14]        ; body's first load, HOISTED past the stamp
mov  DWORD PTR [esi],??_7CTileTriggerSwitch@@6B@
```

so cl5 hoisting past the stamp is normal and retail agrees with it. The split is
**which member the body reads first**: the ctors that match read `[esi+0x14]`
(`m_objAux`, CUserLogic's own member); every ctor that misses reads `[esi+0x38]`
(`m_wwdObject`, the SECOND base CWapX's member, which the inlined base ctor
stored four instructions earlier). Retail's scheduler declines to move that load
closer to its producing store; ours moves it.

Exhausted without a lever:
* every body spelling — named local for the receiver, explicit read-modify-write,
  `this->`, a local for the flag word, `(*m_wwdObject)`;
* the wall-breaker, `match_variants --state-trials 48 --max-depth 2 --limit 320`
  on `??0CSingleAnimation` — 320 variants, **all flat at 99.2381%**, and the AST
  engine reports zero legal atomic mutations in the body (commutative_order=0,
  independent_statement_order=0, declaration_split=0, ...), so only TU state was
  actually searched and TU state does not move it.

⇒ `@early-stop`. Affects at least CSingleAnimation 0xae7f0, CTileTriggerTransition
0x10faf0, CBoomerang 0xe0650, CGruntToySprite 0x7f350, CGruntHealthSprite 0x7eb00,
CCursorSnapSprite 0x3a340, CGruntStartingPoint 0x3df30, CKitchenSlime 0xb23a0,
CPathHazard 0xb35a0, CDoNothing 0x1d5d0.

**Do not confuse it with a real bug**: check the instruction counts first
([instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md)).
These all match exactly; a ctor in the same family whose count does NOT match has
a source defect that is worth fixing.

related: [vptr-stamp-splits-meminit-from-body.md](vptr-stamp-splits-meminit-from-body.md),
[eh-ctor-vptr-restamp-position.md](eh-ctor-vptr-restamp-position.md)
