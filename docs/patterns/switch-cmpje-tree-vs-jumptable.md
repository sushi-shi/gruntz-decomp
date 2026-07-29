# Sparse switch → cmp/je tree (matches 100%); dense switch → jump table (~96% reloc plateau)
tags: cpp:switch | asm:cmp asm:je asm:jmp | topic:codegen-idiom
symptoms: same archetype, one sibling 100% and another stuck ~96%, cmp/je ladder vs `jmp [tbl+eax*4]`
confidence: 8/10

MSVC5 /O2 lowers a switch with SPARSE/few case values to a `cmp/je` binary-search tree (no
`.rdata` table → reaches byte-exact 100%), but a switch with a DENSE contiguous case range to a
`.rdata` jump table (whose base reloc keeps it at the ~96% jumptable-data-overlap plateau).
Explains why identical-archetype siblings score differently. The lowering follows the case-value
density, so you cannot pick it freely — BUT the density cl sees is not the density you wrote:
every arm whose only statement is `break` is folded into the default first. When retail has a
table and you get a chain, that is usually the fold, and it IS steerable — see
[[empty-switch-arms-fold-into-default-and-kill-the-jump-table]]. If a tree-lowered sibling
matched at 100% and yours is at ~96% with a real jump table, that's the table, not a bug.

WALL once it's a jump table (see jumptable-data-overlap). Evidence: DirectInputMgr2::GetErrorString (cmp/je tree) 100% vs CDirectDrawMgr::GetErrorString (jump table) 96%.
