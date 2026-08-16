# A written-out `idx < min || idx > max` guard around `m_items.GetAt(idx)` IS the class's inline accessor

tags: cpp:inline cpp:member cpp:branch | asm:cmp asm:jl asm:jg | topic:codegen-idiom topic:mis-model
symptoms: a three-arm `if (idx >= p->m_minIndex && idx <= p->m_maxIndex) x = (CImage*)p->m_items.GetAt(idx); else x = NULL;`
next to a `CDDrawWorker*`; retail has ONE shared tail where the out-of-range path falls into the same
store as the in-range path; `gruntz walls diagnose` says REGALLOC but the base has 3-13 instructions
retail does not; an extra merged `return 0` block
confidence: 9/10 (5 positive sites, 3 negative controls, 2026-08-16)

`CDDrawWorker::GetAt` is an in-class inline that already exists in
`include/DDrawMgr/DDrawWorker.h`:

```cpp
CImage* GetAt(i32 index) {
    if (index < m_minIndex || index > m_maxIndex) { return 0; }
    return static_cast<CImage*>(m_items.GetAt(index));
}
```

Transcribing its body at the call site instead of calling it is byte-visible, because the
early `return 0` and the fall-through JOIN before the caller's store. Retail:

```asm
; CSBI_MenuItem::ResolveFrame 0xe81e0 - the `else` arm
cmp ecx,[eax+0x64] / jl  L0      ; index < m_minIndex
cmp ecx,[eax+0x68] / jg  L0      ; index > m_maxIndex
mov edx,[eax+0x14] / xor eax,eax
mov ecx,[edx+ecx*4]              ; in range: ecx = m_items[index]
L1: test ecx,ecx / mov [esi+0x30],ecx / setne al / pop esi / ret 8
L0: xor ecx,ecx / jmp L1         ; out of range: ecx = 0, SAME tail
```

The transcription instead materialises `m_frame = NULL` in its own arm and re-reads the member,
so it is 3+ instructions long and does not share the tail.

## Measured

| site | before | after |
|---|---:|---:|
| `CSBI_MenuItem::ResolveFrame` 0xe81e0 | 70.38 | **92.45** |
| `CSBI_GruntMachine::BuildResourceTabStatusBar` 0xe8a70 (2 sites) | 93.17 | **97.84** |
| `CSBI_ImageSet::SetupImage` 0xe72f0 | - | tail matches exactly |
| `CSBI_StatzTabGruntBar::Update` (5 sites) | 91.00 | 90.92 (byte-neutral cleanup) |
| `CMenuItem::Place` | 100.00 | 100.00 (byte-neutral cleanup) |

## The boundary - do NOT apply it where the receiver has its own null guard

Where the source also tests the RECEIVER, `p ? p->GetAt(i) : NULL` is **refuted**: it merges a
basic block retail keeps separate (retail inverts the branch and lays the null arm as the
fall-through - the C2 block-placement coin).

| site | written out | ternary |
|---|---:|---:|
| `CSBI_SideTab::BuildStatzTabStatusBar` (2 sites) | **80.19** | 77.05 |
| `CSBI_Image::SerializeFields` | **84.97** | 83.87 |

Keep the written-out form there. The two shapes are distinguishable up front: if the guard is
exactly `idx < min || idx > max` it is `GetAt`; if it also tests `p == NULL` it is not.

## Corollary: the argument null-guard order is readable, and cl preserves it

`cl 5.0 does not reorder the operands of a short-circuit ||` - controlled in
`CSBI_ImageSet::SetupImage`: source `owner || host` emits `[esp+8]` first, source
`host || owner` emits `[esp+0xc]` first. So the retail emission ORDER names the source order.
Retail tests **host first** in `CSBI_ImageSet::SetupImage`, `CSBI_MenuItem::SetupImage` and
`CSBI_GruntMachine::BuildResourceTabStatusBar` - host-first is the house order. Correcting it
in `CSBI_ImageSet::SetupImage` costs 74.63 -> 68.31 of CURRENT score, because cl then hoists the
first parameter load above `push esi` and rotates eax/ecx through the whole prologue; the order
is still what retail's bytes say. Two disposable spellings were inert (binding host to a local,
deleting the unused-parameter `static_cast<void>` no-op) and one was worse (two separate `if`
guards, 67.45 - retail shares ONE exit target for both).
