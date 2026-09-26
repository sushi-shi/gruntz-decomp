# Todo ledgers

## Rule exceptions

A 100% match is kept even when its source breaks a project rule (a gate, a
source-modeling rule), and so is a rule-breaking spelling that retail's bytes
positively require below 100%. `rule-exceptions.tsv` records every such
function so the deviation can be revisited deliberately later.

| column | meaning |
| --- | --- |
| `rva` | retail address |
| `function` | mangled name |
| `rule` | the rule or gate the source breaks |
| `deviation` | what the source does instead |
| `note` | why it was kept and what a rule-clean form would need |

Add the row in the same commit that lands the source, together with any
allow entry the gate needs. Remove the row when a rule-clean spelling reaches
the same score.

## Syntactic recovery

`syntactic-recovery.tsv` is written by `gruntz match`. A row is a function an
edit left at the same CUR while its new source hash lowered MAX from
`lost_max` (the peak stays in HIST). The matching loop never works these; a
separate fuzzy syntactic recovery pass looks for a spelling of the current
source that regains `lost_max`, and removes the row when it does.
