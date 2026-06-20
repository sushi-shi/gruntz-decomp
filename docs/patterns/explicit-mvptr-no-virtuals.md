# A class that models its vptr as an explicit `m_vptr` field must NOT also get virtual declarations — that inserts a compiler vptr and shifts every offset by 4
tags: cpp:member cpp:virtual | asm:mov | topic:codegen-idiom topic:tu-layout
symptoms: adding `virtual` to a reconstructed owner class regresses already-exact leaves; member offsets shift by 4 (m_0c→+0x10, m_10→+0x14)
confidence: 8/10

If a reconstructed class already models its vtable pointer as an explicit data field (`void*
m_vptr;` @+0), do NOT also put `virtual` declarations on it — `cl` then inserts ITS OWN compiler
vptr @+0 and pushes every modeled member down by 4 (`m_0c`→+0x10, `m_10`→+0x14), regressing exact
leaves. For a single owner-vtable call, use a separate vtable-VIEW helper cast (a small
polymorphic struct you cast `this` to) and keep the data-layout class NON-polymorphic. Conversely,
do NOT declare an explicit `void* m_vtbl` member on a class that DOES use `virtual ~T()` — the
implicit vptr already occupies +0, so an explicit member shifts every field by 4.

STEERABLE (modeling choice). Evidence: UnknownSeverus owner class — adding virtuals shifted
m_0c→+0x10 and regressed leaves; CFileIO — an explicit `void* m_vtbl` would shift every field
(use `virtual ~CFileIO` only). related: dummy-virtual-slots.md (the OPPOSITE: when you DO want the
implicit vptr + slots), tu-completeness-rva-order.md.
