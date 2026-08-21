# A `rep movs` count mismatch is merge-sensitive, not proof of a different object count

tags: cpp:struct cpp:inline cpp:switch | asm:rep asm:movs asm:jmp | topic:tooling topic:wall topic:negative-control
symptoms: `gruntz walls aggregate-copies` reports more copy instructions on one side, but `gruntz walls diagnose` also reports duplicated inline prefixes, different call counts, or a different branch skeleton
confidence: 10/10

`rep movs*` is strong evidence that cl saw a whole-object copy at that site, but
the number of surviving instructions is not necessarily the number of source
copy expressions. C2 can tail-merge identical expanded switch arms on one side
and retain both on the other. Diagnose the function and compare the local copy
neighborhoods before changing an aggregate into fields or deleting a copy.

`CButeMgr::SetString` at 0x1732a0 is the negative control. The source has two
legitimate `CopyValue` call sites. Each expands the `BUTE_VECTOR` arm to the
same 24-byte whole-object copy. The candidate retains two `rep movsd` blocks;
retail shares one switch body between both sites and therefore contains one.
The function simultaneously differs in inline call-set and repeated-prefix
topology (35/33 calls and 48/41 branches), so the 2-versus-1 count is a CFG
merge consequence, not an invented object.

## Why SetString's two bodies do not merge in the current build

Retail EH states 0/1 and 2/3 independently prove two CString payload
temporaries, one at each hit path.  Retail expands both `CopyValue` sites into
one shared switch-body set and routes both temporary cleanups through the same
generated `CString` deleting destructor.  The current build instead expands
that deleting-destructor body at the first site (`CString::~CString` plus
`operator delete`) and calls it at the second.  Those unequal suffixes prevent
C2 from sharing the two switch bodies.  The duplicated string-assignment arm
and the second `rep movsd` therefore disappear together when the suffixes are
identical; neither names another authored copy.

The same source fingerprint has a banked 81.7868 state whose constructor/new
census already matched retail.  Its current 77.0314 state additionally declines
one later `CButeValue(CString)` expansion, so that repeated-site delta is a
current inline-state dip, not evidence for a missing insertion arm.

Controlled source-shaped negatives were byte-identical at 77.0314: explicit
shared `goto` exits for the two hit paths, an `else` around the second miss,
an inner lexical scope for the first lookup, one use of the real `Tree()`
accessor, and per-arm `return` rather than `break` in `~CButeValue`.  None moves
the generated deleting-destructor cutoff.  Keep the typed payload union and
both temporary expressions; weakening the type or collapsing the authored
sites would only transcribe the compiler fold.

Safe reverse use:

1. Ignore current dips whose historical MAX is already 100.
2. Run `gruntz walls diagnose <rva>`.
3. If duplicated or merged blocks remain possible, treat the count only as a
   location hint.
4. Infer a missing or extra aggregate only after the surrounding blocks and
   call-set are structurally paired.
