# Constructor call counts need a calibrated inline-budget harness

- **confidence** c10 for the measured compiler and tool behavior; the original constructor declarations remain unresolved
- **tags** `cpp:ctor` `cpp:inline` `cpp:new` `msvc5:mfc` | `asm:call` | `topic:tooling` `topic:codegen-idiom`

A header-inline constructor can both expand and emit an out-of-line body. The
[container-helper closure](repeated-container-call-is-an-inline-member.md) proves
that positively. A failed consolidation of one constructor does not prove that
the original source used tagged overloads, and retaining a tag is not proof of
its original identity.

The reverse mistake is treating every real-compiler call count as a measurement
of `cb`. `1000 / expanded` estimates a callee only for an independently calibrated
flat caller at the 1000 budget floor. Constructor chains, other inline sites,
caller growth, and allocation boundaries can invalidate that interpretation.

## Controlled constructor trials

Starting from `1637616b9` (the shared-helper branch after integrating #87), move
the canonical constructor's unchanged body from its owner `.cpp` into its class
header as an out-of-class `inline`, remove the old definition, and compile the
owner and every object that called it with their `config/units.toml` flags. Keep
the tagged sibling during this first visibility test; it is already inline.
For `CLogicRecord`, remove its second same-signature body in
`LogicRecordCtorInline.h` during the trial. Restore each case before the next.

The table counts direct `call`/tail-`jmp` relocations in those real objects;
standalone bodies are counted separately. Constants, member widths and store
order are unchanged. The named constructors all have historical MAX 100; these
experiments test source consolidation, not new spelling searches at a wall.

| Constructor | Retail RVA | Existing calls | Canonical-inline calls | Standalone after trial |
|---|---:|---:|---:|---|
| `CWapObj(owner,id,flags)` | 0x156cb0 | 8 | 2 | 32 B in `ddrawsubmgrpages`, formerly `ddrawsubmgr` |
| `WwdDirtyRect()` | 0x15b270 | 3 | 0 | absent |
| `WwdGridNode()` | 0x15b2a0 | 2 | 0 | absent |
| `WwdRegion()` | 0x15b2b0 | 1 | 0 | absent |
| `CResolveNode(owner,id,flags)` | 0x15b2c0 | 3 | 0 | absent |
| `CLogicRecord(owner,id,flags)` | 0x15b300 | 3 | 0 | absent |
| `CAniAdvanceCursor(owner,id,flags)` | 0x15b730 | 1 | 0 | absent |
| `CMotionState()` | 0x136d0 | 3 | 0 | absent |
| `CUserLogic()` | 0x138d0 | 45 | 39 | 75 B, same owner |
| `CUserLogic(object)` | 0x58cd0 | 3 | 0 | absent |
| `CMovingLogic()` | 0x13940 | 1 | 0 | absent |

The `CWapObj` row also replaces the tagged base-initializer calls across the
consumer family, as in the handed-off trial. All 206 affected consumer objects
were rebuilt. Its eight retail calls are four in `CDDrawSurfaceMgr::Init`, two
in `CDDrawSubMgrPages::CreateChildren`, and one each in `ReadPlaneObjects` and
`CreateContainerObject`; only the two `CreateChildren` calls remain.

The dependent WWD family was also compiled **together**: make `CWapObj`,
`WwdDirtyRect`, `WwdGridNode`, `WwdRegion`, `CResolveNode`, `CLogicRecord`,
`CAniAdvanceCursor`, and `CGameObject` canonical constructors header-inline.
Use the now-visible canonical `CResolveNode` and default region/shadow members
inside `CGameObject`. This composition leaves 3/8 `CWapObj` calls and 1/3
`CResolveNode` calls, but removes all calls and all emitted bodies for the other
six constructors, including `CGameObject`'s three retail calls. It does not
recover the required nested topology.

These tests reject the tested consolidation, not all possible original inline
source. Keep the existing boundaries pending positive evidence for missing
helpers, visibility, allocation sites, or a complete surviving family. Do not
retain artificial statements, emitter-only uses, or the disposable definitions.

## What `--measure-cb` actually observed

Use the trial header in a disposable TU with 30, then 64 source statements of
`consume(new T(arguments));`, where `consume(T*)` is an external declaration.
Run `gruntz walls inline-model --measure-cb harness.cpp --fn <ctor-symbol>
--caller callerX --sites N`. Inspect the emitted caller and constructor symbols
as well as the command output. For abstract `CGameObject`, construct its real
`CWwdSpriteObject` derived class using the composed header family.

| Callee | Expanded / total | Old inferred `cb` | Expanded / total | Old inferred `cb` |
|---|---:|---:|---:|---:|
| `CResolveNode` | 24/30 | 41 | 52/64 | 19 |
| `CUserLogic(object)` | 10/30 | 91–100 | 21/64 | 46–47 |
| `CMovingLogic()` | 29/30 | 34 | 62/64 | 16 |
| composed `CGameObject` | 3/30 | 251–333 | 7/64 | 126–142 |

The disjoint intervals for an unchanged body refute the measurement's assumed
fixed caller budget. A purported budget cutoff at `cb=19` additionally
contradicts the model's own `cb<=40` exemption. The other tested constructors
expand all 30 and all 64 heap sites; this observes no cutoff and does not by
itself prove an exempt callee size. A body may itself expand while leaving
nested helpers called.

## MFC placement-new negative control

On the pinned compiler, this one inline constructor expands through ordinary
`new` but stays a call through placement `new`:

```cpp
#include <Mfc.h>
#include <new>
struct ConstructorSiteControl : CObject {
    int id, flags;
    void* owner;
    virtual ~ConstructorSiteControl() {}
    ConstructorSiteControl(void* o, int x, int f) { id=x; flags=f; owner=o; }
};
void consume(ConstructorSiteControl*);
void heapSite(void* o, int x, int f) {
    consume(new ConstructorSiteControl(o,x,f));
}
void placementSite(ConstructorSiteControl* p, void* o, int x, int f) {
    new(p) ConstructorSiteControl(o,x,f);
}
```

`--measure-cb` formerly called the first result budget-exempt and the second
**NOT an inline candidate**. The second conclusion is disproved by the first
site. The actual `CWapObj` trial repeats the difference at 1, 8 and 30 sites
(placement also retains all 64). Controls without `CObject`, including a
polymorphic class with the same fields, expand both sites. This identifies the
MFC allocation path as relevant; it does not establish which compiler internal
rule causes the difference. Do not generalize it to all placement constructors.

## Tool contract and reverse use

`--measure-cb` now reports the call census without automatically inferring `cb`
or eligibility. `--flat-floor-budget` explicitly supplies the independently
established flat-leaf/1000-budget assumptions; only a partial, consistent cutoff
then produces a **conditional** interval. All-expanded and all-rejected cases
remain unquantified. A missing or ambiguous caller is an error, not evidence
that every supplied site expanded.

The real-VC5 `InlineMeasureControls` execute the command through its compiler,
assembly reader and printed verdict. They cover the MFC allocation-site pair,
missing/ambiguous callers, and the calibrated flat-harness opt-in. The existing
model arithmetic and address-based candidacy controls remain separate.

Before removing a constructor twin, establish its retail callers (following ILT
thunks), count its real-TU call sites and standalone emitters, and test the
complete nested family. Calibrate the harness before using a numerical deficit
to propose a source change. A disappearing body or a different ordered call
population is a structural rejection even when a small test emits plausible
bytes or the existing historical-MAX bank remains green.
