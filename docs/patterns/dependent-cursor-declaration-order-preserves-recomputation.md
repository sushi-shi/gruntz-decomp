# Direct plane indexing supersedes the apparent dependent-cursor order

tags: cpp:local cpp:pointer cpp:order cpp:array | asm:lea asm:add | topic:codegen-idiom topic:source-oracle topic:negative-control
symptoms: two hand-reconstructed plane cursors improve a decoder substantially, but a
same-era surviving body indexes the planes directly and closes the remaining topology
confidence: 10/10 (the earlier cursor claim is falsified by surviving source and an
audited exact unchanged-source compiler state)

The first reconstruction used two walking cursors for adjacent PCX planes. Algebraically
sharing the nearer cursor:

```cpp
u8* green = scan + width * 2;
u8* blue = green + width;
```

lets cl 5.0 retain `green` as the common expression, but retail showed:

```asm
lea edx,[scan+width*2]  ; temporary used to form blue
mov esi,width
add esi,edx             ; blue
lea edi,[scan+width*2]  ; green recomputed independently
```

Declaring the farther cursor first and spelling both addresses from their common base:

```cpp
u8* blue = scan + width * 3;
u8* green = scan + width * 2;
```

selected retail's visible address sequence and moved `CRezImage::DecodePcxData` from
93.8608% to 97.7215%. That was a useful compiler-state observation, but it was not proof
that either cursor existed in the source.

The public tree's 1996 `libs/dibmgr/dib.cpp::CDib::InitPcx` later supplied the authored
layer. It has no channel cursors. It uses the shared loop index directly:

```cpp
*dst++ = scan[i - 1];
*dst++ = scan[width + i - 1];
*dst++ = scan[2 * width + i - 1];
```

Restoring that expression together with the surviving function-scope local census raised
the decoder from 97.6772% to 99.9240%. Base and retail then had the exact 399-byte extent,
158 instructions, three calls, eighteen branches, three returns, and three ordered
relocations. Only the first two x-bound loads were transposed. A 32-cell width/height
expression Cartesian produced one identical island; target-adjacent C1 forest trial 4
made the unchanged function exactly 100.0000. MAX was recorded while exact, the probe was
removed, and the humane direct-index body remains.

The old farther-first cursor form and a named red cursor are therefore negative controls.
An emitted address formation can reveal an IL ordering effect without proving a named
pointer local. Prefer a surviving abstraction when it accounts for the complete loop;
then use disposable compiler-state search only to bank an exact result for that unchanged
source, never to retain an inert probe.
