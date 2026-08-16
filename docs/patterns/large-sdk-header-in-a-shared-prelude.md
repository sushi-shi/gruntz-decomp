# A large SDK header in a shared prelude is a cl 5.0 cliff, not a style choice

`cpp:include msvc5:c1 msvc5:sdk | topic:tu-state topic:correctness topic:wall`
confidence 9/10

## Symptom

Two unrelated-looking failures follow the same edit — adding one big SDK header
to a header that many TUs pull:

1. **Bogus member lookup.** cl 5.0 SP3 stops resolving `this`-relative members
   PART WAY THROUGH one function body and reports `C2227: left of '->m_x' must
   point to class/struct/union`, `C2228: left of '.m_y' must have
   class/struct/union type` and `C2065: 'm_z' : undeclared identifier` for
   members it accepted a dozen lines earlier in the same function. The source is
   correct; nothing in the reported region changed.
2. **A broad below-bank sweep** in units that never reference anything the new
   header declares.

## Worked example — `timeGetTime`

Retail imports exactly one symbol from `WINMM.dll`:

```
WINMM.dll 1
    timeGetTime
```

The era SDK does declare it — `MMSYSTEM.H:1984`,
`WINMMAPI DWORD WINAPI timeGetTime(void);` — inside `#ifndef MMNOTIMER`.
`<windows.h>` includes `<mmsystem.h>` only when `WIN32_LEAN_AND_MEAN` is
undefined (`WINDOWS.H:132..165`), and `<afxv_w32.h>:71` defines
`WIN32_LEAN_AND_MEAN` unconditionally — so an MFC app of the era had to pull
`<mmsystem.h>` back by hand. Doing that is what hits the cliff.

### Placement A — the shared umbrella

`#include <mmsystem.h>` added to `<Mfc.h>` and `<Win32.h>`:

| | exact | overall fuzzy | fresh below-bank |
|---|---|---|---|
| base | 3549 | 94.01% | 0 |
| + `<mmsystem.h>` in both umbrellas | 3545 | 93.97% | **61 across 49 units** |

Of those 49 units, **47 never call `timeGetTime`** — `lightfxrender`,
`spriteref`, `savegame`, `typekeycoll`, `font`, `shadetablecache`, `ddsurface`,
`cimage`, … Only `play` and `wwdobjmgr` are real users. That ratio is the
signature: the header is not paying for itself where it landed, it is just
moving C1's handle state under every TU.

### Placement B — the header that actually needs it

`<Gruntz/AniRecordView.h>` holds `CAniRecordView::GetRandomNumber`, an in-class
inline whose body is `static long holdrand = timeGetTime();`, so it needs the
declaration. `<Gruntz/AniElement.h>` includes it, and `src/Gruntz/Grunt.cpp`
includes that. Adding `#include <mmsystem.h>` to `AniRecordView.h`:

```
Grunt.cpp(827) : error C2228: left of '.GetBuffer' must have class/struct/union type
Grunt.cpp(828) : error C2227: left of '->ApplyLookupSprite' must point to class/struct/union
Grunt.cpp(835) : error C2227: left of '->m_animCursor' must point to class/struct/union
Grunt.cpp(836) : error C2065: 'm_poseWalk' : undeclared identifier
Grunt.cpp(842) : error C2109: subscript requires array or pointer type
Grunt.cpp(847) : error C2679: binary '=' : no operator defined which takes a
                 right-hand operand of type 'struct GruntDirectionCell'
```

against source that is unchanged and that compiles without the include:

```cpp
    m_value = m_wwdObject->m_animCursor.m_animation;        // line 814 - FINE
    ...
    const char* nm = m_cells[index].IdleName().GetBuffer(0); // line 827 - C2228
    m_wwdObject->ApplyLookupSprite(nm, frame);               // line 828 - C2227
```

Line 814 resolves `m_wwdObject->m_animCursor`; line 835 does not. The class
context is intact at the top of the body and gone at the bottom.

### Negative controls (same slot, same TU)

| header in `AniRecordView.h` | lines | `Grunt.cpp` |
|---|---|---|
| `<commdlg.h>` | 717 | compiles |
| `<mmreg.h>` | 1623 | compiles |
| `<mmsystem.h>` + `#define MMNOMCI` | ~2520 | **fails, same errors** |
| `<mmsystem.h>` | 3742 | **fails, same errors** |

So it is bulk, not a macro or name collision from `mmsystem.h` in particular.
The cliff for this TU sits between roughly 1600 and 2500 lines of added SDK
declarations.

## Why the MMNO* knobs do not rescue it

`<mmsystem.h>` advertises `MMNODRV`/`MMNOSOUND`/`MMNOWAVE`/… to suppress its
sections, and they are era-authentic. They are still a trap in a shared header:

* `WAVEFORMATEX` is defined at `MMSYSTEM.H:711..731`, i.e. INSIDE the
  `MMNOWAVE` section (`514..852`), and `<dsound.h>` needs `LPWAVEFORMATEX`.
* `mmsystem.h` has an include guard, so whichever TU parses it FIRST fixes the
  choice for that whole TU. A prelude that defines `MMNOWAVE` silently breaks
  any DirectSound unit downstream of it.
* Suppressing enough to clear the cliff means suppressing `MMNOWAVE`.

## The rule

* A big SDK header belongs in the `.cpp` that calls into it, never in a
  platform prelude or a widely-included project header. `GruntzWnd.cpp`,
  `DirectSoundMgr.cpp` and `SoundStream.cpp` already carry `<mmsystem.h>` that
  way and cost nothing.
* When the declaration is needed by a HEADER whose closure contains a large TU,
  and the measurement above reproduces, transcribe the single SDK line with the
  measurement recorded HERE and a one-line pointer at the declaration. One
  documented exception beats an ambient one, and beats two.
* Detection: if the fresh below-bank units are overwhelmingly units that do not
  use the new header, the placement is wrong before the score is. Reach for
  the include site, not for the scores.
