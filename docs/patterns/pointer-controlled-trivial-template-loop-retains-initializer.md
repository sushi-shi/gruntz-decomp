# A pointer-controlled trivial template loop retains its initializer after VC5 deletes the loop

tags: cpp:template cpp:loop cpp:ctor cpp:dtor | asm:mov asm:push asm:call | topic:codegen-idiom
symptoms: a scalar or pointer-to-member template specialization has an otherwise
empty constructor or destructor, but retail retains one dead element-pointer
store; the destructor therefore calls its base destructor through a four-byte
frame instead of tail-jumping to it
confidence: 10/10

For a trivial element type, MSVC 5.0 `/O2` deletes a placement-construction or
pseudo-destruction loop. It does not always delete the loop initializer. The
initializer that survives depends on which variable controls the `for` loop.

This counter-controlled form retains the counter initializer, not the element
pointer:

```cpp
T* cursor = AsElem(m_base);
for (i32 i = m_lo; i <= m_hi; ++i, ++cursor) {
    cursor->~T();
}
```

For `T = CActHandler`, VC5 emits a dead load of `m_lo`. Making the element
pointer the control variable instead emits retail's dead `m_base` load:

```cpp
for (T* cursor = AsElem(m_base);
     cursor <= AsElem(m_base) + (m_hi - m_lo);
     ++cursor) {
    cursor->~T();
}
```

The corresponding constructor starts at `m_alloc`, ends at
`m_alloc + m_grown`, and placement-news each element. With the PMF element type,
VC5 removes both loop bodies and bounds but retains those pointer initializers.
That produces the otherwise puzzling stores without `volatile` or an invented
escape.

The controlled matrix included counter-controlled, count-controlled,
pointer-end, post-increment, reference-helper, and pointer-controlled forms.
Only the pointer-controlled form loaded the required field in both bodies.
`zDArray<CActHandler>::~zDArray` at `0x00008750` moved from 20.00% to exact;
its constructor moved from 82.75% to 92.1875%, leaving only an eax/edx schedule
rotation.

Use the retained field as the oracle. A dead `m_lo` load identifies a numeric
loop variable; a dead `m_base` or `m_alloc` load identifies the element pointer
as the loop-control variable.
