# A `static` helper that every switch arm ends with is OUR invention — expand it

tags: cpp:switch cpp:inline cpp:call | asm:call asm:jmp | topic:codegen-idiom
symptoms: every arm calls one small `static` free function; retail calls the
callee INSIDE that helper directly, N times instead of once; base is short by
~15% and the reloc-sequence diff shows `- call <Helper>` opposite
`+ call <the thing the helper calls>`
confidence: 10/10

A factory/loader `switch` whose arms all finish with the same three or four
statements is tempting to fold into

```cpp
static void* RegSwitchTail(Container* self, Logic* obj, ...) {   // NO
    if (obj->SerializeDispatch(...) == 0) return 0;
    obj->m_owner  = self;
    obj->m_typeId = id;
    return obj;
}
...
case TRIGID_MULTI_SWITCH_3: {
    Logic* obj = new CTileMultiTriggerSwitchLogic;
    return RegSwitchTail(this, obj, reader, typeId, pObj, id);
}
```

Retail never had it. Under `/O2` (`/Ob1`) a plain `static` function is **not**
auto-inlined, so every arm pays a `call` and the tail's own callees vanish from
the arm. The tell is a matched pair in `insn_seq --seq`: one `- call <Helper>`
per arm against `+ call <Validate…>` per arm on the target side, with the arm's
`operator new` + constructor rows matching either side of it.

**Fix: expand the helper at every call site and delete it.** Retail writes the
tail out in each arm and lets cl's *tail merging* share the identical copies —
which is not the same thing as a call, because merging keeps each arm's `new`
and constructor inline and only folds the common suffix.

`marking the helper `static __inline` is NOT the fix` — measured on the same
function it took the score from 71.27 to **0.00**.

## Evidence

`CTileTriggerContainer::DeserializeLogic` (0x00117800), twelve arms behind two such
helpers (`RegSwitchTail`, `RegLogicTail`): retail has four
`SerializeDispatch@CTileTriggerSwitchLogic` calls where we had twelve
`call Reg*Tail`. Expanding both and deleting them: **71.27 → 89.75** in one
build.

Companion in the same function: the jump table (`sema disasm --switch`) proved
`case 1 / case 2 / case 5` are THREE separate arm targets, not the one grouped
arm we had — each stores its own constant into a frame slot before falling into
the shared constructor+tail. Splitting them into value-ordered arms is a further
+0.7. cl re-merges the three anyway, because our switch selector is
address-taken (`reader->Read(&id, sizeof(id))`) and cannot be const-propagated
into the arms the way retail's is; that is the residue.

variants: [error-report-guard-falls-through-to-a-shared-return.md](error-report-guard-falls-through-to-a-shared-return.md),
[switch-arm-layout-order-is-source-order.md](switch-arm-layout-order-is-source-order.md)
