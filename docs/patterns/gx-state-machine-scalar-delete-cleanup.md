# /GX state-machine error paths destroy the local via the scalar-deleting dtor (re-stamp vtbl + `call [vtbl+4]`), not the simple `~T`
tags: cpp:eh cpp:dtor cpp:local cpp:method | asm:call asm:mov | topic:wall topic:eh
symptoms: a big /GX serialize/snapshot fn with many fall-through reject paths; each reject emits `mov [esp+N],<scalardel-vtbl>; lea ecx,[esp+N]; mov [esp+state],K; call ds:[<vtbl+4>]; movb [esp+state],K-1; jmp <common-tail>` then the common tail still runs `~T`; your idiomatic `return 0;` emits only `lea ecx; mov [esp+state],-1; call ~T; xor eax,eax`
confidence: 7/10

A large `/GX` function holding a destructible class local `T S;` and rejecting on many
sequential `if (op()==0) return 0;` paths: the retail compiler, at EACH reject, re-stamps the
local's vtable to the **scalar-deleting** vtable and invokes the scalar-deleting destructor
(`call ds:[vtbl+4]`) as part of the unwind funclet, advancing a per-path `__ehfuncinfo` state pair
(even K then odd K-1) before jumping to a shared tail that runs the plain `~T` + the embedded
member dtor. Idiomatic C++ scope-exit (`return 0;`) cannot express this: it lowers each exit to a
single simple-`~T` call with state -1, so the per-reject blocks diverge wholesale and objdiff's
global alignment desyncs across the long fail ladder. Making `~T` `virtual` does NOT flip it (it
just emits a spurious vtable and regresses) — the scalar-delete is the optimizer's chosen unwind
shape, not a source-visible polymorphism.

```cpp
// what you WRITE (correct logic; the carcass matches, the cleanup ladder does not):
T S; S.Init(); S.Begin();
if (S.WriteHeader(...) ) { ... }
if (op1(&S)==0) return 0;   // each reject -> simple ~T at scope exit
if (op2(&S)==0) return 0;
```
```asm
; what you SEE per reject in retail:
mov  dword [esp+0x10], <scalardel_vtbl>   ; re-stamp the local's vptr
lea  ecx, [esp+0x10]
mov  dword [esp+state], 0x8                ; even EH state
call ds:[<scalardel_vtbl+4>]              ; scalar-deleting destructor
mov  byte  [esp+state], 0x7                ; odd EH state
jmp  <common-tail>                         ; tail runs ~member + ~T
```
WALL (the /GX unwind-funclet destructor selection; defer to a final-sweep leaf-first redo once the
serializer + child classes are fully modeled). Evidence: CDDrawSurfaceMgr::SnapshotChildren
@0x156020 (1285 B, ~14 unwind states) — full carcass (offsets, CTime header, inline strcpy, ordered
child-op call sequence) reproduced, plateaus ~74.6% fuzzy; the residual is this per-reject
scalar-delete ladder + the cb-in-esi vs ebx regalloc. related: big-seh-fuzzy-desync.md,
eh-state-numbering-base.md.
