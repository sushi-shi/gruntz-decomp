# A dead `mov ecx,...` before a call is a `this` the callee's signature dropped
tags: cpp:call cpp:member cpp:class | asm:mov asm:call | topic:correctness topic:identity topic:codegen-idiom
symptoms: dead mov ecx before call, receiver, __thiscall, __stdcall, free function, mangling QAE YG YA, walls thisscan
confidence: 10/10

A `__thiscall` member and a free `__stdcall`/`__cdecl` with the SAME stack
arguments produce **identical callee bytes**: the receiver travels in ECX, which
costs the callee nothing, and a body that never touches `this` never reads it.
So a member modelled as a free function scores **100% EXACT on the callee under
either model**, and no test that looks at the callee - call-set, CFG, frame, EH -
can see the error. Only a CALLER shows it, and only as an ECX load retail emits
before the call that we do not. cl 5.0 emits no dead loads, so a load whose value
nothing consumes before the call **is** a receiver.

```cpp
// WRONG - scores 100% on the callee, caps every caller
void __stdcall UnpackTag(RezTypeTag tag, char* dst);

// RIGHT - same bytes, and the caller now emits the ECX load
class CRezArchive { void UnpackTag(RezTypeTag tag, char* dst); };
```
```asm
; retail caller                       ; ours
mov  ecx,DWORD PTR [esp+0x10]         ; (nothing)
lea  eax,[esp+0x28]                   lea  eax,[esp+0x28]
push eax                              push eax
push esi                              push edi
call ?UnpackTag@...                   call ?UnpackTag@...
; ECX is never read again             ; four stack args + ret 8 on BOTH sides
```

STEERABLE, and the fix is a declaration move, not a body edit. **The
discriminator is that the value is DEAD**: retail routinely materialises a
*pushed argument* through ECX where we use EDX (`mov ecx,[ebx]; push eax; push
ecx; call CellTargetable`), and that is a register-name rotation, not a receiver.
`pop ecx` is cl's `add esp,4` and is never a receiver either. A definition that
is dead but is not a LOAD is also not a receiver: `and ecx,0x3` is arithmetic
that landed in ECX (`?FileExists@@YAHPBD@Z`). Read the ECX source to name the
class - a `[reg+N]` member load names the field, `mov ecx,ebp` after a
prologue `mov ebp,ecx` names the caller's own `this`, and a spilled-`this` frame
slot is identified by the sibling call that used it (`CRezArchive::PackTag` takes
`mov ecx,[esp+0x14]`, then `add esp,4` shifts the same slot to `[esp+0x10]`).

**SWEEP IT WITH `gruntz walls thisscan --retail`, not the paired form.** Our
side has no receiver by construction, so the paired "we lack it" test is a
recall tax rather than evidence; the retail-only screen reads retail's own call
sites, needs no report, and reaches callers that are EXACT, unpaired or
unreconstructed. See
[one-sided-screen-beats-the-paired-diff](one-sided-screen-beats-the-paired-diff.md)
for the calibration and the measured noise floor.

Evidence, four instances, no callee's bytes moved in any of them:
`CMultiStartDlg::CommitLatencySelection` 92.08 -> 100.00 EXACT
(`CLatencyList::GetSelItemData`), `CRezArchive::ImportDirectoryTree` 99.67 -> 100.00
EXACT (`CRezArchive::UnpackTag`), `CGameLevel::LoadWwd` 95.53 -> 96.24
(`CGameLevel::InflateMainBlock`), and `CBattlezMapConfig::RerouteIdleUnit` 0x029af0 -
which the PAIRED screen structurally could not reach. It sat at 100.00 EXACT as
`void __stdcall TileSwitch(CGrunt*, i32, i32, i32, i32, i32)`; retail's sole
caller `CBattlezMapConfig::Step` does `mov ecx,edi` with `edi` the prologue's
own `this`, and the callee opens `mov eax,[esp+0x10]` so ECX is dead in the
body too. The caller is at 87%, so our side ALSO had an ECX definition in the
window and the paired screen cancelled the row. Note the call site needed no
edit: an unqualified `TileSwitch(g, ...)` inside a `CBattlezMapConfig` method
resolves to the member once the free declaration is gone.
