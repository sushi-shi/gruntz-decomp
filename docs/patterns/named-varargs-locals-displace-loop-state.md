# Named varargs payload locals can displace an enclosing loop's live state
tags: cpp:local cpp:varargs cpp:loop | asm:mov asm:push asm:sub | topic:codegen-idiom topic:regalloc
symptoms: a long varargs call has the right positional member loads, but retail's frame is eight bytes larger and its enclosing loop counter plus strength-reduced element offset are stack-homed
confidence: 9/10
variants: call-killed-invariant-is-a-source-local.md, statement-schedule-faithful.md, integer-sum-term-order-is-inert.md

A long varargs argument list does not imply that every member was read directly
at its eventual `push`. Some payloads can be named source locals whose lifetime
begins before an intervening switch or inline helper. Those values consume the
registers that the enclosing loop state otherwise occupies, so cl 5.0 homes the
loop state even though the loop source itself is unchanged.

`CNetSession::BuildGruntzCrcInfo` 0xbf1d0 is the controlled example. Retail and
the initial reconstruction both serialize the same eighteen values to
`wsprintfA`, but the initial body reads `m_vehiclePickupType` and `m_daFlag`
directly at the call. It emits a 0x290-byte function with a 0x10c-byte local
frame. Retail is 0x2a4 bytes with a 0x114-byte frame and stores two live loop
values:

```asm
mov [esp+0x18],edi   ; inner grunt index
mov [esp+0x1c],eax   ; strength-reduced byte offset into the roster
```

The source shape that reproduces those homes names the two positional payloads
immediately after the priority switch, in argument order, before selecting the
arrival pickup:

```cpp
i32 wp;
PRIO(wp, type);
b32 da = grunt->m_daFlag;
PickupType toy = grunt->m_vehiclePickupType;
PickupType tool = ArrivalPickupOf(grunt, type);
wsprintfA(buffer, format, /* ... */, tool, toy, da, wp, /* ... */);
```

The bounded lifetime matrix separates this from a padding accident:

| payload lifetime | bytes | disposable-object fuzzy |
|---|---:|---:|
| both direct at the call | 656 | 87.4385% |
| `da` only named | 660 | 94.9626% |
| `toy` only named | 660 | 92.6685% |
| `toy`, then `da` | 676 | 98.1283% |
| `da`, then `toy` | **676** | **99.2620%** |
| both named after `tool` | 656 | 87.4385% |

The last high state has retail's exact byte size, instruction count, call/branch
topology, frame, payload schedule, and loop homes. A full relocation-aware build
scores it 99.9198%; its only residue is the switch-index scratch register
(`EAX` in base, `ECX` in retail). Ten enum/integer, direct-member, ternary, and
shared-helper selector spellings, an inline priority wrapper, and 128 depth-1/2
structural variants are flat at the same state. That residue is the C2 rotating
cursor, not evidence against the payload locals.

Reverse-use rule: when retail has exactly two extra dword homes carrying live
loop state around a long varargs call, inspect payload fields loaded before an
intervening switch/helper. Name only fields whose load order and later positional
push prove the lifetime. Do not add dummy locals or padding merely to enlarge the
frame; the one-local and reversed-order controls must move the register pressure
in the predicted direction.
