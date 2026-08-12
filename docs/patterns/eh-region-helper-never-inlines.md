# cl5 never inlines a helper that owns an EH region — spell `new X(...)` at every site

- **confidence**: 9/10
- **tags**: `cpp:new` `cpp:eh` `cpp:inline` `cpp:ctor` | `asm:push` `asm:call` | `topic:codegen-idiom`
- **symptoms**: several sibling functions are missing retail's whole `/GX` prologue
  (`push -1; push <handler>; mov eax,fs:0; push eax; mov fs:0,esp`); the base emits a
  plain `call <helper>` where retail expands an `operator new` + a full inline ctor
  chain; per-function fuzzy sits in the 15–50 % band with the CFG otherwise right.

## Mechanism

A `new`-expression on a class with a destructible sub-object needs an EH region: on a
throw inside the constructor the raw memory must go back through `operator delete`.
MSVC 5.0 tracks that with a per-frame state variable (`mov [esp+N],0` after the
allocation, `mov byte [esp+N],1` before each further sub-object ctor, `mov [esp+N],-1`
when construction completes) and a `__except_list` frame.

**cl 5.0 will not inline a function that owns such a region.** Marking the helper
`static inline` changes nothing: the helper is emitted out of line and the caller gets a
bare `call`, so the caller has *no* `/GX` frame at all. There is no `__forceinline` in
MSVC 5.0 to override it (see [`ob1-inline-budget-divergence`](ob1-inline-budget-divergence.md)).

Therefore, when retail shows the same allocate-and-construct block expanded into N
sibling functions, the original source **spelled it out N times**. Factoring it into a
helper is not a refactor the compiler can undo.

## Recipe

1. Confirm the shape: retail has the `/GX` prologue and `push <sizeof>; call ??2@YAPAXI@Z`
   inline; your base has neither and shows one `call`.
2. Delete the helper. Paste the `new` + the follow-up logic into every slot body.
3. Give the class a real **inline** constructor so the ctor chain expands too. Read the
   store order straight off retail — `[base ctor (vptr + base body)] [member sub-object
   ctors] [derived vptr stamp] [derived ctor body]` — and remember cl5 materializes
   actual arguments right-to-left, so the argument whose load appears **first** is the
   **rightmost** parameter.
4. If the map/array store afterwards loads the value *after* the container call in your
   base but *before* it in retail, use the by-value setter (`SetAt(key, v)`) rather than
   `map[key] = v`: the setter's argument is materialized at the call, which is what puts
   the load first. See [`mfc-container-band-table`] for which container you actually have.

## Evidence

`CDDrawWorkerRegistry::DispatchKeyed38/34/30/2C` (0x154ae0 / 0x154be0 / 0x154ce0 /
0x154df0) and `CDDrawWorkerRegistry::InstallTree` (0x154f80) all inline
`new CDDrawWorker(m_ownerCtx, m_workersByName.GetCount())`:

```
154b24: 6a 6c              push 0x6c                  ; sizeof(CDDrawWorker)
154b26: e8 ..              call ??2@YAPAXI@Z
154b36: 89 5c 24 20        mov  [esp+0x20],ebx        ; EH state 0 (memory owned)
154b42: c7 06 30 fc 5e 00  mov  [esi],??_7CLoadable   ; base ctor
154b48: 89 46 04           mov  [esi+0x4],eax         ;   m_id    = map.GetCount()
154b4b: 89 5e 08           mov  [esi+0x8],ebx         ;   m_flags = 0
154b4e: 89 7e 0c           mov  [esi+0xc],edi         ;   m_ownerCtx = owner
154b51: 8d 4e 10           lea  ecx,[esi+0x10]        ; member CObArray ctor
154b54: c6 44 24 20 01     mov  byte [esp+0x20],0x1   ; EH state 1
154b59: e8 ..              call ??0CObArray@@QAE@XZ
154b5e: c7 06 e8 fb 5e 00  mov  [esi],??_7CDDrawWorker; derived vptr stamp
154b64: c7 46 64 9f 86 01  mov  [esi+0x64],0x1869f    ; derived ctor body
```

The four slots were previously filed as an unreachable wall in
[`positive-gate-enables-shrink-wrap`](positive-gate-enables-shrink-wrap.md)
("screen those out by the EH prologue the base lacks"). With the helper dissolved into
the five bodies plus an inline `CDDrawWorker(CDDrawSurfaceMgr*, i32)` ctor, all five went
**100 % EXACT** (38/34: 38.2 → 100, 30/2C: 39.6 → 100, InstallTree: 51.4 → 100; unit
65.5 → 97.6). The intermediate `static inline` helper *containing* the `new` measured
17 % — worse than the original — which is the diagnostic for this pattern.
