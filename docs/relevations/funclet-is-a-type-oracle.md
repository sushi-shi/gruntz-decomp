# The unwind funclet names the type

**The revelation:** a `/GX` unwind funclet is not compiler noise to be masked. It is a
**per-member type assertion the compiler wrote down for us**, and comparing ours against
retail's tells us *which member or base class we modelled wrong* — a question no
percentage, no branch diff, and no member-store audit can answer.

Acting on it took the exception band from 1758 to 2706 exact records of 3034 in one wave,
and dissolved three classes that never existed. Overall exact 3449 → 3466.

---

## Why a funclet is evidence

MSVC 5.0's `/GX` gives every function with destructible locals a table of **unwind
states**. Each state names a tiny code fragment — the *funclet* — that destroys exactly one
object if an exception passes through. cl emits one per object, in construction order, and
each one is three instructions:

```asm
FUN_005d8c60  @ RVA 0x1d8c60  (size 8 B)          ; the unwind funclet for one member
    mov    ecx, DWORD PTR [ebp-0x10]              ; ecx = &that member, off the frame
    jmp    0x1343                                 ; -> ITS destructor
```

That jump target is the whole point. **The compiler is telling us the member's type**, by
name, in the binary. If retail's funclet jumps to `zBitVec::~zBitVec` and ours jumps to
`CUserBaseLink::~CUserBaseLink`, the two builds disagree about *what that member is* — not
about how to schedule instructions.

The census that reads this is `gruntz.delink.eh_band --census`, and its
`different-targets` bucket is the type-defect bucket:

```
                start        after
identical        493          648      of 750 groups
different-targets 147           54     <- "we destroy a different thing than retail"
frame-offset     105           44
```

---

## Worked example: `CUserBaseLink` was never a class

### What we had

```cpp
// include/Gruntz/UserBaseLink.h  (deleted)
struct CUserBaseLink {
    CUserBaseLink();
    ~CUserBaseLink() {}
    zBitVec m_str;
};

// include/Gruntz/UserLogic.h
class CUserLogic {
    ...
    CWwdGameObjectA* m_object;
    CLogicRecord*   m_logicRecord;
    CUserBaseLink    m_link;      // <- one member, holding one member
    i32              m_gatedCallbackCode;
};
```

A wrapper holding a single `zBitVec`. Nothing in the *constructor* path can disprove it:
cl emits the same code for `m_link.m_str = tmp` as for `m_actBits = tmp`, and the wrapper's
own constructor is at the same RVA either way. Every score agreed. It sat there for months.

### What the funclets said

In **every** `CUserLogic`-derived constructor, retail's unwind funclet for the object at
`this+0x18` jumps to `zBitVec::~zBitVec`:

```asm
??1zBitVec@@UAE@XZ  @ RVA 0x16d2a0  (size 38 B)
    push   esi
    mov    esi, ecx
    mov    eax, DWORD PTR [esi+0x8]
    mov    DWORD PTR [esi], 0x5f04c8          ; vptr
    cmp    eax, 0x20                          ; inline capacity?
    jbe    .done
    mov    eax, DWORD PTR [esi+0xc]
    push   eax
    call   0x120c30                           ; free the heap buffer
    add    esp, 0x4
.done:
    mov    ecx, esi
    call   0x16da60
    pop    esi
```

Ours jumped to a `??1CUserBaseLink@@QAE@XZ` COMDAT that cl synthesises for the wrapper's
implicit destructor — **same slot, same offset, one indirection retail does not have.**

The confirmation was the wrapper's only out-of-line function. What we had labelled
`CUserBaseLink::CUserBaseLink()` at `0x16d710` is a 118-byte body that *is* the bit-vector
constructor — the header's `inline zBitVec::zBitVec()` expanded into an otherwise empty
`{}`:

```asm
??0zBitVec@@QAE@XZ  @ RVA 0x16d710  (size 118 B)
    push   0xffffffff
    push   0x5e2b98                           ; the /GX registration this file is about
    mov    eax, fs:0x0
    push   eax
    mov    DWORD PTR fs:0x0, esp
    ...
```

So the wrapper was never anything but a name in front of a member. Only the **destructor
path** could tell the two apart — which is exactly what the funclets did.

### The fix

```diff
--- a/include/Gruntz/UserLogic.h
+++ b/include/Gruntz/UserLogic.h
-#include <Gruntz/UserBaseLink.h>
+#include <Wap32/zBitVec.h>

     CWwdGameObjectA* m_object;
     CLogicRecord* m_logicRecord;
-    CUserBaseLink m_link;
+    zBitVec m_actBits;
     i32 m_gatedCallbackCode;

     {
         zBitVec tmp("", 0);
-        m_link.m_str = tmp;
+        m_actBits = tmp;
     }
```

`include/Gruntz/UserBaseLink.h` deleted. The member was also **renamed on evidence**:
`m_link` was named for the wrapper's supposed role, but the member is a bit-set sized by
`g_defaultProjActSize` and read and written beside `m_gatedCallbackCode` — so it is `m_actBits`.

### The result

```
EH band records exact                    2508 -> 2632   (+124)
funclet groups byte-identical             493 ->  615   of 750
  ...of which the frame-offset class      105 ->   41
```

That last line is the tell that the model, not the codegen, had been wrong: removing one
indirection **also** fixed 64 "frame-offset" rows, because the wrapper had been shifting the
frame layout in those constructors all along.

Cost: 11 functions dip on current % from the ~57-TU header ripple; every one holds its MAX.

---

## The same oracle, twice more

**`RezFreeStdcall` was MFC's `CObject::operator delete`.** A game-looking function at
`0x853d0`, except: **all 56 of its retail callers are unwind funclets inside the band, and
no game code calls it at all.** Its body is `void __stdcall f(void* p) { ::operator
delete(p); }` — MFC's `CObject::operator delete`, verbatim. A function only ever reached
from destruction paths is a *destruction helper*, and the caller census says so before any
byte comparison does. The fabricated definition was removed and the address labelled.
**+34 funclets.**

**`Tm_DestroyArray` was cl's vector destructor iterator.** The body at `0x11f640` walks an
array **backwards** calling `[ebp+0x14]` with `ecx` = element, and is itself `/GX`-protected
by `__ArrayUnwind` — that is `??_M@YGXPAXIHP6EX0@Z@Z`, cl's own helper, exactly. Our PDB had
a `reloc-alias` *guess* at HIGH confidence sitting over the anchored `??_M` row at LOW, so
the delinker spelled it a name no object emits. Swapping the two confidences let every
`delete[]` site and its funclet co-name. **+13 funclets and +13 ordinary functions.**

**A near miss worth keeping:** `CRezArchiveType`/`CRezArchiveDir` declared class-level `operator
new`/`operator delete` that merely forwarded to the globals. Retail's funclet calls the
**global** `??3@YAXPAX@Z`; a forwarder *cannot* produce that, because cl emits
`??3CRezArchiveDir@@SAXPAX@Z` and makes the funclet call that instead. The funclet target
discriminates a forwarder from a direct call — a distinction invisible in the forwarding
function's own bytes.

---

## How to use this

1. `gruntz.delink.eh_band --census --top 40`.
2. Work the **`different-targets`** rows first. Each says: *the object at this frame slot is
   not the type we think it is.* Read the two jump targets; the retail one names the truth.
3. `frame-offset` rows are the weaker sibling — our locals are laid out differently (an
   extra or missing local, or a different declaration order).
4. `unwind-count-differs` (via `--check`, which compares `maxState` out of both FuncInfo
   records) is the strongest structural claim available: our function constructs a
   **different number of destructible objects** than retail's.

**Corollary, and the reason this file exists:** a class that adds no fields and no behaviour
is invisible to every metric *except* the destruction path. If a wrapper, an intermediate
base, or a forwarding `operator delete` is fabricated, the funclets are the only place the
binary says so. Read them before believing a class exists.

*(The same reasoning currently indicts `CLoadable` as a fake intermediate over `CWapObj` —
zero added fields, no RTTI covering its 20+ subclasses, and 31 funclets destroying the base
sub-object as `CWapObj`. Open at time of writing.)*
