# A slot-shift funclet holds no work — but its displacement delta says WHICH question to ask the parent

tags: cpp:eh cpp:local cpp:dtor | asm:lea asm:sub | topic:wall topic:tooling topic:eh
symptoms: `walls ehactions --census` reports `slot-shift` — same destructors, same order, a
  different `[ebp-N]`; 155 sub-100 rows over 28 parents that no source edit seems to reach;
  the urge to work them as rows
confidence: 10/10
variants: eh-band-pairs-by-construction.md, empty-raii-guard-eh-this-slot-is-not-storage.md

An MSVC 5.0 unwind funclet is two instructions:

    lea ecx,[ebp-N]
    jmp <dtor>

When the destructor set and its order already agree, N is the row's ENTIRE remaining content,
and N is not authored anywhere - the parent's frame allocator chose it. So **no source edit
targets a funclet**, and every slot-shift row closes exactly when its parent closes, never
before. The census already says this; what follows is the part the census does not.

## The delta is a free classifier of the PARENT

`gruntz walls ehactions --shift` reads each parent's per-action delta beside its own
`sub esp,N` and its funclets' destructor set. 2026-08-23, 28 parents / 155 rows:

| class | parents | rows | what the parent is being told |
|---|---|---|---|
| `frame-base` | 2 | 7 | the shift IS the prologue delta - a frame-SIZE question (`walls framescan`) |
| fieldless guard | 4 | 4 | nothing: C1 colored a synthetic `this` onto a dead home |
| `uniform`, other | 10 | 30 | the object sits at a different offset in an otherwise same-shaped frame |
| `per-object` | 12 | 114 | the destructible objects moved relative to EACH OTHER - a local's storage or lifetime, not a frame size |

Only **two** parents are a frame-size fact: `CGruntSpawnConfig::BuildVoiceSoundList` (+0x4
against frame -0x4) and `CNetSession::BuildGruntzCrcInfo` (+0x8 against -0x8). Everything else
shifts by an amount the prologue does not explain - `CPlay::ValidateLevelTiles` moves its one
`CString` -0xc while the frame grows +0x8, and `CButeMgr::SetInt` / `SetVector` /
`CBattlezDlg::DoDataExchange` / `zBitVec::zBitVec` move by +0x4 with the frame size IDENTICAL.
Reading a slot shift as "our frame is the wrong size" is therefore wrong 26 times out of 28.

## The four rows that can never close, and why the tool prints the destructors

`--shift` prints each parent's funclet destructor set, because that is what identifies the
bounded case. The four parents whose set contains `??1CWaitCursorScope@@QAE@XZ` -
`CBattlezDlgCustom::DoDataExchange` +0x514, `FillCustomLevelList` +0x11c,
`CGruntzMgr::ResetWorldState` -0xc, `StartUpPrompt` +0x8 - are the empty RAII guard of
`empty-raii-guard-eh-this-slot-is-not-storage.md`: a fieldless class whose destructor never
reads `this`, so C1 may color the synthetic receiver onto any dead frame home and the
displacement carries no information at all. Those four magnitudes reproduce that pattern's
calibration set exactly (its signs are target-minus-base, `--shift` reports base-minus-target).

An independent instrument landing on the same four magnitudes is the reason to trust the other
twenty-four rows of the table.

## Worked example: what the delta hands the parent, and what it does not

`CPlay::ValidateLevelTiles` (0xd2dd0) is the group's biggest row block (22) and its
`uniform -0xc against a frame that grew +0x8` sends you to the prologue, where the parent's
real difference is visible without touching the funclets:

    retail   sub esp,0x34 ... four `mov [slot],eax` zeroes (counts[4], contiguous)
             ... jne <past the list==NULL guard> ... mov eax,[eax+0x4]
             mov DWORD PTR [esp+0x10],0x0        <- validCount, SUNK past BOTH guards
             mov ebp,0x1                         <- ok = 1
    ours     sub esp,0x3c ... four `mov [slot],eax` zeroes
             mov DWORD PTR [esp+0x38],0x0        <- a FIFTH contiguous zeroed dword
             mov DWORD PTR [esp+0x10],0x0        <- and both immediates BEFORE the guard

So the parent zero-initializes five contiguous dwords where retail zeroes four, and retail
sinks its remaining zero past two early returns. That is a local-count question, exactly what
the `uniform`-but-not-frame-size classification predicted.

DISPROVEN LEVER, do not retry: moving `i32 validCount = 0;` below both guards, to the
declaration position retail's sinking suggests, is codegen-INERT here - byte length 0x1e58 and
2406 instructions before and after, first divergence still at +0x17. cl 5.0 re-hoists it. The
fifth zeroed dword is the thing to explain, not the store's position.

## Do not

Do not add padding or a local to move a displacement. Do not open funclet tickets. The rows
score 99.44-99.80 (two instructions, one differing immediate byte), so the whole group is worth
about 0.005% each even if every one closed - the budget belongs to the 28 parents, and the
delta pattern says which of them are frame-size questions (2) and which are local-homing
questions (22).
