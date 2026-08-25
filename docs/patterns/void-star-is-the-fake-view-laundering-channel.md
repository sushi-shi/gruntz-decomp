# A `void*` boundary launders a fake view past every cast ratchet

**Tags:** `cpp:cast` `cpp:class` `cpp:pointer` | `topic:correctness` `topic:tooling`
**Confidence:** 10/10 (four views dissolved, all four with the identical signature)

## Why the existing gates could not see them

The board counts `reinterpret_casts` (36) and the cast ledger classifies each
survivor. A fake view reached through `void*` shows up in NEITHER, because the
dangerous conversion is split into two halves that are each completely legal:

```cpp
// producer side - NO CAST AT ALL: every T* converts to void* implicitly
i32 r = Convert8To16(dc, src, node);          // node is a CImagePaletteNode*

// consumer side - ONE legitimate static_cast, the CORRECT spelling for void*
i32 CRezImage::Convert8To16(HDC dc, CRezImage* src, void* pal) {
    PALETTEENTRY* palette = (static_cast<ScanlinePalette*>(pal))->m_colors;
```

`static_cast<T*>(void*)` is not a smell - it is the only right way to recover a
type from `void*`, so a ratchet cannot flag it without flagging correct code.
No `reinterpret_cast` is ever written, the invented struct is genuinely *used*
(it has member accesses), and the dual reading of one storage is invisible
because each function sees exactly one type. Every fake-view detector we have
keys on something this shape does not do.

## The signature, and all four instances

**A `void*` parameter or return whose value is `static_cast` to a struct that
exists only to name that access.** Measured 2026-08-18, each dissolved to a
class that already existed:

| invented view | `void*` boundary that hid it | real class |
|---|---|---|
| `ScanlinePalette` | `Convert8To16(…, void* pal)` param | `CImagePaletteNode` (`char m_pad0[8]` + `PALETTEENTRY[256]` IS `HPALETTE` + `LOGPALETTE::palPalEntry` at +8) |
| `SbzDeviceList` | `CreateDeviceGroup(CInputDevBase**, ...)` return | `CFixedPtrArray32` (same three fields, same offsets) |
| `CRandomAmbientWorld` | `CWorldSoundSet(void* world)` param | `SoundCueRegistry` (`CWapObj` is the 0x10 pad) |
| `MpSymItem` | `CRezArchiveDir::FirstType/FirstEntry/NextEntry` returns | a one-field view of `CRezArchiveEntry` |

The `CRezArchiveDir` case is the clearest: thirteen navigation accessors returned
`void*` while `CHashElement`'s union already named all three real types, and
every caller cast straight back. Typing the accessors removed **79** `void*`
sites and ~50 casts, and all thirteen stayed byte-exact.

## The rule

**Treat a `void*` declaration as an unaudited boundary, not as hygiene debt.**
Its count is a DETECTOR metric: every remaining one is a place where an invented
type can sit without tripping a single gate. Retype the declaration and the view
either dissolves into the real class or fails to compile - both outcomes are
information.

Order of attack that worked: take the `void*` whose body immediately casts (that
cast names the type), then the ones whose callers all pass one concrete type.
Retype the DECLARATION - a change that turns one cast in the callee into two at
the call sites is a loss.

## Where `void*` is right, and stays

A real Win32/DirectX/SDK/ABI boundary (`LPVOID`, `WPARAM`/`LPARAM` payloads,
callback contexts, surface locks); an allocator seam; and a genuine
byte-cursor-to-record boundary, where callers walk a packed buffer with a `char*`
advanced by a variable stride - `CGameLevel::ReadImageSet` keeps `void*` and
casts once, while `CTileImageSet::Parse` and all three overrides take
`WwdTileImageRecord*`, taking four casts to one.

## Related

[`xref-identity-recovery-over-fake-views`](xref-identity-recovery-over-fake-views.md),
[`phantom-view-dissolution-recipe`](phantom-view-dissolution-recipe.md),
[`model-the-class-not-the-view`](model-the-class-not-the-view.md).
