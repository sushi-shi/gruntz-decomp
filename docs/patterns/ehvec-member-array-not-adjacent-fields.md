# A record callback calling `__ehvec_ctor` proves an array member, not adjacent fields

tags: cpp:array cpp:member cpp:ctor cpp:dtor cpp:eh | asm:push asm:call | topic:codegen-idiom topic:identity
symptoms: __ehvec_ctor __ehvec_dtor push element-count push element-size adjacent non-trivial members expanded GX frame
confidence: 10/10
variants: array-new-cookie-ehvec-ctor.md

When a small record's own constructor and destructor callbacks each contain one
vector-helper call, the repeated subobjects inside that record are an array
member. Modeling them as adjacent individually named members preserves offsets
but makes MSVC5 emit a per-member `/GX` call/state sequence instead.

```cpp
struct Record {
    String m_names[5];

    String& AttackName() {
        return m_names[0];
    }

    Record() {}
    ~Record() {}
};
```

```asm
; Record::Record
push <String dtor>
push <String ctor>
push 5
push 4
push this
call __ehvec_ctor

; Record::~Record
push <String dtor>
push 5
push 4
push this
call __ehvec_dtor
```

This is also a safe reverse-use heuristic: a run of same-sized non-trivial
fields whose callback expands into repeated calls should be regrouped when
retail instead passes their count and element size to `__ehvec_*`. Keep semantic
names through trivial inline reference accessors; do not retain the wrong field
boundaries merely because their offsets coincide.

This differs from `new T[n]`: there is no allocation or array cookie here; the
helper operates directly on an embedded fixed-size member. Proven by
`CGruntCellRec` at 0x0000f400/0x0000f430: five adjacent `CString` fields emitted
large per-member `/GX` bodies, while `CString m_names[5]` emits the retail
five-element helper calls.
