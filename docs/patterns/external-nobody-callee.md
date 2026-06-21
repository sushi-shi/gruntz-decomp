# Declare an unmatched callee as an external no-body method/function so its `call` reloc-masks — pick the convention to kill stack cleanup
tags: cpp:method asm:call | topic:codegen-idiom topic:archetype
symptoms: need a reloc-masked `call rel32` / `FF15 [IAT]` to an engine fn you are NOT matching, without re-declaring or re-matching it
confidence: 9/10

The clean way to emit a reloc-masked call to an engine function you are not matching: declare it
as an **external, no-body** method (or `extern "C"` free function / `operator new`/`delete`).
`cl` emits a `call rel32` whose displacement objdiff masks (it shows `thunk_FUN_*` vs your
mangled name — benign). The CONVENTION you declare is load-bearing for stack cleanup:

- **`__thiscall` member** (receiver in ecx) → `lea ecx,[obj]; call` / `mov ecx,esi; call`, no
  caller cleanup. A collection iterator / engine method on an object MUST be thiscall.
- **`__stdcall`** → callee pops; NO `add esp,N` at the call site (use for dispatchers/callbacks
  the target lets the callee clean: a cdecl decl emits a spurious `add esp,0xc` → arg cascade).
- **`extern "C"` / `__cdecl`** → `push…; call; add esp,N`. Use the explicit `operator new(len)`
  / `operator delete(p)` *function* forms (`??2@YAPAXI@Z`/`??3@YAXPAX@Z`) to get the bare
  `push len; call; add esp,4` — plain `new`/`delete` adds a null-check/ctor.

Reusing a callee already byte-matched in ANOTHER unit is the same: just call it; the `call rel32`
reloc-masks across TUs (no re-declare). A polymorphic interface modeled as a struct whose first
member is a vtable-ptr emits `mov edx,[ecx]; call [edx+slot]` for COM-style dispatch.

STEERABLE. Evidence: CFileImage decoders / CImage 5 siblings (external no-body, reloc-masked);
WwdFile stream ctor/dtor/Open/Read; CNetMgr `MultiDispatch __stdcall` (no `add esp,0xc`) + its 3
`extern "C"` callbacks; CNetMgr reuses already-matched RegistryHelper::SetValueDword cross-TU;
RezMgr::Load collection iterators as __thiscall (cdecl gave wrong `push;call;add esp,4`).
related: dummy-virtual-slots.md, ret-n-calling-convention.md, thiscall-ignoring-this.md.
