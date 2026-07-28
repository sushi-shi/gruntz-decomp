# Derived member values: STORE INTO THE MEMBER, don't route through a local

**Tags:** `cpp:local` `cpp:method` | `asm:inc` `asm:lea` `asm:mov` | `topic:codegen-idiom` `topic:regalloc`
**Confidence:** 9/10

## Symptom

A short block that derives two or three values and parks each in a member reads
naturally as

```cpp
i32 width  = m_bounds.right  - m_bounds.left + 1;
i32 height = m_bounds.bottom - m_bounds.top  + 1;
m_viewW   = width;
m_viewH   = height;
m_anchorX = width  / 2;
m_anchorY = height / 2;
```

and stalls in the high-70s..low-90s with a diff that looks like pure register noise:

```
base                                  target
mov  eax,[ebp+0x58]                   mov  ecx,[ebp+0x58]
mov  ebx,[ecx]                        mov  edx,[eax]
mov  ecx,[ebp+0x5c]                   sub  ecx,edx
mov  esi,[ebp+0x54]                   mov  edx,[ebp+0x5c]
sub  eax,ebx                          lea  eax,[ecx+0x1]
inc  eax          <-- inc             mov  ecx,[ebp+0x54]
sub  ecx,esi                          sub  edx,ecx
mov  [ebp+0x70],eax                   mov  [ebp+0x70],eax
inc  ecx          <-- inc             lea  ecx,[edx+0x1]   <-- lea, not inc
```

Two tells: the base burns **extra callee-saved registers** (`ebx`, `esi` here) that
retail never touches, and it finishes each `+ 1` with `inc` where retail uses
`lea r,[x+1]`.

## Cause

The named locals give the value a live range that spans both derivations, so cl
computes *both* differences up front and needs two extra registers to hold them.
Retail interleaves: subtract, `lea` the `+1` into a fresh register (freeing the
subtrahend's register immediately), store, move on. Assigning the member directly
and then **reading the member back** for the dependent expression reproduces that
one-at-a-time schedule, because each value's live range ends at its own store.

## Fix

```cpp
m_viewW   = m_bounds.right  - m_bounds.left + 1;
m_viewH   = m_bounds.bottom - m_bounds.top  + 1;
m_anchorX = m_viewW / 2;
m_anchorY = m_viewH / 2;
```

## Evidence

`CDDrawWorkerHost::Build` (0x161e80) **90.43% -> 100.00 EXACT** on this change alone;
`CDDrawWorkerHost::InitGeometry` (0x1619f0, the same block twice) **78.33 -> 94.02**;
`CDDrawWorkerHost::Read` (0x161640) went byte-exact over the same span. All three had
been parked as a "zero-register-pinning wall" - it was a mislabeled source bug.

## Related

- [`zero-register-pinning.md`](zero-register-pinning.md) - the wall this masqueraded as.
  Check for the temporary FIRST before accepting a regalloc verdict.
