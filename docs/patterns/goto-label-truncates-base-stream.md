# A `goto` label truncates the base stream: `$<name>$<n>` is NOT `$L<n>`

tags: cpp:goto | topic:tooling topic:scoring-artifact
symptoms: `insn_seq --multiset` says `base=0 tgt=1` for a callee the source
plainly calls; `insn_count`/`sema disasm --base` show a huge one-sided deficit;
the last printed base instruction is a `jmp`/`jcc` to an address the listing
never reaches; the source contains a `goto`
confidence: 10/10

cl 5.0 gives a function's internal blocks a function-style symbol header, and it
uses **two** spellings: `$L<n>` for compiler-generated blocks (switch arms,
if/else joins) and **`$<source-label>$<n>` for the target of a `goto`**. Every
tool that folded local labels back into the enclosing COMDAT tested only
`name.startswith("$L")`, so a named label started a NEW stream and silently
truncated the function there.

```cpp
// This one label is enough to hide everything after it from every base-side tool:
    case WWDLOOP_NEXT:
    loop_restart:                 // cl emits `$loop_restart$32243` -> stream ends here
        ...
```
```asm
0000040c <$loop_restart$32243>:   ; llvm-objdump: same header syntax as a function
```

Tooling, not codegen: `gruntz.core.branches.is_local_label` is now the single
predicate (`^\$(?:L\d+|\w+\$\d+)$`), shared by `sema disasm --base`,
`insn_seq`, `insn_count` and `branches.parse_objdump`. It reported
`CAniAdvanceCursor::Advance` as 320 instructions against 457 and as never
calling `LeafCue::PlayIfElapsed` / never reading `g_sndCueTag`; the real numbers
were 440 and two live call sites. Only two labels tree-wide
(`$loop_restart$32243`, `$tail$29776`) — but a base-side "we never call this" is
a strong enough lead that it cost two lanes.
