# Inline `.rdata` jump-table data scored as mismatched (code byte-exact) — scoring artifact
tags: cpp:switch | asm:jmp | topic:scoring-artifact topic:tooling
symptoms: low objdiff % (e.g. 34%) on a big switch whose code is byte-identical, jump-table base reloc, $L labels
confidence: 7/10

A dense `switch` lowers to a `.rdata` jump table indexed by a byte index table. objdiff counts
the jump-table DATA region (and its base displacement reloc) as mismatched because `cl`'s base
obj references local `$L###` labels while the delinked target carries self-relocs — even when
every CODE byte (the switch dispatch, the index table, the case bodies) is identical. Verify by
raw byte-compare of the function code range; the diffs are confined to the jump-table operand
slots + the table region. The function IS matched; the % undercounts it.

WALL (scoring artifact). Evidence: LoadPowerupIconSprites (965B, 34% objdiff, code byte-exact); Stub_0633e0 (21-case two-level table).
