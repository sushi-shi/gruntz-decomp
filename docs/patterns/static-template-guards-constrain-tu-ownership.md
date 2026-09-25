# Static-template teardown guards constrain TU ownership

## Detection signature

An instantiation-only `.cpp` can manufacture exact COMDATs without establishing
an original source owner. A data-only holding file can also conceal a real
initialization boundary. Before removing either, census the natural consumer
objects and follow the retail initialization and teardown references.

## Controlled VC5 evidence

PR #79 removed three files containing only includes, explicit class
instantiations and address pins: `ZDArrayDerived.cpp`, `ArraySerialize.cpp` and
`RezBufferObject.cpp`. Eleven of their thirteen functions already had natural
emitters: ActionArea's action-handler array, CreditsState's playlist array and
Fader's mesh array. The playlist constructor at `0x94340` and mesh `SetSize` at
`0x17f390` did not: their consumers currently inline those bodies. Their forced
claims were removed, not replaced with dummy calls or compiler switches.

The playlist destructor is an important negative control. The holding object
emitted the retail-sized 81-byte EH body; the real CreditsState and GruntzMgr
consumers emit a 33-byte body. Natural emission proves a consumer exists, not
that its current compilation context recovers the retail COMDAT. Retain that
open discrepancy and the historical maximum instead of restoring the fake TU.

The former `LogicDispatchInit.cpp` owns the eye-animation registry at `0x246060`.
Retail's two 31-byte teardown bodies prove these separate identities:

| Teardown | Registry | Guard | Mask |
| --- | --- | --- | --- |
| `0xacb80` | eye animation, `0x246060` | `0x245f34` | 1 |
| `0xad180` | front animation, `0x2460b0` | `0x245f30` | 1 |

Compiling the two existing explicit static-member specializations separately
produces one guard and mask 1 in each object. Combining those same definitions
in one object shares a guard and assigns masks 1 and 2. A primary-template
definition with `= CActReg(ACT_ID_FIRST, ACT_ID_LAST)` does produce separate
teardown guards, but adds guarded initialization and combines the initializers;
it does not reproduce retail's separate 10/21/14-byte initialization helpers.
Thus separate guards alone are not a general proof of separate original TUs.
The complete initializer/teardown topology rejects both tested merged forms.

The retained model puts each animation registry beside its class methods.
Neighboring `CFrontCandy`, `CDoNothing`, `CBehindCandy` and `CEyeCandy`
constructors return to their own class implementations. Their retail dispatch
callers and vptr stores corroborate those identities; none of their bodies is
rewritten. Original filenames remain inferred. This supersedes the old
FrontCandyAni F-E-F interval argument: its initial F was the different class
`CFrontCandy`, and its assertion that EyeCandyAni had no private storage or
initializers was false.

The asset-root CString moves from `StringStaticPool.cpp` to its SplashState
consumer. Its teardown at `0xf9750` still refers to object `0x24e25c`, guard
`0x24e218`, and mask 1. The declaration belongs with AssetRoot, not Net.

## Safe reverse use and controls

1. Census real consumer COFFs before deleting an emitter-only file; distinguish
   absent standalone bodies from missing template source definitions.
2. Compare all initialization helpers, teardown bytes, ordered relocation
   identities and DIR32 addends. Do not infer ownership from address adjacency.
3. Resolve a failed merge through real class ownership, not a renamed holding
   file or a manually manufactured guard. Preserve uncertain original identities.
4. Correct the consumed data manifest as well as source pins. A matching masked
   body does not prove that the compiler-generated guard binds to the right datum.

`scripts/test_tu_ownership.py` tests natural emission, follows the actual Model
through compiled guards to retail bytes/relocations, and recompiles the
separate/merged negative control with the pinned compiler. The compiler-artifact
gate additionally rejects instantiation-only source files, while allowing
instantiations alongside real implementation or storage.

```sh
nix develop -c gruntz build
nix develop -c python3 -m unittest discover -s scripts -p test_tu_ownership.py -v
```
