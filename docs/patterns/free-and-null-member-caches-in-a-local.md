# `free-then-null` teardown blocks read the member ONCE into a local — an extra member RE-READ is the tell

tags: cpp:local cpp:member cpp:dtor | asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: extra `mov eax,[edi+0x320]` before `push eax`, retail `push esi` / `mov ecx,esi`,
`cmp <reg>,ebp` on a callee-saved zero, "hard-regalloc wall … the cached free-list head"
confidence: 9/10

A teardown/dtor body full of `if (m_x) { m_x->Release(); ::operator delete(m_x); m_x = 0; }`
blocks. cl5 re-reads `m_x` from the object for the `::operator delete` argument; retail
loads it ONCE into a callee-saved register and uses that register for the receiver, the
push AND the null compare. The signal is mechanical: **base has an instruction retail does
not** (`mov <reg>,[this+off]` between the two calls) — never a scheduling wall, always a
missing local. Only the blocks that use the member more than once are affected; a lone
`delete m_x; m_x = 0;` block already matches.

```cpp
// before - cl re-reads [edi+0x320] for the delete
if (m_lightFx) { m_lightFx->Ctor(); ::operator delete(m_lightFx); m_lightFx = 0; }

// after - one read, retail's shape
CLightFxRender* fx = m_lightFx;
if (fx) { fx->Ctor(); ::operator delete(fx); m_lightFx = 0; }
```

```asm
target: mov esi,[edi+0x320] | cmp esi,ebp | je .. | mov ecx,esi | call .. | push esi | call ??3
base:   mov ecx,[edi+0x320] | cmp ecx,ebp | je .. |               call .. | mov eax,[edi+0x320] | push eax | call ??3
```

STEERABLE. Do NOT widen the local past the block (it then lives across the following calls
and takes a callee-saved register of its own). Evidence: `CPlay::ReleaseResources` @0x0c8700
94.90 → **100% EXACT** with three such locals (m_lightFx / m_hitTest / m_frameMarker), filed
as a "hard-regalloc wall … not source-steerable".
