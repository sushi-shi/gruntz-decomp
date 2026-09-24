# A named `ostream&` result preserves insertion sequencing across statements

tags: cpp:call cpp:local cpp:stream | asm:call asm:mov asm:push | topic:codegen-idiom
symptoms: an `operator<<` chain has the exact calls and operands, but retail emits an opening
delimiter before loading later fields and feeds a later insertion from the preceding call's EAX;
the recompile either pre-pushes every argument or reloads the original stream receiver
confidence: 10/10

## The three distinguishable source shapes

MSVC 5.0 does not treat these equivalent-looking forms alike. A monolithic insertion expression
evaluates enough of the nested calls up front that a closing delimiter is pushed before the first
call. Splitting it into ordinary statements delays the delimiter, but throws away the returned
stream and reloads the original receiver. Binding the returned `ostream&` preserves both the
statement boundary and the insertion result:

```cpp
ostream& body = output << static_cast<unsigned char>('(');
ButeIntRect* rect = static_cast<ButeIntRect*>(value->pValue);
ostream& tail = body << rect->a << comma << rect->b << comma << rect->c << comma << rect->d;
tail << static_cast<unsigned char>(')');
```

The assembly signature is:

1. call the opening-delimiter insertion;
2. only then load the aggregate payload;
3. use the prior call's EAX as the receiver for the field chain;
4. push the closing delimiter only after that chain returns, and again use EAX as the receiver.

An ordinary second statement uses the original stream's callee-saved register in steps 3 or 4.
A single expression pre-pushes the closing delimiter before step 1. Neither is the same source
entity or evaluation boundary.

## Evidence

`ButeGroup_Apply` at `0x1712b0` started at 87.7283%, 0x220 bytes and 167 instructions against
retail's 0x228 bytes and 175 instructions. The calls, branches, returns, constants, stores and
payload displacements already agreed, but the recompile never made the callback value live in
EDI and flattened the quoted-string and rectangle expressions.

Separating the opening/body/closing insertions restored the EDI payload lifetime and reached
98.2337%, with exact size, instruction count, 33 calls, 3 branches, 8 returns, 60 normalized
relocations, mnemonics and ordered referents. The only residue was the receiver at the two joins:
the recompile used the original stream in ESI while retail used the preceding insertion result in
EAX. Naming those returned references made the normalized function byte-identical and 100%.

### Scope control: direct value-accessor chains can already be exact

A fresh real-TU control of the same callback (now `CButeMgr::AuxTabItemsSave`)
compared its vector/range arms with two cached payload pointers and five scalar
snapshots against direct chained component-getter expressions. The complete
callback remained byte/reference/extent-identical at 100% (0x228 bytes and 60
normalized references); all 203 compared Bute functions were unchanged. The
source adoption and original Debug local-census evidence are recorded in
`reassess-bute-value-save-chains` in the lineage ledger.

In retail, vector loads k/j/i and range loads max/min prepare the outer QWORD
arguments before the first stream insertion. The vector arm then joins the
range-form insertion tail. A direct accessor chain therefore does not imply
loads interleaved with calls. Nor does this pattern prove that scalar snapshots
in other arms were authored: its evidence concerns the specific delimiter and
returned-stream boundaries above. Preserve those boundaries without freezing
unrelated local transcriptions merely because the whole function was exact.

A natural full inline formatter for each value was the abstraction-level negative control. VC5
flattened both helpers back into the original monolithic island: 0x220 bytes, 167 instructions and
87.7283%. The exact named-result form therefore overrules the inline-helper prior for this case.

## Reverse-use heuristic

Use this only when the receiver evidence is complete: same insertion overloads and operands, a
delimiter or payload load on the wrong side of a call, and retail continuing through that call's
EAX. Do not introduce a reference merely to swap registers. The named entity is justified by the
combination of evaluation timing and receiver provenance; either signal alone can be ordinary
scheduling noise.
