# Unoptimized math helper: full /Od frame yet `__CIxxx` FP intrinsic ⇒ `/Od /Oi`

A small mis-homed math/helper function whose body is unmistakably *unoptimized*
(full `push ebp;mov ebp,esp;sub esp,N` frame, every arg/local round-tripped
through memory, **plain `idiv` by a constant — no magic-number strength
reduction**, plain `mov reg,imm` constant loads) — **but** which still calls the
x87 FP intrinsic (`__CIpow`/`__CIacos`/…, arguments on the FP stack, no `_pow`
library push/call). Pure `/O2` gives `__CIpow` but magic-division + frameless;
pure `/O1` gives `idiv` + frame but the *library* `_pow` (stack args); neither
fits. The combination is `/Od` (no opt) **plus** `/Oi` (enable intrinsics).

WRITE: put the function in its own TU and compile it with a `/Od /Oi` flag
profile (`odi = ["/nologo","/c","/Od","/Oi","/MT"]`). Source is the natural
literal form (`pow(2.0, -d/10.0)`, `(int)x/100`, `(int)d` → `__ftol`).

SEE (target): `push ebp; sub esp,0x18; cmp [ebp+8],0; … mov ecx,0x64; idiv ecx;
fild [ebp-N]; … fld c0; fld [ebp-N]; fchs; fdiv c1; call __CIpow; … call __ftol`.

STEERABLE via the flag profile (`topic:flags`). Evidence: ComputeCmdPercent
(0x135110, a percentage curve mis-homed onto CGruntzSingleCommand) — /O2 34%,
/O1 19.5%, **/Od /Oi 100%** (byte-exact).
