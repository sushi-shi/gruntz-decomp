# /GX dtor: the entry trylevel-0 write is scheduled BEFORE the vptr re-stamp — vs retail's stamp-first

tags: cpp:dtor cpp:eh cpp:virtual | asm:mov | topic:codegen-idiom topic:eh
symptoms: body+offsets byte-identical except the compiler-generated entry `mov [esp+N],0` (trylevel=0) and the user's `mov [this],<vtbl>` vptr re-stamp are emitted in the OPPOSITE order vs retail; ~94%
confidence: 7/10

> **RESOLVED — STEERABLE.** This is NOT a wall when the stamp is a MANUAL
> `*(void**)this=&g_vtbl`: make the class a real polymorphic type (`virtual ~Class()`,
> drop the manual stamp) so cl emits the COMPILER-implicit stamp, which lands in the
> entry state (stamp-first) like retail. See **eh-dtor-implicit-vptr-stamp-first.md**.
> The "not steerable" note below applies only to a *non-convertible* manual stamp.

In a `/GX` (`flags="eh"`) destructor that (a) restores the most-derived vptr (`*(void**)this =
&g_vtbl`, the manual transitional stamp), (b) calls an out-of-line teardown helper, then (c)
destroys a member with a non-trivial dtor (`CByteArray m_94`) — the destructor body sits in EH
state 0 (covering the helper call) and transitions to state -1 right before the guarded member
dtor. MSVC5 emits the entry `mov [esp+N],0` (trylevel = 0) and the user's vptr re-stamp `mov
[this],&vtbl` in a fixed order decided by the /GX EH-state machine.

Retail (MSVC 5.0 SP3) emits the **vptr stamp FIRST** (in the entry state), then `[esp+N]=0` right
before the first guarded call:
```
mov [esi],<vtbl>      ; user vptr re-stamp (entry state)
mov [esp+0x10],0      ; trylevel = 0
call FreeSurfaces
lea ecx,[esi+0x94]
mov [esp+0x10],-1     ; trylevel = -1
call ~CByteArray
```
The recompile emits `[esp+N]=0` FIRST, then the vptr stamp:
```
mov [esp+0x10],0      ; trylevel = 0   <-- swapped
mov [esi],<vtbl>      ; user vptr re-stamp
call FreeSurfaces
...
```
The two `mov`s are swapped; everything else (frame, the two trylevel transitions, both `call`
relocs, the epilogue) is byte-identical, so the function caps ~94%.

NOT steerable by source spelling — making the vptr stamp the first / last body statement, or
writing the member dtor explicitly, all leave the entry trylevel write ahead of the stamp. The
order is the /GX EH-state machine's, a cousin of [eh-ctor-vptr-restamp-position](eh-ctor-vptr-restamp-position.md)
(the ctor analog: there the stamp is sunk LATE; here the trylevel-0 write is hoisted EARLY — same
root cause, the vptr store is never observed by the body so the scheduler is free to place it).

WALL. Evidence: CFileImage::~CFileImage (image unit, 0x141350) — manual vptr stamp to 0x5ef7f0 +
out-of-line FreeSurfaces (0x13e4d0) + CByteArray m_94 member dtor; 94% with ONLY the
stamp/trylevel-0 pair swapped.
