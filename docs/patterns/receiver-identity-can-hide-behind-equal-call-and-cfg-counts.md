# Equal call and CFG counts do not prove receiver identity

tags: cpp:member cpp:local cpp:ownership | asm:mov asm:lea | topic:semantic-identity topic:wall
symptoms: two same-typed receivers use the same methods and member offsets, but the source assigns a path, flags or state to the opposite owner; call multisets, member-displacement histograms and branch counts still agree
confidence: 10/10

`CBattlezMapConfig::PathToNearestCandidate` at `0x02edb0` selected a nearby
candidate to receive a route. Main `520d5b06b` instead calculated the route from
the requesting unit to the candidate, assigned the nodes to the requesting
unit, and exchanged the final state receivers. PR #72 contained most of the
ownership correction, but still read the traversal flags from the candidate.

The original and retail objects both contain 24 calls, 101 branches, one
return and 45 relocations. Their member-displacement, store and immediate
histograms agree; the first-divergence diagnostic therefore reports an
allocation/scheduling difference. None of these summaries establishes which
object supplies a member or receives a call. The retail data flow does:

| Operation | Retail evidence | Source owner |
|---|---|---|
| Requesting unit | parameter at entry stack `+0x48`, retained in EDI | `unit` |
| Candidate | units-table load into EBP at `0x02ef8b` | `cand` |
| Traversal pickup and tool | EDI plus `0x170` / `0x19c` at `0x02f2ae`–`0x02f2d6` | `unit` |
| Route start | ECX = EBP before `GetScreenPos` at `0x02f2f0` | candidate's screen tile |
| Route destination | saved target coordinates at stack `+0x14` / `+0x18` | `target` |
| Old routes recycled | EDI list first, then EBP list | unit, then candidate |
| New nodes appended | ECX derived from EBP plus `0x31c` at `0x02f3f3` | candidate's coordinate list |
| Final states | original parameter plus `0x2d4` gets 0; EBP plus `0x2d4` gets 5 | unit seeks; candidate retreats |

Restoring these owners, the existing saved-position helper and the existing
screen-tile macro changes only this function in the isolated object. Its score
moves from 86.4792% to 93.64195%, above the previous 86.5425% historical MAX,
without changing the call multiset, branch/return counts or relocation count.
The complete receiver trace, not that score increase, establishes the fix.
The surrounding allocation and ordered-helper residue remains open; the old
`@early-stop` claim is removed from this edited function.

For reverse use, label each receiver at its origin before following calls and
stores through an optimized block. Preserve identity across callee-saved
registers, spills and reused locals. Compare the arguments of matching call
sites and the receiver of each matching member displacement. A histogram
comparison or equal call target set is useful triage, not an ownership audit.
