# Copy an embedded struct member via `*p = *src` through a `&member` pointer to get `lea`+literal-zero
tags: cpp:member cpp:local | asm:lea asm:mov | topic:codegen-idiom topic:regalloc
symptoms: target `lea edx,[ecx+N]` (member base in a reg) + literal `mov [ecx+M],0` / `cmp [reg],0`, while recompile pins 0 in a callee reg and uses field-by-field stores off `ecx`
confidence: 7/10

When an init copies an embedded fixed-size struct member (a rect/box) and then
re-checks it, writing the copy as four scalar field stores off `this` lets cl pin a
zero register (`xor edx,edx`) and reuse it for the later `m_x = 0` stores and the
all-zero check (`cmp edx,[ecx+N]`) — diverging from retail, which instead takes the
member's address once (`lea edx,[ecx+N]`), copies through it with a separate temp,
stores the zeros as **literal** immediates, and tests the first field through that
pointer (`cmp [edx],0`). Reproduce retail by giving the member an explicit
`AmbientBox* p = &m_box1;` pointer, doing the copy as a whole-struct assignment
`*p = *src;` (cl emits the 4-mov copy through `lea`+temp, NOT rep movs for 16 bytes),
the no-source sentinel store through `p`, and the first field of the all-zero check
through `p` — so `p` stays live in edx and no zero gets pinned.

```cpp
AmbientBox* p = &m_box1;
if (box != 0) {
    *p = *box;                    // lea edx,[ecx+0x18]; mov edi,edx; 4x (mov ebx,[eax+k]; mov [edi+k],ebx)
} else {
    p->left = (i32)0x80000000;    // mov [edx],0x80000000
}
if (p->left == 0 && m_box1.top == 0 && m_box1.right == 0 && m_box1.bottom == 0)
    p->left = (i32)0x80000000;    // cmp [edx],0 ; mov eax,[ecx+0x1c]; test eax,eax; ...
```
STEERABLE. The field-by-field copy (`p->left=box->left; ...`) does NOT suffice — cl
folds `p` back to `ecx+0x18` and pins the zero. The whole-struct `*p = *box` keeps the
`lea` pointer + a copy temp. Evidence: CRandomAmbientSound::Setup 0xbe50, 58%→100%.
