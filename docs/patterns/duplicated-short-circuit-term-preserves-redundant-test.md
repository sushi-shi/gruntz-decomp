# A duplicated short-circuit term preserves the repeated machine test

tags: cpp:branch cpp:operator cpp:if | asm:test asm:jcc | topic:codegen-idiom topic:wall
symptoms: retail has one more branch than the base; the extra branch repeats an
  earlier test of the same local, and both outcomes converge on the same failure
  or continuation blocks
confidence: 10/10

MSVC 5.0 lowers the arms of a short-circuit `||` independently enough to retain
the same leading predicate twice. A dominating source `if` is not equivalent:
the optimizer proves the second test redundant and deletes it.

`CDDSurface::DecodePid` at `0x145b10` provided the controlled case. Retail's
non-embedded-palette arm is:

```asm
test remap,remap
je   continue
test palette,palette
je   fail
test remap,remap
je   continue
cmp  palBpp,8
jne  continue
test hasPal,hasPal
je   fail
```

The reconstruction had made the first `remap` test dominate the second:

```cpp
} else if (remap) {
    if (palette == NULL) return 0;
    if (remap && palBpp == BPP_PALETTED_8 && hasPal == 0) return 0;
}
```

VC5 therefore emitted 21 branches against retail's 22. The retail shape comes
from the repeated term living in two independent OR arms:

```cpp
} else if (
    (remap && palette == NULL) ||
    (remap && palBpp == BPP_PALETTED_8 && hasPal == 0)
) {
    return 0;
}
```

That change moved the body from 155 to 157 instructions and from 21 to 22
branches, against retail's 158 instructions and 22 branches. Calls, returns,
and all eleven relocations remained equal; fuzzy rose from 92.41% to 96.10%.
The remaining difference is a prologue register rotation, not CFG.

Negative controls were byte-identical to the old 21-branch body: an `else`
containing two separate `if (remap)` statements, an explicit typed dword cursor
instead of `RecordBytes<PidHeader>`, an inverted early guard, and split
declarations for the parsed header fields.

Reverse-use this only when the retail CFG repeats the same test on two paths
whose intervening predicates correspond to distinct short-circuit arms. Do not
introduce redundant conditions merely to add a branch: the repeated machine
test, its two convergence targets, and the shared failure body are the proof.
