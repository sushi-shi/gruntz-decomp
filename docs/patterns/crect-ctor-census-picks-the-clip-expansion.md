# Count `call ??0CRect@@QAE@HHHH@Z` per bounds site to pick which `Clip` expansion it is
tags: cpp:ctor cpp:local cpp:macro mfc:crect | asm:call asm:lea asm:mov | topic:codegen-idiom
symptoms: `call 0x34a4`, `lea ecx,[esp+N]` before the four pushes, `mov ecx,[eax]` right after a
ctor call, `test edi,edi` on the address of a stack local, `GRID_RECT_BOUNDS`, `GRID_CLIP`,
`GRID_CLIP_INL`, `GRID_RECT_INLINE`, `m_gridW`, `m_gridH`, `IntersectRect`
confidence: 9/10

## Symptom

A grunt step/scan function that calls `CMapMgr::Clip` several times sits 5-20 points low, the
frame is several dwords off, and the `--blocks` topology is right. The four bounds macros in
`include/Gruntz/ScanGridMacros.h` all end in the same `IntersectRect` + `m_gridW`/`m_gridH`
tail, so they read identically in the source and are trivially mis-assigned to sites.

## The census

In a TU compiled with `<MfcNoInline.h>` the 4-int `CRect` ctor is an out-of-line
`call` (see [[out-of-line-crect-ctor-means-mfcnoinline-tu]]), so **every rect built by a ctor
is one reloc you can count**:

    gruntz sema disasm <rva> --target --lite | grep -c 'call *0x34a4'   # ILT thunk for ??0CRect@@QAE@HHHH@Z

Then read each site's `rb` and pick by the ctor count at that site:

| what the site shows                                                        | expansion            | ctors |
|---------------------------------------------------------------------------|----------------------|-------|
| `rb` built by the CTOR, no field stores, no `test <addr>,<addr>`            | `GRID_CLIP_NULL`     | 2     |
| `rb` by FIELD STORES + `test <addr>,<addr>` + one ctor in the else arm      | `GRID_CLIP_INL(src)` | 1     |
| `rb` by FIELD STORES, no null test, one ctor for the temp                   | `GRID_CLIP_INL(NULL)`| 1     |
| both rects by FIELD STORES                                                  | `GRID_RECT_INLINE`   | 0     |

The counts must add up to the census. On `CGrunt::StepGooSuckerBehavior` 0xf0e20 they do
exactly: 2 (0xf0e5c/0xf0e64) + 1 (0xf1224) + 1 (0xf1705) + 0 = the four calls in the body.

## Reading `Clip(NULL)`: three rects, not two

`CMapMgr::Clip(const RECT* src)` with a constant-NULL argument folds the `src != NULL` arm
away, and the surviving `else` is `ra = CRect(0,0,w,h)` -- an ASSIGNMENT from a temporary. cl
builds the temporary with a second ctor call and copies four fields **off the ctor's return
register**:

    call ??0CRect@@QAE@HHHH@Z      ; the temporary
    mov  ecx,[eax]                 ; <-- eax, not a frame slot: the copy source is the TEMP
    mov  [esp+0x34],ecx            ; ra.left
    ...

So the site has THREE 16-byte objects (`rb`, the temporary, `ra`) and the `IntersectRect`
src2 is a slot the copy never wrote. A `ra = rb;` model reads the copy source out of `rb`'s
own slot instead, which is the tell that the site was mis-assigned.

`GRID_CLIP`'s `new (&rb) CRect(...)` is NOT this shape -- placement new adds
`lea <reg>,&rb; test <reg>,<reg>; je` (see
[[explicit-ctor-call-has-no-placement-null-guard]]), and retail has no such guard at a
`Clip(NULL)` site.

## Caveat: the census can score WORSE than a wrong pairing

On StepGooSuckerBehavior the proven assignment scores 79.80 where putting one ctor at the
first site and two at the last scores 82.70 -- the wrong pairing happens to align better
against the delinked stream. Take the census; the bytes decide, not the percent.
