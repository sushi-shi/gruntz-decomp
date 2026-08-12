# `header + 1` overshoots a struct with a trailing `u8 payload[1]`

**Tags:** `cpp:struct` `cpp:pointer` | `topic:correctness` `topic:codegen-idiom`
**Confidence:** 10/10 — proven against retail bytes in `CDDrawShadeBlit::Build` (0x1490d0).

## Symptom

The disassembly shows retail advancing a header pointer by the header's **payload
offset** while we advance it by `sizeof`:

```
retail:  add  esi, 0x20        ; esi = pid + 0x20
ours:    add  esi, 0x24        ; esi = pid + 0x24
```

`gruntz.audit.immediates --strong` is what makes this visible: `0x24` appears
nowhere in retail's 371 bytes, and `0x20` nowhere in ours. Nothing else reports
it — `--diff` masks the operand class, `store_offsets` sees where the copy lands
and not where it starts, and the reloc-addend sieve only covers relocated
operands. The `%` cost is a fraction of one percent, so it survives any amount of
matching work.

## Mechanism

A file-header struct whose payload is declared as a one-element trailing array:

```cpp
struct PidHeader {
    u32 formatTag;  PidFlags flags;
    i32 width;      i32 height;
    i32 offsetX;    i32 offsetY;
    u32 fill;       u32 unk1;      // 8 dwords -> payload begins at 0x20
    u8  pixels[1];
};
```

`pixels` is at **0x20**, but `sizeof(PidHeader)` is **0x24**: 0x21 bytes rounded
up to the struct's 4-byte alignment. So `src + 1` — the reflex spelling for "skip
the header" — skips **four bytes too many**, and every byte of the payload is read
shifted. The rest of the same function had it right (`size - 0x20`, and the
embedded palette read at `pid + m_rleLen + 0x20`), which is the tell: one site
disagreeing with its own neighbours about the header size.

This is a runtime defect, not a matching artifact. In `Build` it shifts the whole
RLE stream of every 8-bit PID sprite that goes through the shade-blit path.

## Fix

Name the member. It is the offset, it needs no cast, and it cannot drift when a
field is added:

```cpp
memcpy(m_rleData, src->pixels, m_rleLen);   // +0x20
// NOT: memcpy(m_rleData, src + 1, m_rleLen);   // +0x24
```

The same rule kills the neighbouring `RecordBytes<PidHeader>` union view: the
palette sits directly after the payload, so it is `src->pixels[m_rleLen + i]`,
which is exactly retail's `pid + m_rleLen` formed once and indexed at +0x20.

## Where else to look

Any `X + 1` / `(X*)p + 1` on a type that ends in a trailing array, and any type
whose retail extent is smaller than its C++ `sizeof` — that gap IS the
padding `+ 1` will step over. `PidHeader` (retail 0x20, `sizeof` 0x24) is the
one proven instance; the pattern generalises to every `T hdr; u8 data[1];`
reader in the image and REZ paths.
