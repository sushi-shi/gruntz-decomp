# Extra `mov [esi],&??_7Class` in a dtor = A MIS-MODEL (not a codegen wall): the class
# isn't polymorphic (A), or retail's dtor is compiler-generated and yours isn't (B)
tags: cpp:dtor cpp:eh cpp:virtual cpp:layout cpp:implicit-member | asm:mov | topic:mis-model topic:phantom-wall topic:codegen-idiom
symptoms: ~Class body byte-identical except ONE extra `mov [esi],&??_7Class` re-stamp right after `mov [esp+N],this`, before the trylevel write; recompile caps ~92-93% with the /GX frame + base-dtor call already exact
confidence: 10/10 (both causes MEASURED with cl 5.0 both ways; was 6/10 while the diagnosis was wrong — see HISTORY)

**Symptom.** Your `~Class` matches retail except for one extra most-derived vptr re-stamp
that retail simply does not have:
```asm
6ff: mov esi,ecx
701: mov [esp+4],esi          ; retail goes straight to the trylevel write...
705: mov [esp+0x10],1         ;   (NO mov [esi],&??_7Class here)
70d: call Reset
; our recompile inserts `mov [esi],&??_7Class@@6B@` between 701 and 705.
```

**It is always a MIS-MODEL, never a compiler quirk — and there are TWO of them.** Run the
vtable scan FIRST; it tells you which cause you have:

| `gruntz.core.vtable_scan` says | cause | fix |
|---|---|---|
| **no vtable / no RTTI** for the class | **CAUSE A** — retail's class isn't polymorphic; you declared it so | de-inherit: the "base" is really a MEMBER at +0x00 |
| **a real RTTI-backed vtable** exists | **CAUSE B** — the class IS polymorphic, but retail's dtor is **compiler-generated**; yours is user-declared | delete the destructor declaration; pin the body with `RVA_COMPGEN` |

Cause B is the common one for MFC dialog / engine leaf classes, and the vtable scan alone
distinguishes them in one command. **Do not stop at "it has a vtable, so the wall stands."**

---

## CAUSE A — the class is not polymorphic

**Diagnose** — check the binary, not the codegen:
1. Does `Class` have a vtable in the RTTI/vtable scan (`gruntz.core.vtable_scan`)? Any RTTI name?
2. Does retail's `~Class` restamp at all? (If it never does, it isn't polymorphic.)
If both say no: **the "base" is really a MEMBER at +0x00.**

**Why it hides:** a polymorphic base at +0x00 and that same class as a *member* at +0x00 have
**identical layout AND identical codegen** (`mov ecx,esi; call ~CPtrList` either way). Nothing in
the bytes distinguishes them — only the vtable's existence does.

**FIX:** de-inherit; hold the base as a member; drop `virtual` from the dtor.
```cpp
// WRONG - forces a vtable, a restamping dtor, and a vtable-dispatched `delete`
class CFontConfig : public CPtrList { virtual ~CFontConfig() OVERRIDE; };
// RIGHT - retail has no CFontConfig vtable/RTTI and its dtor never restamps
class CFontConfig {
    CPtrList m_list;   // +0x00 (0x1c) - same layout, same codegen
    ~CFontConfig();    // non-virtual -> `delete p` binds DIRECT, as retail does
};
```
**The protected-access tell.** If de-inheriting breaks on `error C2248: cannot access protected
member` (`m_pNodeHead`, `CNode`, `m_nCount`), that open-coded node-walk is *itself* the mis-model
— it is what forced the fake inheritance. MFC's public accessors are inline and emit the same
loads, so they are byte-identical AND legal:
`GetHeadPosition()` → `return m_pNodeHead;` · `GetNext(pos)` → `n=pos; pos=n->pNext; return n->data;`
· `GetCount()` → `return m_nCount;`

**Modeling the base subobject is still required** for the EH frame + base-dtor call (else you fall
to the ~50-60% plateau of eh-dtor-needs-base-subobject.md) — a *member* supplies both identically.

**Evidence.** `CFontConfig::~CFontConfig` (0x85f40): 92.7% → **100.0000%** on de-inheriting;
`FreeNodes` → **100%** on swapping the protected node-walk for the public API. It also let
`delete m_chatLog` bind direct instead of through a vtable (see the delete-fold work).

---

## CAUSE B — the class IS polymorphic, but retail's dtor is COMPILER-GENERATED

**cl 5.0 elides the most-derived vptr store in an IMPLICIT (compiler-generated) destructor, and
always emits it for a user-declared one — even an empty `~Class() {}`.** With no user body, cl
knows nothing can observe the vptr between the stamp and the base dtor's own stamp, so it never
emits it. This is the whole difference; nothing else about the two dtors differs.

**MEASURED** (cl 5.0 `/nologo /c /O2 /MT /GX`, identical class both ways):

```cpp
struct Derived : Base { virtual ~Derived(); Str m_54; };   // user-declared (any spelling:
Derived::~Derived() {}                                     //  out-of-line OR inline in-class)
//   mov esi,ecx
//   mov DWORD PTR [esi],OFFSET ??_7Derived@@6B@   <-- EMITTED
//   lea ecx,[esi+0x54] ...

struct Derived : Base { Str m_54; };                       // NO declared dtor -> implicit
//   mov esi,ecx
//   lea ecx,[esi+0x54] ...                        <-- NO stamp. Matches retail.
```

**FIX: delete the destructor declaration.** It is still virtual (the base's is), still destroys
the members, still emits. Since there is then no source body to hang `RVA()` on, pin it with a
self-contained label — cl emits the COMDAT in **every using obj**, so put the pin in a TU whose
base obj actually emits it (the `RVA_COMPGEN` authority check is nm membership):

```cpp
// in the class header: NO `virtual ~CFoo() OVERRIDE;` at all
// in a .cpp whose obj emits it (defines the ctor -> vtable -> ??_G -> ??1, or
// stack-constructs a CFoo):
RVA_COMPGEN(0x000b8960, 0x59, ??1CFoo@@UAE@XZ)
```

**Where the COMDAT lands is evidence, not an accident.** Retail emitted `~CMultiStartDlg` at
0xb8960 — beside `CMulti::ShowMultiStartDlg` (0xb86c0), which stack-constructs the dialog. Once
the dtor is implicit, cl reproduces exactly that placement, and the dedicated
`ShowMultiDlg.cpp` holding TU that existed only to host the user-declared body is deleted.

**Evidence.** `??1CMultiStartDlg@@UAE@XZ` (0xb8960): 92.7% → **100.0000%**. `??1CWarlord`
(0x107f0): 73.95% → 85.43% (its residual is a different bug — a missing `CWapX` MI base at
+0x34, RTTI-proven; see src/Gruntz/Warlord.cpp). Same fix already independently in the tree for
`??1CBattlezDlg` (0x14c90) and `??1CBattlezDlgCustom` (0x17140) — see src/Gruntz/Dialogs.cpp.

**Do not confuse this with a real wall.** Three sites were `@early-stop`'d on this doc's old
"unreachable restamp" story. One (`ApplyColorSlot0..3`) had **no restamp in its diff at all** —
the note was copied, not measured; its real residual was argument-evaluation order (the MFC
inline `CWnd::InvalidateRect` member vs the global import), and it went 91.1% → **100%**.
**Re-measure the diff before you inherit a wall's diagnosis.**

**HISTORY.** (1) The doc originally claimed *"cl elided it because the dtor body makes no virtual
call on `this`, so the vptr value is dead"*, concluded *"the polymorphic model is correct; only
this one re-stamp is unreachable"*, and told readers to **defer to the final sweep**. That invented
a compiler behaviour to explain evidence it had not measured. Disproved twice over: cl keeps the
stamp across a call that cannot read the vptr (`~CAttract` 0x8cd90, 100% match, stamps then calls),
so "dead store" was never the rule. (2) The 2026-07-17 rewrite fixed the cause to "the class isn't
polymorphic" (CAUSE A, from CFontConfig) — right for that class, but it then read as *"has a vtable
⇒ the wall stands"*, which re-parked three polymorphic sites that were all fixable (CAUSE B).
A wall that survives only because a doc says it's unfixable is a phantom: when the bytes and your
model disagree, suspect the MODEL first — and **measure the compiler both ways instead of
reasoning about what it "must" do**. Both causes here are now cl-5.0-measured, not argued.
related: eh-ctor-vptr-restamp-position.md, eh-dtor-needs-base-subobject.md, inline-base-dtor-folds-into-leaves.md
