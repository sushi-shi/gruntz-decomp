# Multiply-derived `new`: model the DERIVED ctor INLINE so the two vtable stores land at the new-site
tags: cpp:ctor cpp:inline | asm:mov | topic:codegen-idiom
symptoms: target does `op_new(N); ctor(args); mov [node],vtblA; mov [node+8],vtblB` with TWO vtable stores INLINE at the new-site
confidence: 7/10

When a `new`d engine class is multiply-derived, the target emits the base ctor `call` then the
two vtable-pointer stores INLINE at the new-site (`mov [node],&vtblA; mov [node+8],&vtblB`).
Reproduce by modeling `class Derived : public Base` where the BASE ctor is external/no-body
(reloc-masked) and the DERIVED ctor is INLINE doing `m_vtblA = &g_vtblA; m_vtblB = &g_vtblB;` —
`new Derived(...)` then emits op_new + null-check + base-ctor call + the two inline DIR32 vtable
stores, exactly. (An external/no-body single ctor puts NO inline vtable stores; an empty base
subobject is size-0 so the second vtable would stay @+0.)

STEERABLE. Evidence: CButeMgr::ParseTagLine `new CButeNode(desc,2)` — two vtables @+0/+8, base
ctor @0x16dff0 external, derived ctor inline. related: emit-vtable-in-tu.md, newd-class-real-size.md.
