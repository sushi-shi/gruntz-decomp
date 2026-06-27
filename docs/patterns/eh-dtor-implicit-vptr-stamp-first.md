# /GX dtor: a COMPILER-implicit vptr stamp emits stamp-FIRST — converts the manual-stamp order wall
tags: cpp:dtor cpp:eh cpp:virtual | asm:mov | topic:codegen-idiom topic:eh
symptoms: a /GX `~Class` whose body+frame are byte-identical but the entry `mov [esp+N],0` (trylevel=0) is scheduled BEFORE the vptr re-stamp `mov [this],<vtbl>` (retail is stamp-first); ~93-94% with a MANUAL `*(void**)this=&g_vtbl` stamp
confidence: 9/10
variants: eh-dtor-vptr-stamp-vs-trylevel-order.md, eh-dtor-multilevel-polymorphic-chain.md

This is the STEERABLE fix for `eh-dtor-vptr-stamp-vs-trylevel-order` (which was
mis-classified as a wall). The root cause of the ~94% plateau is that a MANUAL
`*(void**)this = &g_vtbl` stamp is just a body store, so the /GX EH-state machine
schedules it AFTER the entry trylevel-0 write. Retail's stamp is the **compiler's
implicit dtor vptr re-stamp**, which is emitted in the ENTRY state — *before* the
trylevel write. Reproduce it by making the class a REAL polymorphic type (declare
`virtual ~Class()`, drop the manual stamp); cl then emits the implicit stamp-first.

Two sub-cases, by where the SECOND (base/grand) stamp must land:

1. **Single own-stamp** (frame from a non-trivial MEMBER, e.g. a `CPtrArray`/
   `CByteArray` at +0x94): just `virtual ~Class()`, make the +0 vptr explicit in the
   layout (drop 4 bytes of the leading pad), and let the member dtor fold supply the
   frame. The own stamp is the only one and lands stamp-first.

2. **Own + grand stamp, grand stamp AFTER the member resets** (the SeverusWorker /
   "Remus" base family — CImage, CImgHolder): the compiler-implicit BASE re-stamp is
   emitted at the START of the base subobject dtor, so an inline `~Base(){ m_x=…; }`
   would put the grand stamp BEFORE the resets. Retail has it AFTER. Fix: put the
   base-field resets in the DERIVED body and give the base an EMPTY non-trivial inline
   virtual dtor — cl then emits only the grand re-stamp for it, which folds in as the
   LAST store.

```cpp
// sub-case 2 (CImage): own stamp-first, then FreeAll, then resets, then grand stamp
struct CImageBase {            // polymorphic; empty body => folds as JUST the grand re-stamp
    virtual ~CImageBase() {}
    i32 m_04, m_08; void* m_0c;
};
struct CImage : CImageBase {
    virtual ~CImage();         // UAE; cl emits the implicit ??_7CImage stamp at entry
    void FreeAll();
};
CImage::~CImage() { FreeAll(); m_04 = -1; m_08 = 0; m_0c = 0; }  // resets precede the fold
```
```asm
d5e9d: mov [esi],??_7CImage   ; implicit own stamp — ENTRY state, stamp-first
d5ea3: mov [esp+0x10],0       ; trylevel 0  (now AFTER the stamp, == retail)
       call FreeAll
       mov [esi+4],-1 / [esi+8],0 / [esi+c],0     ; derived-body resets
       mov [esi],??_7CImageBase                   ; folded base re-stamp — LAST store
```
The implicit stamps reloc-mask (fuzzy) against the target's differently-named vtable
symbols (`g_imageVtbl`/`g_severusWorkerDtorVtbl` etc.) — the bytes are identical, only
the masked operand name differs, so fuzzy hits 100% even when the shared grand-base
symbol is named by an out-of-scope TU. CAVEAT: if a `??_G`/`ScalarDelete` calls the now-
virtual dtor, qualify it (`this->Class::~Class()`) to keep the DIRECT call, else it
dispatches through the vtable (`push 0; call [vptr]`) and regresses.

STEERABLE. Evidence: `~CImage` 0xd5e80 94.3→100, `~CFileImage` 0x141350 / `~CFileImageSurface`
0x142360 94→100 (sub-case 1), `~CImgHolder` 0x16500 93.3→100 (sub-case 2). Resolves the
eh-dtor-vptr-stamp-vs-trylevel-order wall for the manual-stamp case.
