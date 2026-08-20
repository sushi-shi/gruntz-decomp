# A compound guard before `else if` keeps the failed-conjunct re-test

tags: cpp:branch cpp:if | asm:test asm:jcc asm:jmp | topic:codegen-idiom topic:correctness
symptoms: retail has one more conditional branch than a nested reconstruction; the
extra branch re-tests the first condition only when the first arm's second conjunct
fails, while a successful first arm jumps over the whole second test
confidence: 10/10

For two flags `A` and `B`, these three equivalent-looking shapes do not produce the
same CFG under cl 5.0 `/O2`:

```cpp
// nested: the A && !B path never enters the else-if
if (A) {
    if (B) F();
} else if (!A && B) {
    G();
}

// siblings: the successful F path falls into the second test
if (A && B) F();
if (!A && B) G();

// retail shape
if (A && B) {
    F();
} else if (!A && B) {
    G();
}
```

The last form emits the distinctive hybrid:

```asm
test A,A
je   second_B
test B,B
je   retest_A
call F
jmp  done
retest_A:
test A,A
jne  done
second_B:
test B,B
je   done
call G
```

The re-test alone is not enough: separate sibling `if`s retain it but let the `F`
path fall through, losing retail's unconditional skip. The skip alone is likewise
not enough: the nested form lets dominance remove the re-test. Both edges together
identify the compound-first-condition / `else if` source structure.

Measured on `CMultiStartDlg::SyncChannelSlot` (`0xc2ab0`). The nested reconstruction
emitted 10 branches against retail's 11. Separate sibling `if`s kept 10 total branches
because one `jmp` became the re-test. The compound form emitted 11/11 branches with
18/18 calls and 19/19 relocations, changing `gruntz walls diagnose` from CFG to
REGALLOC/SCHEDULING. The remaining two-instruction delta is zero-register selection;
bare truth tests and explicit `!= 0` spellings compile identically.

The residual zero-register choice is bounded separately from the CFG result. Retail
materializes zero in `EAX` beside the two flag stores and first `EnableWindow` call;
the reconstruction keeps zero in `EDI` across the whole zero-selection arm. Standalone
and call-argument chained assignments, a scoped `BOOL` value shared by those three
uses, and top-of-function declarations all compile to the same `EDI` body. A controlled
96-trial parser-visible TU-state search found one compiler island, and 122 compiling
syntax-aware expression/member/helper variants found that same island. The source is
therefore structurally complete at the branch level; this evidence does not justify a
carrier local, generated helper, or retained TU-state declaration.
