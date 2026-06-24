# Scope a destructible temp to elide the /GX EH frame when no throwing call is live during it
tags: cpp:eh cpp:local cpp:dtor | asm:push asm:mov | topic:codegen-idiom topic:eh
symptoms: spurious push -1 / push handler / mov fs:0,esp; CString temp; frameless retail; rep movs strcpy
confidence: 8/10
variants: split-tu-eh-dtor-vs-frameless-cstring.md

A function takes a CString (or any destructible) temp, uses it, then makes
further calls (a sink method, memcpy, etc.). Writing the temp at function scope
keeps it live across those later calls, so MSVC 5.0 /GX wraps the whole body in a
C++ EH frame (`push -1; push handler; mov fs:0,esp`) to unwind the temp if one of
them throws — but retail has NO frame. MSVC only emits the frame when a
potentially-throwing call is live WHILE the destructible is in scope. Restrict the
temp to an inner `{ }` block that ends right after its last use (before the later
calls): with no throwing call overlapping the temp's live range, /GX elides the
frame and the body comes out frameless, matching retail.

```cpp
// BAD: name lives to end of fn -> EH frame around Store()/EngineCopy()
CString name = GetLevelName();
strcpy(dst->m_75, (const char*)name);
sink->Store(dst, src);          // "throwing" call while name still live -> /GX frame
// GOOD: scope the temp so it dies before the later calls
{
    CString name = GetLevelName();
    strcpy(dst->m_75, (const char*)name);
}                                // ~name here; no throwing call during its live range
sink->Store(dst, src);           // frameless
```
```asm
; BAD: 6a ff / 68 <h> / 64 a1 00.. / 64 89 25 00..  (push -1; push handler; mov fs:0,esp)
; GOOD: 51 53 55 .. (plain prologue, no fs:0)        <- retail
```
STEERABLE. CGruntzMgr::FillSaveInfo (0x927b0): scoping the name temp removed the
EH frame, 59.3%→92.8% (then →99.2% with an out-of-line EngineCopy for the +0x14
snapshot copy). Distinct from split-tu (that splits the unit); here one inner
block suffices.
