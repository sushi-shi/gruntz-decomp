# Rebind a pointer parameter when retail preserves the source and consumes the replacement

**Tags:** `cpp:pointer cpp:parameter cpp:local cpp:call | asm:mov asm:push | topic:codegen-idiom topic:regalloc`
**Confidence:** 8/10 — recovered on CGameLevel::LoadWwd (0x15d280).

## Symptom

A transform or decompression call returns a replacement buffer. Retail keeps the original
input pointer in a stack home for later metadata reads, but carries the returned replacement
in the same callee-saved register that previously held the pointer parameter. A source with
separate `source`, `result`, and byte-view locals instead gives all three independent live
ranges and rotates the register and stack-home assignments through the rest of the function.

## Source shape

Preserve the original pointer explicitly, then rebind the pointer parameter to the returned
object and derive the byte view from that rebound parameter:

```cpp
Header* source = header;
char* block = reinterpret_cast<char*>(source);

if (source->flags & COMPRESSED) {
    header = reinterpret_cast<Header*>(Inflate(source, buffer, capacity));
    if (header == NULL) {
        return 0;
    }
    block = reinterpret_cast<char*>(header);
}

UseMetadata(source);
UsePayload(block);
```

This is not merely renaming. Reassigning the parameter tells cl that the transformed object
continues the parameter's value role, while `source` is the distinct long-lived value that
must survive the call.

## Evidence and boundary

In CGameLevel::LoadWwd, this change made retail's original-header stack home and decompressed
block register agree and raised the function from 86.15% to 91.22%; the accompanying capacity
expression had already raised it from 84.76%. All ten ordered relocations agree. The remaining
differences are stack-home and loop-register colouring: an alternate cursor/index spelling was
byte-identical, and all 14 eligible states in a 60-trial mixed TU probe stayed at 91.22%. Keep
the pointer-lifetime correction and park that bounded residual instead of distorting ownership.
