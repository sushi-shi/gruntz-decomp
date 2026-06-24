# /GX method with a scoped destructible local: EH frame is 4 B short + scope cookie differs
tags: cpp:eh cpp:local | asm:push asm:mov | topic:wall topic:eh
symptoms: body byte-identical, retail `sub esp,0x44` + `push 0x8` vs recompile `sub esp,0x40` + `push 0x0`, every `[esp+N]` shifted by 4, ~86%
confidence: 6/10

A NON-ctor/dtor method that holds one scoped C++ object with a non-trivial destructor gets a
`/GX` SEH frame so the object unwinds if an inner `call` throws (the destructor lives only in the
funclet — there is NO inline dtor call on the normal path). Modeling that object as a real local
with a `~T()` correctly produces the frame (push -1 / push handler / mov fs:0,esp), and the whole
body matches instruction-for-instruction — but the recompile reserves one fewer dword: retail
`sub esp,0x44` where ours is `sub esp,0x40`, and the SEH scope-table cookie pushed second is
retail `0x8` vs ours `0x0`. Every `[esp+N]` frame reference is then 4 bytes off, capping fuzzy%.

```cpp
i32 Foo::CountInRect(CWwdGrid* grid) {
    i32 count = 0;
    CWwdGridIter it;                       // ctor stamps the engine vtable inline
    for (CWwdObject* o = it.Start(grid, 0); o; o = it.GetNext()) { /* throwing calls */ }
    return count;                          // ~CWwdGridIter() -> /GX frame, dtor only in funclet
}
```
```asm
168460: push -1 ; push 0x5e2518 ; mov fs:0,esp ; sub esp,0x44   ; retail
   base: push -1 ; push 0x0     ; mov fs:0,esp ; sub esp,0x40   ; recompile (-4, cookie 0)
```
WALL — the extra trylevel dword + the scope-table state cookie are the /GX EH-state machine's
own numbering (cf. eh-state-numbering-base), not steerable from C; the local's struct size
(0x44) is already matched. Evidence: CWwdSpatialMgr::CountInRect (0x168460) 86%, body exact.
