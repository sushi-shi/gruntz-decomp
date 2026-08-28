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
topology (36/33 calls and 50/41 branches), so the 2-versus-1 count is a CFG
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

Replacing the invented typed payload union with the retail-selected direct
`void*` member recovered the banked 81.7868 state and made the other eight
`Set<Type>` functions exact. Its constructor/new census matches retail. The
remaining repeated-site delta is therefore not evidence for a missing insertion
arm; it is the nested deleting-destructor cutoff described above.

Controlled source-shaped negatives were byte-identical at 77.0314: explicit
shared `goto` exits for the two hit paths, an `else` around the second miss,
an inner lexical scope for the first lookup, one use of the real `Tags()`
accessor, and per-arm `return` rather than `break` in `~CButeValue`. None moves
the generated deleting-destructor cutoff. Keep the direct heterogeneous payload
member and both temporary expressions; collapsing the authored sites would only
transcribe the compiler fold.

A later lineage-shaped composition tightened that bound. Converting the first one through
five lookup declarations to assignment-in-condition form is byte-flat at 81.7868; the
sixth conversion crosses to the older 77.0314 island. Adding an explicit `else` is flat on
the baseline but, from the six-site dip, returns exactly to the baseline island. Thus the
two authentic levers compose, but there is no third compiler texture between them and the
CString deleting-destructor expansion/call split remains unchanged. Neither spelling is a
route to the retail merge and neither experiment belongs in source.

Safe reverse use:

1. Ignore current dips whose historical MAX is already 100.
2. Run `gruntz walls diagnose <rva>`.
3. If duplicated or merged blocks remain possible, treat the count only as a
   location hint.
4. Infer a missing or extra aggregate only after the surrounding blocks and
   call-set are structurally paired.
