# Mutating a parameter in place binds it to a callee-saved home at entry

tags: cpp:local cpp:param cpp:call | asm:mov asm:sub asm:push | topic:codegen-idiom topic:regalloc
symptoms: a REGALLOC-classed function whose base loads a parameter into a
caller-saved register at the prologue and later copies a derived value into a
callee-saved register (`mov eax,[esp+N]` ... `sub eax,edx; mov ebp,eax`),
while retail loads the SAME parameter straight into a callee-saved register as
the first prologue instruction (`push ebx; mov ebx,[esp+8]`) and mutates it in
place (`sub ebx,edx`); every register downstream rotates and the second
argument's load is sunk into the branch that uses it
confidence: 8/10

## Mechanism

cl 5.0 allocates call-crossing values to ESI/EDI/EBX/EBP in DEFINITION order.
A parameter that the source REASSIGNS (`idx -= at; ... m_lo = idx;`) is one
IL tuple whose definition is the function entry itself, so it takes its
callee-saved home in the prologue and stays there for the whole body. Spelling
the same computation through a fresh local (`i32 lonew = idx - at;`) defines a
NEW tuple later, so the parameter gets a caller-saved scratch home at entry,
the local binds a different callee-saved register, and the whole rotation
shifts. The two spellings also schedule differently: with the in-place form
retail sinks the second operand's stack load into the branch arm that consumes
it; with the fresh-local form cl hoists it to the prologue.

## Detection

`walls diagnose` says REGALLOC with identical multisets; the base/target pair
differ from insn 1: retail `push ebx; mov ebx,[esp+0x8]` vs base
`mov eax,[esp+0x4]; push ebx`. Later, the value retail mutates in place
(`sub ebx,edx`) appears in base as compute-then-copy (`sub eax,edx; mov ebp,eax`).
The final member writebacks (`m_lo = idx`) store from the parameter's register
on the retail side.

## Production

`_zvec::GrowTo` 0x16da80 (typekeycoll): replacing `i32 lonew = idx - at` /
`i32 hinew = idx + at` with `idx -= at` / `idx += at` (both arms) moved
82.64 -> 91.86 in one build; the arm-1 register assignment became exact
(idx->EBX, realloc result->EBP as retail). The residue is the second arm's
realloc-result home (EDX vs EBP), a rotation coin: the result does not cross
a call in that arm, so cl gives it a scratch register while retail unified it
with arm 1's callee-saved home.

The same mechanism applies to LOCALS, not just parameters: `CNetSession::Tick`
0xbf9e0 (netsessionmgr) spelled the loop bound base as a fresh
`i32 next = seq + 1` and sat at 86.03 with a `lea ecx,[edi+0x1]` where retail
copies the old value out and increments in place (`mov ebx,edi; inc edi`).
Respelling as `i32 t = seq * m_period; seq = seq + 1; for (; t < seq *
m_period; ...)` took it to 99.87 in one build; the residue is one
load-order coin in the per-iteration bound recompute (which of the two
memory operands of the imul loads first). Detection is the same either way:
base `lea r2,[r1+K]` keeping r1 alive against retail `mov r2,r1` + in-place
`add/inc r1`.

The binding order also decides WHICH callee-saved register the RECEIVER gets, so
the shape reaches `this`-versus-derived-value rows that read as pure colour.
`DSoundVoice::Tick` 0x137060 (directsoundmgr) is 43 instructions on both sides
with the same frame and the same call/branch/ret/reloc quadruple, and its whole
5.70% gap is retail holding `this` in ESI and the elapsed time in EDI while base
does the reverse - visible as `mov esi,ecx` being instruction 2 in retail and
instruction 4 in base. `i32 elapsed = now - m_rampStartTime;` defines a new
call-crossing tuple that binds ESI ahead of `this`; `now -= m_rampStartTime;`
keeps the parameter's own live range, which starts at entry and therefore loses
the tie to the implicit `this` parameter. One build, 95.70 -> 100.00 EXACT. The
detection here is not the prologue load but the ORDER of the two materialisation
moves: both sides emit `push esi; <X>; push edi; <Y>` and only the contents of X
and Y swap.

The SNAPSHOT direction is the same tuple question read backwards. When a
function walks a buffer and returns the number of bytes consumed, retail
copies the incoming pointer out once and then advances the PARAMETER
(`mov esi,eax; inc eax; mov dl,[eax]`), where a derived-cursor spelling
(`char* buf = data + 1; char* start = buf - 1;`) makes cl advance the derived
local and recover the origin afterwards with a trailing `lea esi,[eax-0x1]`.
The origin snapshot must therefore read the parameter directly, and the cursor
must be the parameter: `char* start = data; ++data; ... = *data++; return data
- start;`. `CGruntzSingleCommand::Parse` 0x23f90 and `CGruntzMultiCommand::Parse`
0x24000 (gruntzcmdmgr) are both EXACT with that spelling (93.55 / 93.88 before);
initialising `start` from the cursor instead (`start = data; buf = data + 1`, or
`buf = data; start = buf; ++buf`) drops both to 86.45 / 83.20, because the
parameter then dies at the first derived definition and the allocator loads it
into the callee-saved register directly.
