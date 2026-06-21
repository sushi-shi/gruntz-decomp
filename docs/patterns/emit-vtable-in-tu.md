# Emit a class's `??_7` vtable in your TU by declaring ALL its virtual slots (stubs for unmatched) so the ctor's vptr store reloc-masks
tags: cpp:ctor cpp:virtual | asm:mov | topic:codegen-idiom
symptoms: the ctor's `mov [esi], OFFSET ??_7Class@@6B@` has no symbol to bind to / wrong vtable; only some slots matched
confidence: 8/10

To make `cl` materialize `??_7Class@@6B@` in your object (so the ctor's `mov [this], OFFSET
vftable` reloc-masks against it), declare the class with ALL its virtual slots in the binary's
order — the matched ones as real methods, the rest as out-of-line empty stubs (NOT added to
symbol_names.csv). cl then emits the vtable in-TU and the vptr store binds locally. Getting the
full slot ORDER right also flips overridden-method manglings `Q`→`U` and makes vtable-slot
indirect calls land correctly (dummy-virtual-slots.md).

STEERABLE. Evidence: CMapMgr 6-slot vftable emitted in-TU (slot0 Reset matched, 1-5 stubs);
CGameApp full 16-slot vtable in tomalla order flipped `?CloseResources@…@@QAE` → `…@@UAE` (update
symbol_names.csv to the `U` form; bodies are byte-identical virtual-vs-nonvirtual so greens held);
the CGameWnd idiom. related: dummy-virtual-slots.md, inline-vdtor-for-scalar-deleting.md.
