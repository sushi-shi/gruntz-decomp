# The LAST member ctor gets NO unwind state - diff the ladder against the class's OWN out-of-line copy

tags: cpp:ctor cpp:eh cpp:inline cpp:member | asm:mov asm:call | topic:eh topic:audit
symptoms: `walls ehactions` says ACTION SHAPE DIFFERS with exactly one extra (or missing)
TRAILING rung naming a real member (`*[ebp+N]+0xNNN -> ??1CPtrArray@@UAE@XZ`); the member
demonstrably exists in retail; `walls diagnose` on the same rva says INLINE/CALL-SET
confidence: 10/10
variants: eh-unwind-map-is-a-c1-fingerprint.md, eh-frame-presence-is-a-source-fact.md, inline-depth-two-declines-in-the-largest-caller.md

An unwind state is opened before a THROWING OPERATION, not after a construction. So a ctor's
state ladder is one rung SHORTER than its destructible-member count: the last member's ctor is
followed only by non-throwing stores, so no state ever names it. That makes the trailing rung a
function of what CALLS survive after the last member ctor - which is an inline decision, and
therefore caller-dependent. A rung that appears on one side and not the other is NOT proof of a
missing or extra object.

```asm
    mov   BYTE PTR [esp+STATE],4   ; state 4 = "~CString @+0x410"
    lea   ecx,[esi+0x488]
    call  ??0CPtrArray@@QAE@XZ     ; the LAST destructible member ...
    mov   [esi+0x4a0],ebx          ; ... only plain stores follow, so NO state 5,
    mov   [esi+0x4a8],ebx          ;     and no `~CPtrArray @+0x488` funclet exists
```

**The control is the class's own out-of-line ctor/dtor COMDAT.** It is compiled from the same
members with no caller context, so its ladder is the object-truth baseline. Disassemble it and
compare rung-for-rung with the parent's; only a difference THERE is a member/base modelling
defect. Worked: `CGruntzMgr::TransitionState` 0x8b960 (84.16) - base carries a 25th funclet
`*[ebp+0xc]+0x488 -> ??1CPtrArray@@UAE@XZ` that retail lacks, and `m_cameraBookmarks` is real
(retail's `??0CPlay@@QAE@XZ` 0x8c9d0 constructs it at `lea ecx,[esi+0x488]`). Retail's out-of-line
copy stops at state 4, and so does ours - identical. The parent's extra rung is entirely
downstream of `??0ClockInterval@CPlay@@QAE@XZ`, which retail expands at inline depth 2 and our cl
declines, leaving a throwing call after the last member ctor. `walls diagnose 0x8b960` names it
directly (`base calls, target expanded/lacks: ??0ClockInterval@CPlay@@QAE@XZ`, plus
`??0CMultiBootyState` declined and `??0CRgn` over-expanded).

**REFUTED source hypothesis, recorded so it is not retried.** The same reading applied to
`CDDrawSurfaceMgr::RestoreChildren` 0x156530 / `SnapshotChildren` 0x156020, whose base ladders
carry 2 and 4 extra `~CString @-0x148` rungs: those come from our cl inlining `??1CFileMemBase`
at 2 of the 11 teardown sites (retail calls it at all 11) and from our cl inlining `CFileMem::Reset`
into the inlined `CFileMem` ctor where retail emits `call 0x157a50`. Moving `CFileMem::Reset` out
of `include/Io/FileMem.h` into a `.cpp` scores RestoreChildren 93.84 -> 96.89 and SnapshotChildren
70.12 -> 71.61 - and is WRONG: `LoadRecordFile` 0x156ad0, in the SAME TU and below both of them,
INLINES that very `Reset` in retail (`walls diagnose`: `target calls Empty twice, base once` /
`base calls ?Reset@CFileMem@@UAEXXZ`), and the change costs it 100.00 -> 75.96. `Reset` is
inline-visible in retail; the divergence is per-caller inline budget, and a higher score bought
with an out-of-line move is a distortion.
