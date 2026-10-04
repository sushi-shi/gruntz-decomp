# Win32 window procedures and dynamically loaded Toolhelp functions

This page accounts for **nine retained expressions**: three in `MultiStartDlg.cpp` and six in `Utils.cpp`. They cross real Windows API boundaries. Equal pointer widths alone would not justify an arbitrary callback cast; each target signature and calling convention must agree with the actual exported function or window message.

## Window subclassing: three sites

| Site | Exact expression | Source → destination |
| --- | --- | --- |
| [InitializeWorldCombo](../../src/Gruntz/MultiStartDlg.cpp#L97), save previous procedure | `reinterpret_cast<WNDPROC>(GetWindowLongA(editHwnd, GWL_WNDPROC))` | SDK `LONG` result → `WNDPROC` |
| Same function, install callback | `reinterpret_cast<LONG>(MultiMapComboEditProc)` | `LRESULT (CALLBACK*)(HWND, UINT, WPARAM, LPARAM)` → SDK `LONG` |
| [MultiMapComboEditProc](../../src/Gruntz/MultiStartDlg.cpp#L106), decode `WM_SETTEXT` | `reinterpret_cast<LPCTSTR>(lParam)` | SDK `LPARAM` → narrow-build `const char*` |

The declarations come from the [pinned VC5 toolchain](../toolchain-vc50-sp3.md): `msvc/include/WINUSER.H` declares `WNDPROC`, `GetWindowLongA`, `SetWindowLongA`, and `CallWindowProcA`; `WINDEF.H` defines `CALLBACK` as `__stdcall`, `LPARAM` as `LONG`, and the message/result integer types. `WINNT.H` defines `LONG` as `long`. The project is Win32: these integers and pointers occupy 32 bits. The source callback exactly matches `WNDPROC`, including its return type and `CALLBACK` convention.

`InitializeWorldCombo` locates the combo's child edit control, makes it readonly, stores its previous procedure in the `WNDPROC` global `g_savedMultiWndProc`, and installs `MultiMapComboEditProc`. The callback rejects an empty `WM_SETTEXT` string and delegates other messages through `CallWindowProcA`. It does not dereference a function pointer as an object. In particular, the saved procedure must be passed to `CallWindowProcA`, not assumed to be directly callable: Windows can represent a previous procedure with an internal value understood by that API.

The text cast is valid only under the `WM_SETTEXT` payload contract: its integer message slot carries a pointer to readable, NUL-terminated narrow text for the duration of message dispatch. `strcmp` reads it without taking ownership. The cast does not validate nullness, termination, or provenance; arbitrary messages with invalid text remain unsafe. No blanket claim about malicious message inputs follows from the correct type conversion. The installed callback has static program lifetime; `g_savedMultiWndProc` is a single global, so the code also assumes the corresponding subclass lifecycle rather than independently tracking multiple edit instances.

Microsoft's [commit-pinned edit-control subclassing example](https://github.com/MicrosoftDocs/win32/blob/e103fa4e8810bd8d42c4777e17081e24dbe62dbd/desktop-src/winmsg/using-window-procedures.md#subclassing-a-window) uses the same Windows integer/procedure boundary and delegates through `CallWindowProc`. This is an API example, not original Gruntz source. The [WM_SETTEXT contract](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-settext) identifies the text pointer carried by `lParam`.

### Origin and possible removal

[6d500a7eab](https://github.com/sushi-shi/gruntz-decomp/commit/6d500a7eab0cec44cf10b2e6e983867f4f5645bc) mechanically converted typedef-target C-style casts to named casts. Later [a4b9e6b95b](https://github.com/sushi-shi/gruntz-decomp/commit/a4b9e6b95b1093007d384d655b95862a20bad4c8) hid several Windows boundaries behind a message union. [3f5238e4a4](https://github.com/sushi-shi/gruntz-decomp/commit/3f5238e4a4d4afca10ecce899b21a247995bbfb5) established all three current expressions while correcting the saved procedure from `i32` to `WNDPROC` and the callback return from `i32` to `LRESULT`. Its reason was native callback typing, not proof that every integer can safely be called or dereferenced. Subsequent naming/rehome changes did not introduce these boundaries.

Removing the casts by returning to union type-punning would obscure the same conversions. A modern port can use `GetWindowLongPtr`/`SetWindowLongPtr` and pointer-sized integers, but those are not the original VC5 declarations. Directly changing `LONG` to a larger application type cannot change the SDK's ABI. No source correction is needed at these three conversion expressions under their stated Win32 contracts.

## Toolhelp loading: six sites

[Utils.cpp](../../src/Gruntz/Utils.cpp#L17) defines the actual target types:

```cpp
typedef BOOL(WINAPI* PROCESSWALK)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef HANDLE(WINAPI* CREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL(WINAPI* MODULEWALK)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
```

| Individual site | Exact expression |
| --- | --- |
| [ExistProcess, snapshot](../../src/Gruntz/Utils.cpp#L192) | `reinterpret_cast<CREATESNAPSHOT>(GetProcAddress(hKernel, "CreateToolhelp32Snapshot"))` |
| [ExistProcess, first process](../../src/Gruntz/Utils.cpp#L198) | `reinterpret_cast<PROCESSWALK>(GetProcAddress(hKernel, "Process32First"))` |
| [ExistProcess, next process](../../src/Gruntz/Utils.cpp#L204) | `reinterpret_cast<PROCESSWALK>(GetProcAddress(hKernel, "Process32Next"))` |
| [GetProcessModule, snapshot](../../src/Gruntz/Utils.cpp#L282) | `reinterpret_cast<CREATESNAPSHOT>(GetProcAddress(hKernel, "CreateToolhelp32Snapshot"))` |
| [GetProcessModule, first module](../../src/Gruntz/Utils.cpp#L288) | `reinterpret_cast<MODULEWALK>(GetProcAddress(hKernel, "Module32First"))` |
| [GetProcessModule, next module](../../src/Gruntz/Utils.cpp#L294) | `reinterpret_cast<MODULEWALK>(GetProcAddress(hKernel, "Module32Next"))` |

Every source is the SDK's generic `FARPROC` returned by `GetProcAddress`; the exact destination is the corresponding typedef above. `msvc/include/TLHELP32.H` confirms the parameter types, `BOOL`/`HANDLE` results, and `WINAPI` (`__stdcall`) convention. The program checks each result before calling. `KERNEL32.DLL` is obtained with `GetModuleHandle`; no borrowed module reference is released while its procedures are used. Each walk passes a real, correctly aligned `PROCESSENTRY32` or `MODULEENTRY32` object with its size initialized; no object aliasing or expired payload is introduced by these casts.

These are supported Windows dynamic-link operations, not a general ISO C++ license to call any reinterpreted function pointer. A wrong export name, parameter list, return convention, or calling convention would invalidate the call even on 32-bit x86. Here the names and prototypes agree. Static imports could remove the casts but would change dynamic availability and failure behavior.

### Provenance and independent example

[5cebf5371e](https://github.com/sushi-shi/gruntz-decomp/commit/5cebf5371e80d51e4621ac3f45c327d5fd7d5853) adopted the surviving Monolith utility bodies, their typedefs and original function names, replacing reconstructed loader wrappers. [The lineage ledger](../../config/lithtech_lineage.tsv) records `autorun-utils-existprocess` and `autorun-utils-getprocessmodule`, source `Game/Game/Autorun/Utils.cpp`, blob `60d52ba30fc9a49f0b2b56efb9dbc0958398d69d`, as adapted cross-game copies. That is the original-family provenance; it is stronger than attributing them to a later include or comment change.

The independently maintained [StackWalker Toolhelp loader at a pinned commit](https://github.com/JochenKalmbach/StackWalker/blob/7af402408202a5c00021fd57e18e39e7e6f11062/Main/StackWalker/StackWalker.cpp#L616-L669) declares matching `__stdcall` snapshot/module-walk signatures, converts `GetProcAddress` results, checks them, and calls them with actual Toolhelp records. It is a real analogue for the boundary, not evidence of Gruntz authorship. The six current casts are retained.
