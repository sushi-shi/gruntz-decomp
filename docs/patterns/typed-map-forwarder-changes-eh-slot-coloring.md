# A typed `CMap*` forwarding helper can change EH slot coloring

tags: cpp:mfc cpp:template cpp:local | asm:lea topic:eh topic:codegen-idiom

When retail calls an MFC map's native `Lookup(key, void*&)` and immediately treats the
result as a typed pointer, preserving the call target and optimized data flow is not enough.
A header-inline typed forwarding helper can give C1 a different local entity graph and move
an adjacent destructible object's unwind receiver.

`CSpotLight::Tick` (`0x000b1af0`) is the measured case. Both sides call
`CMapStringToPtr::Lookup`, have one `CString` state, and reserve `0x18` bytes, but the typed
source

```cpp
LeafCue* cue = NULL;
MapLookup(map, name, cue);
```

put `name` at `esp+0x18` and its cleanup at `[ebp-0x1c]`. Retail puts the string at
`esp+0x10`, passes `esp+0x18` as the map output, reloads that slot into `eax`, and cleans the
string at `[ebp-0x24]`. Restoring the native boundary closes the funclet:

```cpp
void* found = NULL;
map.Lookup(name, found);
LeafCue* cue = static_cast<LeafCue*>(found);
```

The primary function moved 78.8750% to 80.2379%, while the EH census moved one group from
`frame-offset` to `identical` (706/33 to 707/32). This is a type/entity correction, not a
padding technique: the map really stores `void*`, the native API really takes `void*&`, and
the typed pointer is derived only after the lookup.

Negative controls are diagnostic. Moving the typed pointer declaration before the string,
broadening its scope, and putting it in an enclosing scope were byte-identical. Initializing
it before constructing the string hoisted the zero store, fell to 77.2661%, and contradicted
retail's late zero immediately before `Lookup`. If the target shows the native output home
and a post-call typed use, restore that source boundary rather than permuting declarations.

This is not a tree-wide instruction to bypass `MapLookup`. In
`CMenuState::LoadGameAssetNamespaces`, replacing all three typed forwards with direct
`void*&` calls was byte-identical and left its four EH funclets displaced. There the real
entity defect was the enclosing scope of the first typed output; a focused block removed
the surplus frame word while retaining the typed wrapper.
