# A forward declaration's `class`/`struct` tag controls the mangled type kind

tags: cpp:decl cpp:class cpp:struct cpp:call | asm:call | topic:identity topic:reloc-fidelity
symptoms: masked instructions are byte-identical, but call relocations name `PAVType` on one side and `PAUType` on the other; undefined-closure reports a declared-only alias for the `PAV` form
confidence: 10/10

MSVC 5.0 records the aggregate tag visible when it parses a function signature:
`class T` is encoded as `V` and `struct T` as `U`. A later declaration using the
other tag does not repair signatures already parsed in that translation unit.
The compiler accepts the mixed declarations, so the defect can survive ordinary
compilation and differ only in COFF symbol identity.

```cpp
// Wrong when the owning definition is `struct CInputDeviceGroup`.
class CInputDeviceGroup;

// Right: use the owning declaration's tag everywhere.
struct CInputDeviceGroup;
```

The controlled A/B was `CInputState::SelectDevices` at 0x000383b0. With the
`class` forward declaration, all 0x1c0 bytes and all instruction counts matched,
but its three calls relocated to:

```text
?CreateDeviceGroup@DirectInputMgr2@@QAEPAVCInputDeviceGroup@@...
```

The defining object and retail instead named `...QAEPAUCInputDeviceGroup@@...`.
That left a declared-only `PAV` alias, and strict relocation scoring read 99.9171%.
Changing only the forward tag to `struct` redirected all three relocations to the
defined `PAU` symbol, removed the alias, and restored 100% exact.

Detection is mechanical: when `gruntz walls diagnose` reports REFERENT with
identical masked bytes and the names differ only by `PAV` versus `PAU`, search all
forward declarations of that type and make their tags agree with its one real
definition. Do not add an alias or change the function's semantic return type;
the relocation is identifying a declaration mismatch.
