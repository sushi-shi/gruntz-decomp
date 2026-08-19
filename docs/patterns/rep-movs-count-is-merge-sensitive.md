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

Safe reverse use:

1. Ignore current dips whose historical MAX is already 100.
2. Run `gruntz walls diagnose <rva>`.
3. If duplicated or merged blocks remain possible, treat the count only as a
   location hint.
4. Infer a missing or extra aggregate only after the surrounding blocks and
   call-set are structurally paired.
