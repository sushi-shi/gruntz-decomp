# Two `cmp p,0` on one pointer = a source `if` around a `delete` whose dtor INLINES
tags: cpp:dtor cpp:branch cpp:eh | asm:cmp asm:test asm:je asm:call | topic:codegen-idiom
symptoms: retail tests the same register against zero twice 8-13 bytes apart, the second `je` landing on the `p = NULL` store; between them a spill of the pointer to a frame slot; the base has only the first test and calls an out-of-line `??1`; `dup_compare --any-dest` flags the pair
confidence: 9/10
variants: guarded-delete-null-store-elision.md, ctor-inline-cut-depth-varies-per-new-site.md

`guarded-delete-null-store-elision.md` is the OUT-OF-LINE case: with `call ??1...`,
cl folds the source `if (p)` and `delete`'s own guard into one test. When the
destructor is **inline in the header** the fold does not happen — `delete p` expands
in place, keeps its own null test, and the source `if` supplies a second one. So a
doubled test is not a peephole quirk; it says *this class's destructor is inline*.

```cpp
// header, NOT the .cpp - this is what makes the second test appear
inline CWorldSoundSet::~CWorldSoundSet() { Deactivate(); }

// call site
if (m_worldSounds != NULL) {
    delete m_worldSounds;      // expands: null test, Deactivate(), ~CPtrList(m_list), operator delete
    m_worldSounds = NULL;      // INSIDE the guard
}
```

```asm
  mov  edi,[esi+0x54]
  cmp  edi,ebp                 ; the source if
  je   AFTER_NULL_STORE
  cmp  edi,ebp                 ; delete's own guard
  mov  [esp+0x24],edi          ; EH-visible copy for the unwind funclet
  je   NULL_STORE
  mov  ecx,edi
  mov  [esp+0x1c],0            ; EH state for the inlined dtor body
  call Deactivate
  lea  ecx,[edi+0x8]
  mov  [esp+0x1c],-1
  call ??1CPtrList@@QAE@XZ     ; the member, destroyed after the body
  push edi
  call ??3@YAXPAX@Z
NULL_STORE:
  mov  [esi+0x54],ebp
```

STEERABLE, and it is a structure fix, not a spelling: move the destructor body into
the class header, delete the `.cpp` definition, and re-pin the out-of-line copy with
`RVA_COMPGEN` in whichever object file still *calls* it (cl declines the inline in
large callers and emits the COMDAT there — retail does the same). The hand-transcribed
alternative (`p->Deactivate(); (&p->m_list)->CPtrList::~CPtrList(); ::operator delete(p);`)
reproduces the body but never the second test, because there is no `delete` to emit it.

Measured 2026-08-08: `~CWorldSoundSet` inlined -> `CGruntzMgr::LoadWorldMode` 89.55 ->
98.06, `CGruntzMgr::Close` 89.76 -> 91.87, `CGruntzMgr::Run` 82.07 -> 83.61, and the
COMDAT at 0x85ed0 stayed 100.00 after moving its pin from worldsoundset to gruntzmgr.
Same move for `~DirectInputMgr2` (0x85fc0) and `~CSaveGame` (0x85b50); the latter also
retired `include/Io/SaveGameDtorInline.h`, a header whose only job was to give one TU
an inline copy while the .cpp kept an out-of-line one.
