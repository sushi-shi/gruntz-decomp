# An erased dynamic-vector fetch plus a construction loop is typed indexing

tags: cpp:inline cpp:template cpp:container | asm:call | topic:codegen-idiom topic:identity

The earlier version correctly recognized two inline cuts of one operation,
but assigned both methods to the wrong owners. The old `_zvec::IndexToPtr`
at 0x312a0 is the erased dynamic-vector fetch owned by `_zdvec`. The old
`_zdvec::IndexToPtr` at 0x310f0 constructs CString elements and belongs to
`zDArray<CString>::operator[]`. The source and adoption decisions are recorded
by `nolf-zdarray-*` in `config/lithtech_lineage.tsv`.

A typed array operation first obtains a slot through the erased dynamic base,
then placement-constructs the newly grown elements and returns the slot by
reference. Retail callers show both an out-of-line erased fetch followed by
an expanded constructor loop and a complete expansion of the same operation.
Those cuts do not require different source-level operations or different
container identities.

The earlier `ArrivalRecycle` experiment (0x59230, 68.46 -> 93.19) recovered a
missing use/side-effect boundary: writing a second unused fetch beside the
real operation let `/O2` delete its address arithmetic while retaining side
effects. The countdown loop also matters: `while (n-- != 0)` and a separate
end-of-loop decrement are different compiler inputs. These observations
survive the owner correction; the invented class and raw/typed accessor
names do not.

Both recovered typed indexing bodies now come from one template definition.
They remain exact at 0x310f0 and 0x464e0. Restoring header visibility changes
caller call cuts and reopens earlier reviews; it does not prove those callers
closed. In particular, a visible template body can expand under `/Ob1` even
without the `inline` keyword. See
[the measured template exception](vc5-template-members-inline-without-inline-keyword.md).

Related: [relocation sequence and missing statements](reloc-sequence-diff-names-the-missing-statement.md),
[inline depth and body cuts](inline-depth-splits-one-body-into-two-shapes.md).
