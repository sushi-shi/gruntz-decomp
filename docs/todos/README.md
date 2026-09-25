# Rule exceptions

A 100% match is kept even when its source breaks a project rule (a gate, a
source-modeling rule). `rule-exceptions.tsv` records every such function so the
deviation can be revisited deliberately later.

| column | meaning |
| --- | --- |
| `rva` | retail address |
| `function` | mangled name |
| `rule` | the rule or gate the source breaks |
| `deviation` | what the source does instead |
| `note` | why it was kept and what a rule-clean form would need |

Add the row in the same commit that lands the 100% source, together with any
allow entry the gate needs. Remove the row when a rule-clean spelling also
reaches 100%.
