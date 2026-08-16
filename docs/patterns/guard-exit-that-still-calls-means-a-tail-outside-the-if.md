# A guard's early exit that STILL makes the last call: the tail statement is outside the `if`
tags: cpp:branch cpp:if cpp:call cpp:stream | asm:pop asm:call asm:jcc | topic:codegen-idiom
symptoms: retail interleaves the callee-saved `pop`s into the middle of the tail block, the guard's `je` lands ON a call rather than on the `ret`
confidence: 10/10

The whole body is byte-identical except the epilogue: retail's leading guard
jumps to a block that still executes the final call, and its `pop ebx/ebp/esi`
are threaded between the tail's own instructions instead of sitting after the
call. That is cl converging two paths with different push depths - the guarded
region owns the extra pushes, so the in-body path must pop them BEFORE the
join. It means the trailing statement is not inside the `if`.

```cpp
// retail: je <the osfx call>;  ... pop ebx / pop ebp / <tail> / pop esi / call osfx
ostream& operator<<(ostream& os, const zBitVec& bits) {
    if (os.opfx()) {
        ...
        os.flags(saved);
    }
    os.osfx();          // OUTSIDE - both paths reach it
    return os;
}
```
```asm
000e: je   0xaa                 ; the guard lands on the tail CALL, not the ret
...
00a0: pop  ebx                  ; pops interleaved with the tail's own work
00a1: pop  ebp
00a2: mov  eax,[edx+4]
00a5: pop  esi
00aa: mov  ecx,edi
00ac: call ?osfx@ostream@@QAEXXZ
```

STEERABLE, and it is the canonical iostream inserter idiom
(`if (os.opfx()) {...} os.osfx(); return os;` - MSVC 5's own `<ostream.h>`
inserters are written this way, so read every `op<<`/`op>>` reconstruction with
it in mind). `operator<<(ostream&, const zBitVec&)` 0x193080 91.48 -> 100.00
EXACT on the one statement move.
