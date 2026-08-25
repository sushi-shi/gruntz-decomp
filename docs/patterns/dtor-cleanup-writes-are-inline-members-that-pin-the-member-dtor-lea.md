# A destructor's cleanup writes are inline members - that is what pins the member-dtor `lea`

tags: cpp:dtor cpp:member cpp:inline | asm:lea asm:mov | topic:codegen-idiom topic:scheduling
symptoms: a destructor stuck at 90-94% whose instruction multiset is IDENTICAL to retail and whose whole residue is that `lea ecx,[esi+N]` - the `this` of a member sub-object's destructor - sits at the HEAD of the preceding reset-store run in our compile and in the MIDDLE of it in retail
confidence: 10/10

## The mechanism

`lea ecx,[this+N]` computing the receiver of an embedded member's destructor is a
one-instruction chain with no successors until the `call`. cl 5.0's list scheduler
therefore floats it into the run of independent stores that the destructor body
emits just before it. Written longhand, that run is one flat list of statements and
the `lea` hoists over ALL of them; retail places it partway in.

The lever is the same inline-expansion boundary as
[ctor-body-first-statement-is-an-inline-member.md](ctor-body-first-statement-is-an-inline-member.md),
read from the destructor side: statements grouped inside an expanded inline member
form ONE scheduling unit, and the `lea` lands at a unit boundary instead of at the
head of the whole run.

`CGameObject::Unload()`, expanded into four destructors, resets two `WwdDirtyRect`
members and one scalar. `WwdDirtyRect::Reset()` already existed in the header and
`CResolveNode::Unload()` already called it; only `CGameObject::Unload()` had the
two fields written out:

```cpp
// before - the lea hoists over all five stores
m_shadow.m_rect.left = COORD_UNSET;
m_shadow.m_armed = -1;
m_screenX = COORD_UNSET;
m_dirty.m_rect.left = COORD_UNSET;
m_dirty.m_armed = -1;

// after - two expansion units with one statement between them; the lea lands between
m_shadow.Reset();
m_screenX = COORD_UNSET;
m_dirty.Reset();
```

Retail, `CGameObject::~CGameObject` 0x15b4f0:

```
mov [esi+0xc0],edi      ; m_shadow.m_rect.left  \ Reset() unit
mov [esi+0xd8],ebx      ; m_shadow.m_armed      /
mov [esi+0x5c],edi      ; m_screenX
lea ecx,[esi+0xdc]      ; <- the CString member dtor's `this`
mov [esi+0x20],edi      ; m_dirty.m_rect.left   \ Reset() unit
mov [esi+0x38],ebx      ; m_dirty.m_armed       /
mov BYTE PTR [esp+0x1c],0x2
call CString::~CString
```

## Measured, one build each

`CGameObject::~CGameObject` 90.26, `CWwdSpriteObject::~CWwdSpriteObject` 92.92,
`CWwdDeferredObject::~CWwdDeferredObject` 93.57, `CWwdDotObject::~CWwdDotObject`
93.63 - **all four to 100.000 EXACT**. All six `Unload()` overrides (which are real
out-of-line functions of their own) stayed at 100.000, so the callee itself is
unaffected.

Negative control that pins the mechanism to BOTH boundaries, not one: keeping
`m_dirty.Reset()` but writing `m_shadow` longhand dropped
`CWwdSpriteObject::~CWwdSpriteObject` 100.000 -> 97.35 and left the others where the
full form had already put them. The `lea`'s position is decided by the unit list,
so a half-converted run is not a half-fix.

## Cost: the deepest destructor pays the /Ob1 budget

`CWwdGameObject::~CWwdGameObject` 0x15bd10 destroys four bases and two members and
so expands `CGameObject::Unload()` twice. It moved 69.83 -> 71.54 and remains a
wall of a different class: `walls diagnose` reports INLINE/CALL-SET - retail
EXPANDS `CGameObject::Unload()` at the `~CGameObject` sub-object step and CALLS
`CWapObj::~CWapObj`, while we CALL `CGameObject::Unload` there and then have budget
left to EXPAND `~CWapObj`. Two `Reset()` calls raise the callee's front-end cb, so
the sequential budget (`clamp(2*cb(caller),1000,35000)`, spent in tuple order)
runs out one site earlier. Retail's `~CWapObj` has exactly TWO retail call sites
(`gruntz sema xref 0xd5d70` -> the ILT thunk -> `CWapObj::\`vector deleting dtor'`
and this destructor), which proves it IS an in-class inline that cl declined
exactly once - it is not an out-of-line definition, so "make it non-inline" is
refuted.

Four exact destructors for one that moved 1.7 is the right trade; the remaining
site is a budget question, not a schedule one.

related: [ctor-body-first-statement-is-an-inline-member.md](ctor-body-first-statement-is-an-inline-member.md),
[repeated-container-call-is-an-inline-member.md](repeated-container-call-is-an-inline-member.md),
[inline-budget-emits-ool-comdat.md](inline-budget-emits-ool-comdat.md)
