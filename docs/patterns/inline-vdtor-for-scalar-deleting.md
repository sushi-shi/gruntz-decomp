# To match a `??_G` (scalar-deleting dtor) whose target inlines the dtor body, define `~Class()` INLINE in the header
tags: cpp:dtor cpp:eh cpp:inline | asm:call asm:test | topic:codegen-idiom
symptoms: your `??_G` does `call ??1Class` (one extra instruction, structural diff) where the target inlines the dtor body (store vtbl; call base; `test [esp+8],1`; conditional delete; `ret 4`)
confidence: 8/10

The `??_G` scalar-deleting dtor IS the slot-0 vtable function (`??_GClass@@UAEPAXI@Z`, carries
the hidden `int flags`: `test [esp+8],1` then conditional `operator delete`, `ret 4`). When the
target INLINES the destructor body into `??_G` rather than `call`ing the out-of-line `??1`, you
reproduce it by defining `~Class()` INLINE in the header — cl then folds the vtable-restore + base
cleanup directly into the synthesized thunk. An out-of-line dtor makes `??_G` emit `call ??1Class`
(the extra instruction). Two supporting levers from the same match:

- A base cleanup reached as `__thiscall` (model it as a method on a tiny helper struct,
  `((CGameModeBase*)this)->BaseCleanup()`) so the dtor tail-call emits NO `add esp,4` (a free
  `extern "C"` cdecl fn adds the spurious cleanup).
- `delete pObj` reproduces the call-site `mov eax,[ecx]; push 1; call [eax]` exactly — one
  `virtual ~T()` puts `??_G` at slot 0; no manual vtable forging.

STEERABLE. Evidence: CState `??_G` @0x8c710 inline dtor → 98.75% (out-of-line → extra `call ??1`);
CMapMgr dtor calls Reset inline; CGruntzApp dtor inlines base `~CGameApp` (CloseResources called
twice). related: reloc-typing-vptr-global.md (the residue is reloc-masked operands).
