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
ButeIntRect* rect = value->payload.m_rect;
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

A natural full inline formatter for each value was the abstraction-level negative control. VC5
flattened both helpers back into the original monolithic island: 0x220 bytes, 167 instructions and
87.7283%. The exact named-result form therefore overrules the inline-helper prior for this case.

## Reverse-use heuristic

Use this only when the receiver evidence is complete: same insertion overloads and operands, a
delimiter or payload load on the wrong side of a call, and retail continuing through that call's
EAX. Do not introduce a reference merely to swap registers. The named entity is justified by the
combination of evaluation timing and receiver provenance; either signal alone can be ordinary
scheduling noise.
