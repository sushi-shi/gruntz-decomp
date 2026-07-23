# Recover a hidden member from the caller's `mov ecx`
tags: cpp:method cpp:static asm:mov | topic:codegen-idiom
symptoms: a member fn whose body never reads `this`/ecx, or a free-looking fn that calls methods without loading ecx; callers load the same real object into ecx immediately before the call
confidence: 10/10

A `__thiscall` member that never touches `this`/ecx produces callee bytes IDENTICAL to a
`static __stdcall` (same `ret N`, args all off the stack). The ONLY difference is on the CALLER
side: callers of the thiscall emit a redundant `mov ecx,this` before each call. So decide by the
caller, not the callee — if callers set ecx, keep it a (this-ignoring) thiscall member; declaring
it static drops the `mov ecx` and regresses every caller.

STEERABLE. Evidence: RegistryHelper::GetRegistryKey (@0x139650) is `?…@@QAEH…` thiscall (ret 0xc)
even though its body never reads `this` — declaring it static regressed callers. Contrast
geterrorstring-static-cdecl.md (the DX/Net error formatters ARE genuinely static __cdecl — there
the call site is `push×3; call; add esp,0xc`, no `mov ecx`). related: ret-n-calling-convention.md.

The same evidence also works in reverse. A reconstruction can misidentify a real member as a
free `__stdcall` function and then invent a no-body “threaded” helper so its body can issue a
member call without naming the receiver. The characteristic shape is:

1. every real caller loads the same class pointer into `ecx` immediately before the call;
2. the alleged free function never initializes `ecx`;
3. it directly calls one or more known `__thiscall` methods; and
4. its `ret N` accounts only for the explicit stack arguments, exactly as a member would.

That is not a special hidden-register free-function ABI. The incoming `ecx` is `this`: rehome the
function on the proven class, call the known methods normally, and delete the fabricated bridge
prototype.

Two measured examples establish the pattern:

- `GetCtrlE` at `0x000c2640` is the fifth `CMultiStartDlg` accessor beside `GetCtrlA..D`.
  Callers load the dialog into `ecx`, and the function calls `CWnd::GetDlgItem` without setting
  `ecx`. Restoring `CMultiStartDlg::GetCtrlE` removed the phantom `GetDlgItemThreaded`; the two
  adjacent selector helpers became exact once restored as members too.
- `SerializeDispatch` at `0x00163710` receives `CGameLevel::m_mainPlane` in `ecx` and directly
  calls `CDDrawWorkerHost::Save`/`Load`. Restoring it as a `CDDrawWorkerHost` member removed the
  phantom `PlaneSaveVia` and `PlaneLoadVia` aliases.

When draining declared-only aliases, search for this pattern in reverse: the fake bridge is often
the last visible symptom of a caller/callee class identity that has already been proven by `ecx`.
