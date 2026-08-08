# `delete`'s own null test survives only when the guard and the `delete` both read the MEMBER
tags: cpp:branch cpp:dtor cpp:local | asm:cmp asm:jcc | topic:codegen-idiom
symptoms: a failed-init cleanup block is one `cmp`/`jcc` pair short of retail - `--blocks --lite` shows retail with `3i [jcc]` / `3i [jcc]` / `Ni [fall]` / `3i [jmp]` where the base has `3i [jcc]` / `N+? i [jmp]`; `dup_compare --any-dest` flags the site as two identical `cmpl %ebp,%esi` twelve bytes apart
confidence: 9/10
variants: redundant-test-elimination-is-syntactic.md

`delete p;` on a class with a destructor emits its own `if (p) { dtor; operator
delete(p); }` null test. cl 5.0 deletes that test when it can see the same value
already tested — which it can if the guard and the `delete` name the same LOCAL:

```cpp
// NO - one test. cl proves `so != NULL` at the delete and drops its check.
CStatusBarMgr* so = m_guts;
if (so == NULL) { return 0; }
so->Teardown();            // and this call is not in retail either
delete so;
m_guts = NULL;
return 0;

// YES - two tests, which is what retail has
if (m_guts == NULL) { return 0; }
delete m_guts;             // ~CStatusBarMgr inlines and calls Teardown itself
m_guts = NULL;
return 0;
```

Reading both off the member gives cl two distinct expressions; it still CSEs the
LOAD (one `mov esi,[ebx+0x2dc]`) but keeps both `cmp esi,ebp` / `jcc`, which is
byte-for-byte retail. The behaviour is the syntactic peephole of
[redundant-test-elimination-is-syntactic](redundant-test-elimination-is-syntactic.md)
with the second comparison compiler-generated instead of written.

Two things travel with this shape and are worth checking at the same site:

* **A spurious pre-`delete` cleanup call.** If the class's destructor already
  calls `Teardown()`/`Unload()`, retail's block has ONE call and ours has two.
  Count the calls in the target block before assuming the extra one is real.
* **`::operator delete` is a different construct and retail uses it too.** In
  `CMulti::LoadGameAssetNamespaces` the `m_hitTest` cleanup is genuinely
  `io->Deactivate(); ::operator delete(io);` with NO null test and NO destructor
  (8i, matching) while the `m_guts` and `m_beginMarker` cleanups two blocks later
  are `delete`. Do not normalise the three to one spelling.

Measured 2026-08-08: `CMulti::LoadGameAssetNamespaces` 0xb5460 80.21 -> 84.46
(with the latency-loop fix below), `CPlay::LoadGameAssetNamespaces` 0xc7ec0
78.26 -> 78.32 with all three cleanup blocks now block-shaped like retail's.

## The neighbour bug in the same function

`g_gameReg->m_options[i].m_latency.m_avg = 0; g_gameReg->m_options[i].m_latency
.m_count = 0;` re-loads the global for the second field. Retail takes the address
once - `mov ecx,[g_gameReg]; mov [ecx+eax+0x37c],ebp; lea ecx,[ecx+eax+0x37c];
mov [ecx+0x4],ebp` - i.e. `PlayerLatency* lat = &g_gameReg->m_options[i]
.m_latency;` then two stores through it. Worth 1.9 points on its own.
