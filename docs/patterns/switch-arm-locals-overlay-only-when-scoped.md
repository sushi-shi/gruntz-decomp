# Sibling scopes can change stack-slot reuse

When alternative branches use distinct locals, declaring those locals inside
their respective scopes can change the frame and stack displacements:

```cpp
switch (mode) {
case Save: {
    int value = ReadValue();
    SaveValue(value);
    break;
}
case Load: {
    Object* result = LoadObject();
    UseObject(result);
    break;
}
}
```

The [recorded SerializeFields A/B](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/switch-arm-locals-overlay-only-when-scoped.md)
reports a smaller matching frame after moving arm-specific declarations from
function scope into their arms.

This demonstrates a scope-sensitive allocation, **not** the old universal claim
that VC5 overlays slots only for disjoint lexical scopes. A shared retail slot
does not uniquely recover source scope, and a frame-size difference can also
come from spills, temporaries, alignment, or EH.

Compare actual slot accesses and complete lifetimes, accounting for changing
push depth. Test meaningful local ownership; do not add arbitrary blocks solely
to steer allocation.
