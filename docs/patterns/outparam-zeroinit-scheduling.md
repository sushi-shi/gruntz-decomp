# Lookup-helper out-param zero-init: target SINKS `mov [&out],0` past the arg pushes — scheduling coin-flip
tags: cpp:local asm:mov asm:push | topic:wall topic:scheduling
symptoms: identical instruction multiset, 2-3 instrs permuted per lookup, plateau ~80-95%, the `Lookup(name,&spr)` family
confidence: 8/10

The sprite/anim "lookup then use" family (`…->Lookup(key, &spr)`) plateaus at high-but-not-100%
because retail SINKS the out-param zero-init store (`mov [&spr],0`) past the `lea &spr`/arg
pushes, while our `cl` HOISTS it before. The instruction MULTISET is identical — only the order
of those 2-3 instructions per lookup permutes. Verified source-invariant under /O2 (block-scoped
locals and an explicit `&spr` temp both reproduce the hoisted schedule). It is the same
scheduling coin-flip across the whole family.

WALL (source-invariant scheduling). Evidence: SpriteLoaders/IconLoaders/ActionOptionsMenuBar (84-94%), all CGrunt Create* (99%).
