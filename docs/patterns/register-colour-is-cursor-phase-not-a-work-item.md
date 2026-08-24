# A destination-register difference at the same instruction index is cursor PHASE, and never a work item
tags: asm:mov | topic:wall topic:regalloc topic:negative-control
symptoms: same slot same symbol different destination register, `mov edx,[g_gameReg]` vs `mov ecx,[g_gameReg]`, retail is one byte earlier, moffs32, A1 encoding, register rotation, EAX ECX EDX ESI EDI EBX EBP, register transposition
confidence: 10/10

Two aligned instructions with the same mnemonic and the same (relocation-resolved)
other operand but different registers. The rotation order is
`EAX ECX EDX ESI EDI EBX EBP`, read off a cursor that advances once per IL tuple,
so the delta is a small signed rotation.

```asm
   base    mov edx,DWORD PTR ds:?g_gameReg@@3PAVCGruntzMgr@@A
   retail  mov ecx,DWORD PTR ds:?g_gameReg@@3PAVCGruntzMgr@@A     ; delta -1
```

**Do not rank these by BYTE offset.** `mov eax,moffs32` is the 5-byte `A1` form and
`mov <other-reg>,[disp32]` is 6, so a byte-offset ranking turns every colour site
involving EAX into an apparent "retail is one byte earlier from here on". The
per-site effect is real and the net effect is zero: over the whole class 403 sites
make retail one byte LONGER and 403 make it SHORTER (1045 carry no size delta).
Rank on INSTRUCTION INDEX; the byte offset is only the question when the byte offset
IS the question.

**The sign is symmetric, so there is no bias to correct.** Tree-wide census
(2026-08-23, 871 sub-100 functions): 1851 colour sites across 374 functions,
`-3:80 -2:277 -1:555 +1:631 +2:231 +3:77`, 912 negative / 939 positive, and 216 of
the 374 functions carry BOTH signs among their own sites. Restricted to global
references whose relocation symbol agrees on both sides: 144 sites, 83 functions,
`-3:2 -2:23 -1:42 +1:57 +2:19 +3:1`, 67 negative / 77 positive, EAX asymmetry 39/40.

**Usually a readout, not a cause: 17 of the 374 functions are PURE colour**, i.e.
have no other divergence at all - `CSpotLight::SerializeMove` 99.96,
`CBootyState::LoadGruntEffectSprites` 99.95, `CPathHazard::CPathHazard` 99.92,
`ReadMenuOptionsDialog` 0x36a30 99.90, `CState::FadeInTitle` 99.88,
`zBitVec::operator=` 99.84, `CGrunt::IsDropReady` 99.84, `CFrontCandy` /
`CDoNothing` / `CBehindCandy` ctors 99.83, `LoadDestructButtonSprite` 99.82,
`CDDrawSurfacePair::DrawCross` 99.73, `SaveVideoCheckboxes` 0x378c0 99.50,
`CTimer::HandleEvent` 99.43, `CSymRec::CSymRec` 99.35, `CGrunt::StepWarpExit` 98.77
and `CAniAdvanceCursor::Construct` 95.42. In the other 357 the colour rides on an
independent divergence, and THAT is the row's work.

**The positive control shows the direction of causation.** Re-introducing a
redundant `CoordPoolNode* fl` local in `CGrunt::LoadStateRecord` 0x555e0 makes the
free-list publish store read EAX where retail reads EDX, and the sieve reports
exactly one site there with delta +2; deleting the local again closes the colour AND
the function (99.99 -> 100.00). The colour moved because the IL tuple moved.

**Why no colour-targeted edit exists WITHIN ONE EXPRESSION.** `CAniAdvanceCursor::Construct` is the
minimal case: THIRTEEN instructions, one basic block, no branches, no calls, six
member assignments emitted in the source's own order - and its entire 4.58% gap is
an EAX/EDX transposition (`src` in EAX and the constant 1 in EDX; retail the
reverse), with both sides materialising the two values in the SAME order. There is
no expression left to respell. In general, to change the register of tuple N you
must change the number of tuples before it in the same block, which changes other
instructions; when the streams are otherwise identical the tuple count is identical
and only the cursor's entry phase differs, which is C1 handle state. Four inert A/Bs
on `SaveVideoCheckboxes`: named locals for the results, one reused local, the
TU-state probe, and inverting `if (x == NULL) return;` into `if (x != NULL) { ... }`.

**"Retail's colour is one instruction WORSE" is NOT a proof of unreachability -
CORRECTED 2026-08-23.** `MidiManager::GetMasterVolume` 0x1389c0 was this file's
strongest example: a constant division running the identical magic-multiply
sequence on both sides, where retail accumulates in EDX and pays a closing
`mov eax,edx` to return while cl accumulates in EAX and needs no move, with five
byte-identical A/Bs (a named result local, reusing the input local, the constant
first, an explicit parenthesisation, an `else if` chain). It closed 91.30 ->
**100.00 EXACT** by hoisting the clamp-and-scale into an `inline` helper
(`MidiVolumeToPercent`) and calling it - see
[[inline-helper-supplies-the-il-tuple-a-colour-row-needs]]. The tuple-count
argument below is right; what was wrong was concluding that no SOURCE construct
adds a tuple. An inlined call does, and the era devs wrote inlines.

**The TU-state lever does not reach it either.** Nineteen disposable probes on
`Construct` - nine `typedef`s immediately above its `RVA()` pin and ten function
prototypes above the TU's first project include, counts 1..16 - leave it at 95.42 to
four decimals every time, and leave `CAniAdvanceCursor::Deserialize` in the same TU
at 95.77 every time. Declaration-count steering
([[declaration-count-window-steers-regalloc]]) is not a way round a colour row.

**Detector control.** The same sieve over the 6,596 EXACT functions reports 2 sites,
both in one `grunt` row whose fuzzy rounds to 100.00 - i.e. no false positives from
truly exact rows, and no PURE rows at all.
