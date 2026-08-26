# An early initialization failure partitions later `delete` tails
tags: cpp:branch cpp:new cpp:delete cpp:return | asm:call asm:jcc asm:ret | topic:codegen-idiom topic:tail-merge
symptoms: a factory has the right allocation, initialization, registration, and list insertion calls, but base cross-jumps an insertion-failure delete into an earlier cleanup and is short by one deleting-destructor call plus one return epilogue
confidence: 10/10
variants: retail-duplicates-small-return-epilogues.md, single-predecessor-tail-block-gets-replicated.md, allocate-check-then-body-is-the-then-block.md

When a factory has several failure stages, source ownership of the first cleanup
can decide which later `delete; return NULL;` tails cl 5.0 cross-jumps. A positive
success region with one final cleanup is behaviorally equivalent to an early
initialization-failure return, but it is not codegen-equivalent.

`CNetMgr::AddPlayer` 0x178b30 initially used one positive region:

```cpp
CNetPlayerNode* node = new CNetPlayerNode;
if (node->Initialize(/* ... */) != 0) {
    // SetPlayerData, then AddTail
    if (pos == NULL) {
        delete node;
        return NULL;
    }
    return node;
}
delete node;
return NULL;
```

cl cross-jumped the explicit insertion-failure delete into the final cleanup.
The result was 0x125 bytes / 100 instructions / 8 calls / 8 branches / 2
returns, against retail's 0x140 / 110 / 9 / 7 / 3. The absent ten instructions
were exactly one virtual deleting-destructor path and its complete null-return
epilogue.

Give initialization failure its own return before entering the later stages:

```cpp
CNetPlayerNode* node = new CNetPlayerNode;
if (node->Initialize(/* ... */) == 0) {
    delete node;
    return NULL;
}

// SetPlayerData; its error reports and reaches the final cleanup.
// AddTail failure keeps its own delete + return.
```

Now cl merges the initialization and `SetPlayerData` failure cleanups with each
other, but keeps the `AddTail` failure distinct. It also selects retail's two
different vtable scratch registers (`EAX` for the earlier delete, `EDX` for the
insertion failure), which prevents the late machine-tail cross-jump. The natural
early-return form is byte-exact at all 110 instructions.

The controlled source-shape matrix makes the ownership lever explicit:

| initialization/tail form | bytes | calls / branches / returns | result |
|---|---:|---:|---:|
| positive success region | 293 | 8 / 8 / 2 | 88.1651% full build |
| positive truth test | 293 | 8 / 8 / 2 | same compiler island |
| named initialization result | 293 | 8 / 8 / 2 | lower register island |
| positive insertion `if/else` | 300 | 9 / 8 / 2 | destructor restored, epilogue still shared |
| negative guard + `goto` | **320** | **9 / 7 / 3** | 99.9083% raw-object score |
| negative guard + early return | **320** | **9 / 7 / 3** | **100% normalized full build** |

Reverse-use rule: when the missing unit is precisely a later failure's deleting
destructor plus epilogue, assign failure cleanup to the stage that can detect it.
Try a natural early failure return before inventing barriers or distinct null
expressions. Seven null-return spellings on the dipped positive-`if/else` base
were byte-flat; return spelling cannot repair ownership established by the
outer region.
