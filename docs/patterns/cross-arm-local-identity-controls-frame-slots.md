# Cross-arm local identity controls frame slots

- **confidence** c9
- **tags** `cpp:local` `cpp:branch` `cpp:pointer` | `asm:sub` `asm:mov` | `topic:codegen-idiom` `topic:regalloc`

## Symptom

Two large, mutually exclusive arms perform the same operation in opposite
directions. Each arm has locals with the same semantic role. The reconstruction
declares a fresh local in each arm, while retail has one extra frame dword or
places a long-lived value in a different stack home even though instruction,
branch, call, and relocation topology otherwise agrees.

MSVC 5 does not always erase the source identity boundary between those locals.
Declaring one local before the outer branch and assigning it in each arm can
change frame reservation and slot coloring without changing the executed
operations.

## RenderWarpTile evidence

`CFaderShape::RenderWarpTile` 0x00181e50 has two duplicated directional arms.
On the 88.912480% base, each LUT arm declared its own pointer:

```cpp
if (forward) {
    if (useLut) {
        u8* lut = table->data;
        // ...
    }
} else if (reverse) {
    if (useLut) {
        u8* lut = table->data;
        // ...
    }
}
```

Making the role one cross-arm entity raised the score to 88.938410% and changed
the frame from `sub esp,0x20` to retail's exact `sub esp,0x24`:

```cpp
u8* lut;
if (forward) {
    if (useLut) {
        lut = table->data;
    }
} else if (reverse) {
    if (useLut) {
        lut = table->data;
    }
}
```

A four-cell cross-arm identity control then separated `row` and `base`. Sharing
only `base` raised 88.938410% to 88.962720% and moved both the saved half-width
and the per-arm row-byte base to retail's `[esp+0x24]` home. Sharing only `row`
fell to 88.894650%; sharing both reproduced 88.912480%. Thus the result is not a
generic benefit from hoisting locals.

Further negative controls were decisive: 16 within-arm pointer declaration,
assignment, order, and hoist forms were byte-flat; cross-arm source pointers
were byte-flat; cross-arm destination pointers fell to 88.944890%; and shared
copy counts were flat or slightly worse. The lever is the identity shared by
the mirrored source roles, not declaration height by itself.

## Reverse-use rule

1. Require mutually exclusive arms whose locals have the same semantic role
   and compatible types. Do not merge merely adjacent same-typed temporaries.
2. First prove a retail frame or slot discrepancy that the separate-entity base
   lacks.
3. Test each role independently. Keep only identities that recover a specific
   retail frame or stack-home feature; a score movement alone is insufficient.
4. Preserve separate locals when the controls show different identities, as
   `row` and `dstLine` do here.
