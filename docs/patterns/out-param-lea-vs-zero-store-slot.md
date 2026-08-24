# The out-param `lea`/zero-store slot IS reachable - by ownership, not by spelling

tags: cpp:local cpp:call cpp:inline msvc5:mfc | asm:lea asm:mov | topic:codegen-idiom topic:scheduling
symptoms: a `Map.Lookup(key, out)` site at 88-98% whose ENTIRE residue is that retail emits
`lea r,[out]` before `mov [out],0` (or sinks the store one further slot into the argument
pushes), or hoists one step of the receiver chain above the key load, and cl emits them the
other way round; identical size, identical instruction multiset, `walls diagnose` says
REGALLOC/SCHEDULING
confidence: 9/10 (three named sites closed to EXACT)

```cpp
CObject* found = 0;
m_spriteMgrHolder->m_workerMap->m_map1.Lookup(name, found);
if (found) { return 1; }
```
```asm
; retail 0xe2d32                      ; cl
lea  eax,[esp+0x50]                   mov  DWORD PTR [esp+0x50],0x0
mov  DWORD PTR [esp+0x50],0x0         lea  eax,[esp+0x50]
mov  ecx,DWORD PTR [ecx+0x18]         mov  ecx,DWORD PTR [ecx+0x18]
push eax                              push eax
```

## This entry used to say PARKED. That verdict was wrong.

The four probes it recorded (declaring the out-local above the guard, hoisting the receiver
into an anonymous local, moving the `char buf[0x40]` declaration, a 0..15 declaration-count
sweep) are all CALLER-SPELLING probes. None of them changes who OWNS the local, which is the
only lever that reaches this slot - see
[out-param-null-init-belongs-to-an-inline-helper.md](out-param-null-init-belongs-to-an-inline-helper.md).
All three sites this entry named are now EXACT:

| function | before | after | lever |
|---|---|---|---|
| `CSpriteRefTable::Add` 0xe2890 | 97.50 | **100.00** | typed-return `LookupWorker(CMapStringToOb&, LPCTSTR)` inline |
| `CSpriteRefTable::LoadGruntzPalette` 0xe2d10 | 96.92 | **100.00** | the same helper, called for its truth value |
| `CGrunt::EnsureStruckSlot` 0x57b70 | 88.22 | **100.00** | `CDDrawSurfaceMgr* world = g_gameReg->m_world;` - a NAMED local |

The two in-tree EXACT controls the old entry cited (`CDDrawChildGroup::AttachSprite`
0x159830, `MidiManager::FindSequence` 0x138730) are still EXACT with the caller-owned
spelling. They prove the tie exists, not that it is unbreakable: both orders cost the same
5 Pentium issue slots (`mov [esp+disp8],imm32` carries both a displacement and an immediate,
so it is unpairable either way), and which side of the tie cl lands on is decided by which
IL the store belongs to.

## The second lever: hoist the receiver into a NAMED local

`EnsureStruckSlot` is the control that separates the two levers - it has no helper and its
zero-store is already before the pushes on BOTH sides. Its whole residue was that retail
loads `g_gameReg->m_world` immediately after the guard and cl loads it three instructions
later:

```asm
; retail                              ; cl
mov eax,ds:g_gameReg                  mov eax,ds:g_gameReg
mov ecx,[eax+0x10]                    mov ecx,[eax+0x10]
test ecx,ecx / je                     test ecx,ecx / je
mov eax,[eax+0x30]        <- hoisted  mov edx,[esp+0xc]
mov edx,[esp+0xc]                     mov DWORD PTR [esp+0x4],0
lea ecx,[esp+0x4]                     mov eax,[eax+0x30]
mov DWORD PTR [esp+0x4],0             lea ecx,[esp+0x4]
```

`CDDrawSurfaceMgr* world = g_gameReg->m_world;` as its own statement puts the load where
retail has it and closed the function. The same edit took `CSpotLight::SerializeMove`
0xb2050 98.06 -> 99.96 and simultaneously fixed a whole-function EBX/EBP swap: with `world`
declared before the mode `switch` it becomes a call-crossing value that binds a callee-saved
register, which is why retail's parameter binding order differs from a body that re-reads
`reg->m_world` inside one arm. Read the retail disassembly for a member load hoisted ABOVE
the block that uses it - that is the source declaring a local, not the scheduler.

related: [out-param-null-init-belongs-to-an-inline-helper.md](out-param-null-init-belongs-to-an-inline-helper.md),
[out-param-reset-between-arg-setup-and-call-is-in-the-helper.md](out-param-reset-between-arg-setup-and-call-is-in-the-helper.md),
[repeated-member-expression-is-an-inline-helper.md](repeated-member-expression-is-an-inline-helper.md)
