# Throwing `::operator new` emits the member-ctor /GX state transition a nothrow C alloc omits
tags: cpp:ctor cpp:new cpp:eh | asm:mov asm:call | topic:codegen-idiom topic:eh
symptoms: missing `mov BYTE PTR [esp+N],<state>` right before a `call` to the heap alloc inside a /GX ctor; ctor ~96% with one absent EH-state byte-store; the alloc otherwise byte-exact
confidence: 8/10

Inside a /GX constructor that has already built one or more destructible members,
the next allocation in the body is a throw point: MSVC bumps the member-construction
EH state (`mov BYTE PTR [esp+N],<state>`) right before the `call` so an unwind cleans
the live members. The retail alloc is the throwing global `operator new` (0x1b9b46),
so the transition is present. Modeling that alloc as a nothrow `extern "C"` helper
(MSVC5 treats extern "C" as non-throwing) DROPS the transition → the byte before the
call goes missing and the ctor plateaus ~1-2% below exact. Spell the buffer alloc as
`::operator new` (not the C `RezAlloc` alias) to recover the state store.

```cpp
// inside a /GX ctor, AFTER the destructible members are constructed:
m_name = (char*)::operator new(strlen(name) + 1);   // throwing -> EH state bumped
// NOT: m_name = (char*)RezAlloc(strlen(name) + 1);  // extern "C" nothrow -> no bump
```
```asm
push   ecx                      ; the size
mov    BYTE PTR [esp+0x20],0x1  ; <- member-ctor EH state -> 1 (only with throwing new)
call   <operator new>           ; 0x1b9b46
```
Steerable: CSymTab::CSymTab 0x139de0 went 95.95%→97.33% on the swap (residual is the
operator-new vs delinked `_RezAlloc` reloc-name artifact — exact is unreachable since
the throwing-name and the `_RezAlloc` symbol-name requirements conflict).
