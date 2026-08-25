# A repeated per-level reset in a dtor chain survives only as an inline MEMBER call

tags: cpp:dtor cpp:inline cpp:member cpp:class | asm:mov | topic:codegen-idiom
symptoms: retail dtor stores the same [this+N] pair two or three times with no
call between, both constants (0x80000000/-1) materialized once at the top; a
flattened transcription needs volatile pointers to keep the duplicates; the
honest per-level plain-store chain gets dead-store-eliminated to one pair
confidence: 9/10

The CDDrawPixelWorker/B dtors (0x1570d0/0x157240) write the WwdDirtyRect reset pair
(`m_rect.left = COORD_UNSET; m_armed = -1`) THREE times: once in the derived
dtor body, once in ~CDDrawPlacedWorker, once in ~CResolveNode - the classic
copy-paste-per-level dev shape, all inlined into one emitted function. Two
reconstructions fail:

- a flattened single body with `volatile LONG*` pointers reproduces the stores
  but mis-schedules the constant materialization (72.47) - and volatile is a
  steering device, not a model;
- the honest chain with PLAIN member stores per level lets our cl prove the
  earlier pairs dead (same [ecx+off] addresses, no intervening calls) and
  collapse them (34.00).

What retail's compile actually saw is a tiny inline MEMBER function per reset:

```cpp
struct WwdDirtyRect {
    void Reset() {
        m_rect.left = COORD_UNSET;
        m_armed = -1;
    }
};
~CDDrawPixelWorker()    { m_pixelValue = 0; m_dirty.Reset(); }
~CDDrawPlacedWorker() { m_dirty.Reset(); }        // new inline level
~CResolveNode()     { m_screenX = COORD_UNSET; m_dirty.Reset(); }
```

Each Reset() expansion stores through its own `this`-derived pointer temp, and
cl5's dead-store elimination does not prove the three expansions alias - all
three pairs survive, the two constants CSE to one materialization each, and the
schedule (`mov edx,0x80000000; or eax,-1;` before the body's byte store) falls
out. Both dtors went 72.47 -> 100.00 EXACT in one build. The intermediate
vptr stamps of the inlined base dtors die naturally (consecutive [ecx] stores,
no calls between), so only the final ??_7CObject stamp remains - VTBL_ABSENT
stays satisfiable for the base.

Reading rule: a REPEATED store pair inside one dtor is one inline helper called
once per hierarchy level, never a volatile hack and never a single flat body.
