# Separate a shared return result from the successful working pointer

**Tags:** `cpp:return` `cpp:local` `cpp:pointer` `cpp:goto` | `asm:xor` `asm:mov` `asm:push` | `topic:codegen-idiom` `topic:regalloc`
**Confidence:** 10/10

## Symptom

A `/GX` factory has one candidate and retail epilogue, and its calls and branch
skeleton already agree, but the register plan does not:

```asm
candidate failure: xor esi,esi ; jmp epilogue
retail failure:    xor eax,eax ; jmp epilogue
```

The candidate reuses that same esi value for the object constructed on the
success path. Retail starts using esi as the successful object only after the
allocation, leaving another callee-saved register available for a long-lived
argument.

## Cause and fix

One source pointer is serving two different jobs: the common return phi and the
post-allocation working object. VC5 coalesces both jobs into one callee-saved
register. Preserve the distinct live ranges visible in retail:

```cpp
T* result;
if (bad0) {
    result = 0;
    goto done;
}
if (bad1) {
    result = 0;
    goto done;
}

{
    T* value = new T(...);
    value->Finish();
    result = value;
}
done:
return result;
```

The block is material: VC5 rejects a `goto` that jumps across a declaration with
an initializer. Do not use this spelling merely to rotate registers. Require the
full signature: separate retail failure/result and success/working registers,
one shared epilogue, and an argument that retail holds across the successful
calls but the candidate reloads.

## Evidence

`SoundDevice::CreateBuffer` (`directsoundmgr`, `0x001366f0`) began at 65.5630%.
Changing the initialized shared pointer into one uninitialized pointer assigned
in every failure arm restored the branch skeleton but still used esi for both
roles (77.4202%). Splitting the return result from the successful voice restored
retail's eax/esi/ebp plan and reached 96.5546%. Removing an unsupported COM
out-parameter preinitialization and restoring the `WaveFormatX::cbSize` source
order closed the remaining two instructions: 100.00% exact at the retail
360-byte extent.

This is the inverse companion to
[`single-return-variable-pins-accessor-regalloc.md`](single-return-variable-pins-accessor-regalloc.md):
that pattern merges independent return values to create one desired live range;
this pattern separates a return phi from a success-only working value when
retail proves that they were never one pseudo.
