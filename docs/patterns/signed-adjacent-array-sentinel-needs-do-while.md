# A signed adjacent-array sentinel needs an address-word comparison at a do/while latch

tags: cpp:array cpp:loop cpp:pointer | asm:cmp asm:jl asm:jb | topic:codegen-idiom
symptoms: retail advances a pointer through one global array, then compares it with a
relocation naming the immediately adjacent global; the back edge is signed `jl`, and
an indexed constant-bound loop loses the relocation while a normal pointer comparison
emits unsigned `jb`

An adjacent global can serve as the one-past-end sentinel of the array before it. When
retail's `cmp` relocation names that next global, the relocation is source evidence:
replacing the test with `i < N` may preserve behavior for the current layout but no
longer reconstructs the expression that cl compiled.

On 32-bit MSVC 5, an ordinary pointer relational comparison emits unsigned `jb`.
Retail's signed `jl` therefore requires comparing the pointer address words as signed
32-bit integers. Use the project's typed ABI wrapper instead of a raw cast:

```cpp
AddrWord<const T> cursor;
AddrWord<const T> end;
cursor.m_addr = array;
end.m_addr = nextArray;
// ... advance cursor.m_addr ...
while (cursor.m_word < end.m_word);
```

Loop placement is an independent oracle. A pre-tested `for` or `while` adds a guard
before the first iteration when the compiler cannot prove the two global addresses'
ordering. If retail has only the signed latch branch, the source is a `do/while`: the
first element is processed unconditionally and the adjacent-symbol comparison is made
after advancing the cursor.

`CMultiBootyState::LoadGameAssetNamespaces` (0x1d440) is the control. An index bound
had the wrong referent. A direct pointer comparison had `jb`. A signed address-word
comparison in a `for` had the right `jl` and relocation but 43 branches/75 blocks.
Moving the same comparison to a do/while latch gives retail's 42 branches, 74 blocks,
and complete symbolic branch sequence; fuzzy rises 82.499 -> 82.61.

Do not infer this form from adjacency alone. Require all three signals: the relocation
names the next array, retail uses signed `jl`, and no preheader trip-count branch exists.
