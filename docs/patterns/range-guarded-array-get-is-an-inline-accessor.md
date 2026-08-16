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

A SECOND refutation, measured 2026-08-16: the guard must produce **only a value**. Where each
arm carries its own control flow, retail duplicated the tail and folding is wrong -
`CKitchenSlime::LoadSprites` 94.75 -> **90.35** (reverted) for

```cpp
if (spr->m_minIndex <= 1 && spr->m_maxIndex >= 1) {
    CImage* img = (CImage*)spr->m_items.GetAt(1);
    player->m_frameIndex = 1; player->m_layer = img; m_stepMag = 0.0; return 1;
}
player->m_frameIndex = 1; player->m_layer = NULL; m_stepMag = 0.0; return 1;
```
even though the two arms differ only in the stored value: each has its OWN `return`.

## The rule, in the order to apply it

1. Guard is exactly `idx < min || idx > max` (or its De Morgan twin) -> candidate.
2. The arms differ ONLY in the value assigned to one variable, and both fall into shared
   code -> **it is `GetAt`; recover it.**
3. The guard also tests the RECEIVER (`p != NULL && idx >= ...`) -> **NOT `GetAt`.** Leave it.
4. Each arm has its own `return` / control flow -> **NOT `GetAt`.** Leave it.

A receiver null-test in a SEPARATE enclosing block (`if (spr) { ...GetAt shape... }`) does not
disqualify - that is `CWwdGameObjectA::ApplyName`, 89.41 -> **94.12**.

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

## When the recovery MOVES BYTES, and when it is cleanliness only (2026-08-16, 20-site sweep)

Applying the four steps to 20 further sites across 11 files produced **zero** byte movement -
gate green, 0 fresh, no function above or below its bank. So the rule is right but the payoff
is conditional:

* **It moves bytes when the out-of-range path JOINS the in-range path at a shared store/use**,
  because that is where retail's `return 0` falls into the caller's tail:
  `CSBI_MenuItem::ResolveFrame` 70.38 -> 92.45, `CSBI_GruntMachine::BuildResourceTabStatusBar`
  93.17 -> 97.84, `CWwdGameObjectA::ApplyName` 89.41 -> 94.12.
* **It is byte-neutral when the result goes to a plain local that is tested afterwards** - cl
  emits the same code either way. All 20 sites below were of this kind. Still worth doing (20
  casts and 40 raw `m_minIndex`/`m_maxIndex` comparisons removed, the real accessor used), but
  do not queue them expecting score.

Applied, byte-neutral: `SBI_StatzTabArrow` x5, `SBI_WarlordHead::Render` x3,
`CSBI_GruntMachine::Render` x2, `WwdFactoryObject` ClampFirst/ClampLast, `SBI_ImageSetAni::Render`,
`CAniPlayer::RenderCel`, `ChatBox` x2, `GruntToySprite`, `GruntHealthSprite`, `LightFx`,
`CSBI_MenuItem::SetState`.

## Step 0: a bare `m_items.GetAt(p->m_minIndex)` is NOT the accessor

Five sites spell the first element directly with **no range guard at all** (`MenuPage.cpp:267,454`,
`GruntzMgr.cpp:2393,2442`, `SBI_RectOnly.cpp:285`). Rewriting them as `p->GetAt(p->m_minIndex)`
would ADD a range test that retail does not emit - `CSBI_MenuItem::ResolveFrame`'s `a == -1` arm
is the proof, it is `mov ecx,[eax+0x64] / mov edx,[eax+0x14] / mov eax,[edx+ecx*4]` with no
compare. Leave them. Likewise `SBI_ImageSetAni.cpp:69,74` are min/max SELECTS
(`(b4 >= 0) ? tbl->m_minIndex : tbl->m_maxIndex`), not indexing at all.

## The recovery can UNLOCK a regalloc knob that the transcription blocked

`CSBI_ImageSetAni::Render` 0xe7b00 was 99.4444 with 90/90 instructions, the same size, and the
same call / branch / `ret` / relocation counts - the whole gap was `eax` and `ecx` swapped across
the range test. It closed to **100.00 EXACT** by deleting the receiver local:

```cpp
-   CDDrawWorker* tbl = m_frameSet;
-   CImage* cel = tbl->GetAt(m_frameIndex);
+   CImage* cel = m_frameSet->GetAt(m_frameIndex);
```

This is one-use-local-is-a-regalloc-knob.md in its DELETE direction, and it only became available
after the accessor was recovered: the written-out form names the receiver three times
(`tbl->m_minIndex`, `tbl->m_maxIndex`, `tbl->m_items`), so the local cannot be removed and the
knob cannot be turned. Naming the index instead (`i32 idx = m_frameIndex;`) is the wrong
direction - 99.4444 -> 99.1778. So on a site that is byte-neutral but sits at 99.x with an equal
instruction count, try both directions of the local knob; the accessor form is what makes the
receiver a single-use expression.

**The knob is a CANDIDATE, not a lever - refuted on the very next site.**
`AnimWorkerObj::ResolveTarget` 0x1651b0 presents identically (99.3333, 31 instructions, a single
`ecx`/`edx` swap, every count equal), and BOTH directions are sharply negative:

| spelling | score |
|---|---:|
| original (`CMapPtrToPtr* res = &...; void* out = 0;`) | **99.3333** |
| delete the `res` local | 86.6667 |
| declare `out` before `res` | 86.6667 |

The original is optimal; both edits were reverted. The difference from `ImageSetAni::Render` is
that there the member expression was genuinely single-use, whereas here the map pointer feeds a
`this` that must land in `ecx`, so collapsing its live range costs 13 points instead of gaining.
Test the knob because it is cheap to try and cheap to revert - do not expect it to close a site,
and never apply it unmeasured.
