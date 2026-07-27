# Act-registrar create path: feed the name-slot lookup the GLOBAL counter, not the local id
tags: cpp:local cpp:loop cpp:global | asm:mov asm:push asm:dec asm:test | topic:codegen-idiom
symptoms: `?RegisterActs@C*@@SAXXZ` / `RegisterXLogic_*` / `RegisterType` stuck at exactly
92.9130 / 91.1739 / 96.4783 / 79.0896%; diff shows `mov edi,[g_typeCounter]; push edi` where retail
has `mov eax,[g_typeCounter]; push eax; mov edi,eax`; and/or `mov ebp,[m_grown]; test ebp,ebp` where
retail has `mov ecx,eax; dec eax; test ecx,ecx; je; lea ebp,[eax+0x1]`
confidence: 10/10

The ~55 activation registrars all share one hand-repeated find-or-create block. Two independent
source slips kept the whole family off 100%; both were mis-filed as "register-pinning walls" in the
per-function `@early-stop` notes. Neither is a wall.

## 1. The counter is read TWICE from the global, and cl CSEs it

Retail, create path (`CLightFx::RegisterActs` @0x9d320, identical in every sibling):

```asm
call  <ActFindId>
mov   edi,eax
test  edi,edi
jne   done
mov   eax,ds:[g_typeCounter]   ; ONE load, CSE of two source reads
mov   ecx,<&g_buteTree>
push  eax                      ;   -> the ActInsertId argument
push  <key>
call  <ActInsertId>
mov   esi,ds:[g_typeCounter]   ; reloaded (a call sits between)
mov   eax,ds:[g_typeColl.m_lo]
cmp   esi,eax
mov   edi,esi                  ;   -> id, must survive the next call => callee-saved
mov   dword ptr ds:[g_typeColl.m_grown],0
```

The tell is the **copy**: cl materialises the value in a scratch register for the push and then
copies it into the callee-saved home of `id`. That only happens when the *name-slot lookup argument*
is another read of the global — the CSE has two consumers with different lifetimes. Write it as

```cpp
ActInsertId("A", g_typeCounter);              // arg = the GLOBAL
id = g_typeCounter;                           // and so is the id latch
CString* slot = ActNameLookup(g_typeCounter);  // <- the GLOBAL, NOT `id`
```

Passing `id` instead gives the lookup a single consumer, so cl coalesces the load straight into the
callee-saved register (`mov edi,[g_typeCounter]; push edi`) and drops the copy. Costs ~2.7% per
block — ×6 blocks in `RegisterWarlordActions` it was the entire residual.

Two further slips in the same family, both the same bug wearing a different shirt:
- `id = g_typeCounter; ActInsertId(key, id);` — the argument must be the global, so the local's
  live range starts *after* the load, not at it. Reorder to insert-then-latch.
- an extra `i32 key = g_typeCounter; id = key; Lookup(key);` scratch local — collapse it.
- the latch **missing entirely** (`ActInsertId(...); CString* slot = Lookup(g_typeCounter);` with no
  `id = g_typeCounter`). That is also a live logic bug: the create path registered the handler at
  slot 0. Retail's `mov edi,esi` is the missing line.

## 2. The scratch-slot free loop is POST-decrement

Retail runs the `m_grown` fixup as `while (n-- != 0)`, i.e. load, pre-decrement, test the OLD value,
recover the trip count with `lea`:

```asm
mov   ebx,ds:[g_typeColl.m_alloc]
mov   eax,ds:[g_typeColl.m_grown]
mov   ecx,eax
dec   eax
test  ecx,ecx
je    skip
lea   ebp,[eax+0x1]
```

The peeled spelling `if (n != 0) { do { … } while (--n); }` emits a plain `test ebp,ebp; je` and
misses the `dec`/`lea` pair. Write the loop top-tested with the post-decrement:

```cpp
i32 n = g_typeColl.m_grown;
CString* list = ActNameSlots();
while (n-- != 0) {
    if (list != 0) {
        list->CString::~CString();
    }
    list++;
}
```

(Same idiom as [`test-old-value-decrement-loop-while-postdec.md`](test-old-value-decrement-loop-while-postdec.md);
this file records where the family lives and that it is the *registrar* archetype.)

## 3. Not every block in a registrar uses the SAME accessor — check the last one

`RegisterActs_644af0` (0x5be30, 19 keys) still sat at 97.49% after §1, with one block's worth
of residual. The keys are *not* interchangeable: `g_typeColl` has **two** index accessors and the
devs used both in the same function.

| retail call | via ILT | what it does |
|---|---|---|
| `_zvec::IndexToPtr` @0x312a0 | 0x3864 | the plain base accessor; the caller open-codes the `m_grown` free loop after it |
| `_zdvec::IndexToPtr` @0x310f0 | 0x437c | the DERIVED accessor — the same index math with the free loop **already inlined in its body** (@0x31156) |

Eighteen blocks call the base form and open-code the loop. The nineteenth calls the derived form
and has **no loop at the site at all**:

```asm
call  <ActInsertId>
mov   esi,ds:[g_typeCounter]   ; id  -- ONE consumer, so cl loads straight into callee-saved esi
push  <key>                    ; the operator= arg, pushed early
push  esi                      ; the accessor arg = `id`, NOT the global
mov   ecx,<&g_typeColl>
call  0x437c                   ; _zdvec::IndexToPtr
mov   ecx,eax
call  <CString::operator=>
inc   ds:[g_typeCounter]
```

Note the argument flips back to `id` here — §1's "always pass the global" is not a blanket rule,
it is a *readout of the consumer count*. `push eax; mov edi,eax` (materialise + copy) means two
consumers ⇒ pass the global; `mov esi,[g]; push esi` (load straight into a callee-saved home)
means one ⇒ pass the local. Read the tell, do not apply the rule blindly.

Give the odd block its own macro rather than open-coding it, and factor the shared
`*pool.Resolve(id) = reinterpret_cast<CActHandler>(handler)` tail into a third macro both call —
otherwise the duplicated PMF reinterpret trips the cast ratchet.

**97.49% → 100.00% EXACT.**

## Result

Applied mechanically across the family (31 + 13 + 7 + 11 + 6 sites): **2838 → 2880 exact**
(+42 functions), overall fuzzy 75.74% → 75.93%, zero regressions. Every one of them carried an
`@early-stop` claiming a "register-pinning / slot-vs-id callee-saved coloring wall". Re-audit an
`@early-stop` before believing it.

related: test-old-value-decrement-loop-while-postdec.md, zero-register-pinning.md,
predecrement-guard-lea-recover-count.md
