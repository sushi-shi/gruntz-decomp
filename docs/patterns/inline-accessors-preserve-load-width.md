# An inline accessor can preserve a load before narrowing

A narrow load is not determined only by the final value's width. With MSVC
5.0 SP3 and `/O2 /MT`, an inline accessor can preserve a full-width argument
load through a macro that truncates it.

The real `CLatencyList::FillCombo` TU provides a three-way control. Its two
[record fields](../../include/Net/KeyedList.h) are signed 32-bit integers:

```cpp
// Direct fields inside the SDK macro: 16-bit loads.
i32 data = MAKELONG(rec->m_commandDelay, rec->m_resendInterval);

// Inline getters with hand-written packing: 32-bit loads, different schedule.
i32 data = ((rec->GetResendInterval() & 0xffff) * 0x10000)
           | (rec->GetCommandDelay() & 0xffff);

// Inline getters inside the SDK macro: retail's 32-bit loads and masks.
i32 data = MAKELONG(rec->GetCommandDelay(), rec->GetResendInterval());
```

The getters return `i32` by value and have no emitted calls at this site.
The last form also reproduces the surrounding register allocation and the
packing-before-`GetName` schedule. Reproduce the controls in
[`LatencyList.cpp`](../../src/Net/LatencyList.cpp) with
`gruntz match latencylist`; compare the field loads as well as the final OR.

This is an observed expression-boundary effect, not evidence of an inline
budget limit. It does not establish that every getter prevents load narrowing,
or that matching bytes uniquely prove these accessor names. When a direct-field
SDK macro emits narrower loads than retail, test the owner's typed accessors
before replacing the macro with arithmetic.
