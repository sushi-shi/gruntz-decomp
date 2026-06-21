# Write the null/early path FIRST (`if(!p){…} else …`) to emit `test;jne` polarity
tags: cpp:branch | asm:test asm:jne | topic:codegen-idiom
symptoms: branch polarity is `je` where the target has `jne` (or vice-versa) on a null/early-out test, otherwise byte-exact
confidence: 7/10

The order you write a null/early-out test fixes the `je`/`jne` polarity. Writing the
null/early path FIRST (`if (!p) { return; } else { … }`) emits `test reg,reg; jne <continue>`;
writing the success path first emits `je`. Match the target's polarity by ordering the source
branch accordingly. (Distinct from the call-result coercion: `Fn(...) != 0` emits the
`neg/sbb/neg` 0/1 normalize, see int-to-bool-normalize.md.)

STEERABLE. Evidence: the L1725 null-check one-liner; MakeImageKey/LoadFromRez per-branch
re-test `if (ext && stricmp(…)==0)` reproducing `test esi; je commonRet`. related:
default-then-override-flag.md (branch polarity from flag init order).
