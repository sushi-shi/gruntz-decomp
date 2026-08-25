# The assignment must be INSIDE the full-expression, or the temp dtors run first

tags: cpp:local cpp:call cpp:eh | asm:call asm:mov | topic:codegen-idiom

symptoms: a `CString`-temp-heavy statement sequence is instruction-for-instruction
  identical to retail EXCEPT that the store of the looked-up result into its
  destination lands AFTER the two `~CString` calls (and gets scheduled into the NEXT
  statement's argument setup), where retail stores it BEFORE them; repeated N times
  across an unrolled loader the plateau sits in the 70s

confidence: 9/10

## Shape

An out-parameter lookup written as two statements:

```cpp
CAniElement* out = 0;
MapLookup(map, "GRUNTZ_" + m_animSetName + sfx, out);   // full-expression ends here
(dst) = out;                                            // ... so the temps die BEFORE this
```

The two `CString` temporaries built by `operator+` are destroyed at the semicolon of
the `MapLookup` statement, so cl emits

```asm
call MapLookup
lea  ecx,[esp+0x2c] / mov byte [esp+0x20],bl / call ??1CString
lea  ecx,[esp+0x10] / mov [esp+0x20],ebp     / call ??1CString
mov  eax,[esp+0x28]        ; <- out, read only now
mov  [esi+0x394],eax       ; <- and sunk into the next statement's setup
```

Retail instead has

```asm
call MapLookup
mov  eax,[esp+0x28]
lea  ecx,[esp+0x2c]
mov  [esi+0x394],eax       ; <- store BEFORE the dtors
mov  byte [esp+0x20],bl
call ??1CString
...
```

which is only reachable if the assignment is part of the **same full-expression** as
the temporaries — i.e. the lookup returns the value:

```cpp
static inline CAniElement* FindAnimElement(CMapStringToPtr& map, LPCTSTR key) {
    CAniElement* out = 0;
    MapLookup(map, key, out);
    return out;
}

#define LOAD_POSE(dst, sfx) \
    ((dst) = FindAnimElement(map, "GRUNTZ_" + m_animSetName + (sfx)))
```

The inlined helper keeps the `out` stack slot and both `~CString` calls exactly where
they were; only the assignment moves ahead of them, because temporaries are destroyed
at the end of the full-expression and the assignment is part of it.

## Evidence

`CGrunt::LoadAnimNameTable` @0x00049c60 (grunt TU) — the macro is expanded 19 times, so
the one-statement-vs-two-statement difference moved the whole function
**77.30% -> 99.90%** in a single edit.

`CAniRecordView::ResolveIndices` @0x00168d00 adds the C1 control. Retail passes
`tokens.GetAt(i)` directly to `Lookup`, stores the output into `m_cues[i]`, and only
then destroys the returned `CString`. Spelling the tail as two statements destroys
the temporary before the store and scores 93.42%. Naming the CString scores 97.07%
but swaps its cleanup home with the output pointer, leaving a uniform `+0x4` EH row.
The single expression

```cpp
void* v;
v = 0;
m_cues[i] = (map.Lookup(tokens.GetAt(i), v), static_cast<SoundCue*>(v));
```

puts the unnamed temporary and output pointer in retail's homes and makes both unwind
funclets exact. The body settles at 96.86% because cl schedules `v = 0` before GetAt
where retail sinks it between the Lookup pushes; that remaining store-placement coin
does not outweigh the exact lifetime and C1 structure.

## Recognizing it

Any `out`-parameter helper wrapped in a macro/loop where the diff shows your store of
the result appearing one or two statements LATE (typically already merged into the
following statement's `push`/`lea` setup) while everything else lines up. Ask whether
retail's spelling could have been `x = f(...)` rather than `f(..., x); use(x);` — the
temp-destruction point is the tell, not the scheduling.
