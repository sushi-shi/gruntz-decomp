# A MEMBER cursor walk: latch `GetAt`'s node, THEN advance — or cl CSEs the two loads into one

tags: cpp:loop cpp:member cpp:mfc | asm:mov | topic:codegen-idiom
symptoms: cursor read twice, mov edx [this+N] duplicate load, GetNext GetAt CSE, list walk advance one register
confidence: 9/10

## Symptom

A `CObList`/`CPtrList` walk whose POSITION cursor lives in a **member** (not a local)
is byte-exact everywhere except the advance at the bottom of the loop, where retail
loads the cursor member **twice** and the recompile loads it once:

```asm
; retail - two loads of [this+0x84], and a separate mov into the node register
  mov eax,[ecx+0x84]      ; guard + GetAt
  test eax,eax
  je   ...
  mov edx,[ecx+0x84]      ; GetNext's own load
  mov eax,[eax+0x8]       ; node->data
  mov esi,eax
  mov edx,[edx]           ; node->pNext
  mov [ecx+0x84],edx

; base - one load, reused
  mov eax,[ecx+0x84]
  test eax,eax
  je   ...
  mov esi,[eax+0x8]
  mov edx,eax
  mov eax,[edx]
  mov [ecx+0x84],eax
```

## Cause

`GetAt(POSITION)` takes the position **by value**; `GetNext(POSITION&)` takes it **by
reference**, so its inlined body reloads through that reference. Whether cl folds that
reload into the guard's load depends on what sits between them: assigning the walk
variable directly from `GetAt` leaves the two loads adjacent with nothing in between and
cl commons them. Latching `GetAt`'s result into a fresh local and assigning the walk
variable *after* the advance separates them, and both loads survive — which is retail.

The `mov esi,eax` retail emits (rather than `mov esi,[eax+8]` straight) is the same
latch showing up: the node lands in a temp before it becomes the loop variable.

## Fix

```cpp
// ONE load - cl commons GetNext's reference reload with the guard's
if (m_selId != 0) {
    payload = static_cast<T*>(m_list.GetAt(m_selId));
    m_list.GetNext(m_selId);
} else {
    payload = 0;
}

// TWO loads - retail
if (m_selId != 0) {
    T* next = static_cast<T*>(m_list.GetAt(m_selId));
    m_list.GetNext(m_selId);
    payload = next;
} else {
    payload = 0;
}
```

A `POSITION pos = m_selId;` local for the guard is **not** the lever — cl proves it
equal to the member and commons anyway (measured).

## The companion lever: the text CString is a TEMPORARY, not a scoped local

The same list-box fillers pass `node->GetName()` to `LB_ADDSTRING`. Retail destroys that
CString **inside the full-expression**, immediately after `SendMessageA` returns and
*before* the `!= -1` test:

```asm
  lea  edx,[esp+0x24] ; the temp
  mov  ecx,esi ; call GetName
  mov  eax,[eax]                 ; <- text read out of the RETURNED pointer
  ...
  call SendMessageA
  lea  ecx,[esp+0x24]
  mov  edi,eax                   ; result must survive the dtor -> callee-saved reg
  mov  [esp+0x1c],0xffffffff
  call ~CString
  cmp  edi,0xffffffff
```

```cpp
// scoped local -> dtor at the end of the block, text re-read from the temp's slot
{ CString name = node->GetName(); idx = SendMessageA(h, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)name); }

// temporary -> dtor right after the call, text from GetName's returned pointer
i32 idx = SendMessageA(h, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)node->GetName());
```

Forcing the add-string result to outlive the dtor is what pushes it into a callee-saved
register, and that in turn is what spills `this` to a frame slot in retail's
`CNetMgr::PopulateSessionList`. The whole register assignment falls out of the
temporary's lifetime — do not chase it as a colouring coin-flip.

## Evidence (2026-07-28, `src/Net/NetMgr.cpp`, unit `netmgr`)

| function | before → after | levers |
|---|---|---|
| `CNetMgr::PopulatePlayerList` @0x178d40 | 78.60 → **100.00 EXACT** | temporary + latch |
| `CNetMgr::PopulateSessionList` @0x178790 | 93.27 → **100.00 EXACT** | latch |
| `CNetMgr::FindProvider` @0x179270 | 96.23 → **100.00 EXACT** | latch |
| `CNetMgr::PopulateProviderList` @0x178470 | 78.60 → 97.30 | temporary + latch (x2) |

All four had been filed as regalloc/aliasing-conservatism walls
("not source-steerable", "the recompile derefs both from one register"). The
conservatism is real — it is just source-steerable after all.

## Related

- [`linked-list-walk-node-eax-rotation.md`](linked-list-walk-node-eax-rotation.md)
  — the earlier, wrong reading of this same residual.
- [`call-result-local-flips-callee-saved-set.md`](call-result-local-flips-callee-saved-set.md)
  — the other family where a named local for a call result changes the colouring.
- [`inlined-mfc-accessors-transcribed-as-raw-offsets.md`](inlined-mfc-accessors-transcribed-as-raw-offsets.md)
