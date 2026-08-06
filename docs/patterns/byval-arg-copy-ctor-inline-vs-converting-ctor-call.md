# Mixed by-value copy and assignment call can expose a shadowed SDK class

**Tags:** cpp:ctor cpp:local mfc:crect | asm:call asm:mov asm:sub | topic:mis-model topic:regalloc

## Symptom

A function passes a small MFC value class by value at two call sites. One path
builds a local with Win32 helpers and the compiler copies its four fields directly
into the outgoing argument. A later path calls a body at another RVA before making
the same by-value call.

It is tempting to invent a layout-compatible project class, describe the called
body as a converting constructor, and reinterpret the Win32 local as that class to
force the first copy to inline. That model can improve code shape while preserving
the wrong ownership and type identity.

## Reverse-audit signature

Check the mangled name before inferring a constructor. In `EngStr_RenderText`
(0x115930), the called body at 0x115b30 is:

```text
??4CRect@@QAEXABUtagRECT@@@Z
```

This is MFC 4.2 `void CRect::operator=(RECT const&)`, not a WAP32 constructor.
The other emitted rectangle bodies have ordinary MFC signatures as well. A
project-local `CRect : tagRECT`, a cast between `RECT` and that class, and a
source-owned TU for those bodies are therefore one connected shadow-model defect.

## Correct model

Use MFC's `CRect` through `<MfcWin.h>`. A normal named value can serve both paths:

```cpp
CRect rect;
if (shadow) {
    CopyRect(&rect, rc);
    OffsetRect(&rect, 2, 3);
    renderer.DrawWrapped(text, surface, rect, 1, flags, 0);
}
rect = *rc;
renderer.DrawWrapped(text, surface, rect, 1, flags, 0);
```

The first by-value argument uses the real class's copy construction; the second
path invokes its real `RECT` assignment operator. Do not use a layout reinterpret
to steer the two paths. Class ownership, mangling, and the relocation target
outrank the current fuzzy score.

## Withdrawn claim

The earlier version called the type `WapRect`, classified 0x115b30 as an external
engine converting constructor, and endorsed `*(WapRect*)&rect`. Those claims were
falsified by the decorated symbol and the MFC 4.2 header definition. The score gain
was produced by a shadow class, not evidence that the cast was authentic.

## Confidence

c10 — the MFC decorated names, SDK declaration, call target, and consumer source
shape agree.
