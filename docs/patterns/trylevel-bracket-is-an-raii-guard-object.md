# A trylevel bracket around two plain API calls is an RAII GUARD OBJECT

- **Confidence:** 9/10
- **Tags:** `cpp:eh` `cpp:ctor` `cpp:dtor` `cpp:class` | `asm:mov` `asm:call` | `topic:codegen-idiom` `topic:eh`
- **Seen:** `FillCustomLevelList` 0x0003af90 (70.96 -> 86.72 on the guard alone,
  -> 100.00 EXACT with two further fixes), `CBattlezDlgCustom::DoDataExchange`
  0x000180e0 (its trylevel numbering was off by one for the same reason).

## Symptom

The recompile has **no `/GX` prologue at all** (`push -1 / push <handler> /
push fs:0 / mov fs:0,esp`) where retail does, and is tens of bytes short — but
every *body* instruction lines up. Two plainly-paired API calls bracket the
function body:

```
call <AfxGetThreadState>            <- ctor side
mov  ecx,[eax+4]
call <CWinApp::BeginWaitCursor>
mov  DWORD PTR [esp+0x338],ebx      <- trylevel := 0, IMMEDIATELY after
...   whole body ...
call <_chdir>
mov  DWORD PTR [esp+0x338],-1       <- trylevel := -1, IMMEDIATELY before
call <AfxGetThreadState>            <- dtor side
mov  ecx,[eax+4]
call <CWinApp::EndWaitCursor>
```

## Why it is a class and not two calls

A trylevel state exists to name an object that must be **unwound**. Two bare
calls create no object, so cl emits no frame — which is exactly what the
recompile shows. The state is entered the instant the first call returns and
left the instant before the second, i.e. the region *is* the object's lifetime.

The confirming evidence is the **shared out-of-line destructor**. Run
`gruntz sema xref <addr>` on the tiny function that holds the dtor's body
(here 13 bytes at `0x00018430`, `call AfxGetThreadState / mov ecx,[eax+4] /
jmp EndWaitCursor` — a TAIL JUMP, which a hand-written `void` helper would not
be): its callers are a fistful of addresses in the **unwind-funclet band**
(`0x001d....`-`0x001e....`), one per /GX function that uses the guard. Eight of
them here. An ordinary helper is called from real function bodies; a body called
*only* from funclets is a destructor.

## The fix

Put the guard in a **header** (never a `.cpp`-local type) with both bodies
in-class so cl inlines them at each use, and pin the out-of-line COMDAT copy
with `RVA_COMPGEN` in whichever TU owns that rva:

```cpp
class CWaitCursorScope {
public:
    CWaitCursorScope() { afxCurrentWinApp->BeginWaitCursor(); }
    ~CWaitCursorScope() { afxCurrentWinApp->EndWaitCursor(); }
};
```

```cpp
RVA_COMPGEN(0x00018430, 0xd, ??1CWaitCursorScope@@QAE@XZ)
```

Then replace the call pair with one declaration whose SCOPE is the trylevel
region — the guard must be declared where the state goes to 0 and go out of
scope where it goes to -1 (in `FillCustomLevelList` that is *after* the trailing
`_chdir("..")`, not before it).

## Corollary — the state NUMBER is a lexical counter

cl5 numbers trylevel states by a lexical walk of the function, so a guard
declared before other destructible locals shifts every later state up by one:
`CBattlezDlgCustom::DoDataExchange` had retail writing `1` into the trylevel
where the recompile wrote `0` for its `CString glob`, purely because retail had
one more (earlier) object. **An off-by-one trylevel value is a missing object
earlier in the function, not a scheduling accident.**

## Related

- [`eh-dtor-model-members-as-destructible`](eh-dtor-model-members-as-destructible.md)
- [`derived-dtor-inlines-only-in-class-base-dtors`](derived-dtor-inlines-only-in-class-base-dtors.md)
- [`two-identical-blocks-with-different-eh-states`](two-identical-blocks-with-different-eh-states.md)
