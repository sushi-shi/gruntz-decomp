# LICM placement: declare-before-loop lands pre-guard, declare-inside lands preheader

**Signature.** A loop-invariant value (a bias, a row pointer, a scaled constant)
sits in the wrong basic block: our build computes it in the block BEFORE the
loop guard, retail has it in the loop PREHEADER (after the guard, before the
body) — or vice versa. Every other block aligns.

**Mechanism (cl 5.0 /O2).** A local declared and initialized *before*
`while (count-- > 0)` is ordinary straight-line code: it stays in the pre-guard
block. The same expression declared *inside* the loop body is loop-invariant,
so cl's LICM hoists it — and LICM's hoist target is the **preheader**, not the
pre-guard block. The two spellings are semantically identical and land the same
instructions in different blocks.

**Lever.** Match retail's block by choosing where the declaration lives:

```cpp
u8* rd = m_rleData + bias;      // BEFORE the while  -> pre-guard block
while (count-- > 0) { ... }

while (count-- > 0) {
    u8* rd = m_rleData + bias;  // INSIDE the body   -> LICM -> preheader
    ...
}
```

Worth +2.9/+3.3 per function on the DDrawShadeBlit blit arms (opus-A wave);
retail keeps `rd` and the scratch/source biases in the preheader there.

**Related:** [[do-while-is-an-echo-write-while]] (the guard shape itself),
[[forward-goto-hoists-target-block]].
