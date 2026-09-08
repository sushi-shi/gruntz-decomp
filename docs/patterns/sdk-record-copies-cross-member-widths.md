# Unrolled SDK record copies cross member widths

tags: cpp:sdk cpp:aggregate | asm:mov | topic:verification topic:layout

Replacing the application GUID's word overlay with the real SDK `GUID` exposed
a false positive in the retail data-width audit. The previous union was opaque
to the checker. With the SDK declaration, the dword load at object offset four
was reported as an oversized read of the two-byte `Data2` member. That dword
actually copies both `Data2` and `Data3`.

Retail sites 0xb7464 and 0xb77dd each copy all sixteen bytes into a by-value stack
argument. They load the object at offsets 0, 4, 8, and 12, then store the
unchanged register values at matching offsets through one destination pointer.
The typed GUID initializer preserves the retail bytes. Reintroducing a union or
widening `Data2` would hide or create a model defect.

The width consumer now recognizes a complete decoded record copy. It requires
all dword source chunks of one complete, non-union, non-polymorphic record,
matching destination offsets, unchanged loaded values, no intervening control
flow, and no modification of the destination base. It suppresses only the
proved load instructions. It has no GUID-name or RVA exception. Existing
scalar-width, shortfall, split-object, and indexed-array checks remain active.

The controlled failure was the real `g_dplayAppGuid` finding at +4. The full
consumer tests require that a complete copy passes, while a missing final store,
a wrong destination offset, a modified loaded register (including a partial
register write), an overwritten destination, or an additional bare oversized
read still fails. The existing data-access category suite and its
whole-tree injected-defect controls also pass. The final retail data-access
gate reports zero new findings and zero accepted-model exceptions.

When an SDK type restoration breaks a width gate, inspect the complete copy
sequence before editing the type. A machine operand width is not necessarily a
source member width. A handful of same-width accesses alone is insufficient;
prove where the unchanged bytes are written and test the gate's actual consumer.
