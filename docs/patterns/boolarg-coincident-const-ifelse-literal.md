# Short-circuit bool arg beside a coincident constant: write if/else with literal args, not the inline `||`
tags: cpp:branch cpp:local | asm:push asm:mov | topic:codegen-idiom
symptoms: push eax;push eax vs push 1;push 1; mov eax,1 before two pushes; GetDlgItem EnableWindow; short-circuit || as a call argument; true-branch 3 bytes too long
confidence: 8/10

A call site like `GetDlgItem(1)->EnableWindow(A()||B()||C())` — a short-circuit `||`
passed as a bool argument while ANOTHER operand (here the `GetDlgItem` nID `1`) equals
the bool's true value. The inline-`||` spelling materializes the bool into `eax` (=1 in
the true path) and the optimizer then REUSES that `eax` for the coincident constant push,
emitting `mov eax,1; push eax; push eax` — 3 bytes longer than retail and a size mismatch.
Splitting into an explicit if/else with **literal** args keeps the two constants as
separate immediate pushes in the true branch, and the false branch reuses the failed
short-circuit test's zero (`push eax`) for free — exactly matching retail.

```cpp
// NOT this (shares eax for the coincident const -> mov eax,1; push eax; push eax):
//   GetDlgItem(1)->EnableWindow(A() || B() || C());
// THIS (literal pushes in true, reused failed-test zero in false):
if (A() || B() || C()) {
    GetDlgItem(1)->EnableWindow(1);
} else {
    GetDlgItem(1)->EnableWindow(0);   // cl reuses the je's eax=0 -> `push eax`
}
```
```asm
; true  branch:  6a 01            push 1        ; 6a 01  push 1 (nID)   -> two literals
; false branch:  50               push eax(=0)  ; 6a 01  push 1 (nID)   -> reused zero
```
Steerable. Closed CBattlezDlg::ApplyOption0..3 (0x15de0/15e60/15ee0/15f60) 91.84% → 100%;
the inline-`||` form was both 3 bytes long AND size-mismatched on the true branch.
