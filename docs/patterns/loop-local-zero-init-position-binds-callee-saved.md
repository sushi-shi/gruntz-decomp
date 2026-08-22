# A loop-local's `= 0` POSITION is its binding slot: an early zero-init steals a callee-saved and hoists the receiver lea

tags: cpp:local cpp:loop cpp:decl | asm:xor asm:lea asm:push | topic:regalloc topic:codegen-idiom
symptoms: `walls diagnose` says REGALLOC/SCHEDULING with equal calls/branches;
ours hoists ONE `lea reg,[this+disp]` receiver into a callee-saved register and
copies it to ECX per call (`mov ecx,ebp`), where retail re-derives
`lea ecx,[this+disp]` at every call site; ours also carries one MORE live
callee-saved zero and one MORE frame slot (`sub esp` differs by 4)
confidence: 9/10

## The mechanism

Call-crossing values bind ESI, EDI, EBX, EBP in DEFINITION order, and a local's
DEFINITION is its first assignment - the declaration line is inert (a decl-only
`u32 copied;` at the top changes nothing). A `u32 copied = 0;` written as the
FIRST statement of the loop body defines the value before the body's other
call-crossing temps (per-call receiver leas, `&table[i]` cursors), so it binds
an EARLY callee-saved slot. The zero constant it materializes then serves the
function-entry guards, and - decisively - the leftover callee-saved register
lets cl HOIST the repeated `lea ecx,[this+0x124]` receiver into it for the
whole function. Retail, which defined the accumulator AFTER the body's guard
calls, has no free callee-saved left, so every call re-derives its receiver
lea in place, one frame slot disappears, and the accumulator lands in EBP
where its zero doubles as the `done = 0` store source.

## The A/B (CFecFile::ExtractArchive 0x0017bcd0, 89.09 -> 95.73)

```cpp
for (u16 i = 0; i < count; i++) {
    u32 copied = 0;                    // A: first statement - 89.09
    if (m_stream.Read(&m_entry, ...))  //    lea ebp,[ebx+0x124] hoisted,
        ...                            //    copied->EDI, zero->ESI, frame +4
    if (file.Open(...) == 0) ...
    if (m_stream.Seek(...) != ...) ...
    u32 copied = 0;                    // B: after the guard calls - 95.73
    i32 done = 0;                      //    copied->EBP, per-site lea ecx,
    while (done == 0) { ... }          //    retail frame size
}
```

Falsified in the same session: a fn-top `u32 copied = 0;` (recreates the early
binding, worse), a fn-top decl-only + loop-top `copied = 0;` (the assignment IS
the definition - identical to A), and (in CGrunt::LoadEntranceConfig) breaking a
member-load CSE via an inline-helper boundary - cl 5.0 CSEs loads across inline
instances.

Detection: compare the `lea <callee-saved>,[<this>+disp]` count between sides.
Ours 1 + N x `mov ecx,<reg>` vs retail N x `lea ecx,[<this>+disp]` means a
call-crossing value defined too early freed the register that made the hoist
profitable - look for a `= 0` accumulator/flag declared above the call run and
move its definition below it.
