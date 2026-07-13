# Object construction: model a real ctor — spelled-out `new`+stores (or an un-inlined helper) mis-colors the whole factory
tags: cpp:ctor | cpp:vtable | cpp:inline | asm:mov | topic:codegen-idiom | topic:regalloc
symptoms: factory `new Class` + field stores stuck ~62–84%, `call <ctor-helper>` where retail inlines, vptr `mov [obj],??_7Class` NOT first, esi/edi/ebx this/arg/result rotation, `push <size>;call ??2@YAPAXI@Z`
confidence: 9/10

A "factory" allocates an object, seeds a few fields from its args/`this`, then calls a
method on it (`new T; obj->f1=a; obj->f2=0; …; obj->Method()`). If you write the seed as
**spelled-out stores after `new T`** (or hide it in a `static inline` helper), MSVC5 does
NOT reproduce retail: it either emits a `call` to the helper (cl declines to inline it), or
it inlines the construction but colors `this`/arg/result into the WRONG callee-saved
registers and copies the `new` result to a temp. Either way the whole body mis-aligns
(~62–84%). Retail built the object through a **real parameterized constructor**; model that
and cl reproduces retail's register allocation.

```cpp
// NO - spelled-out stores after new, OR a static-inline helper (cl won't inline it):
CImage* nf = new CImage;
nf->m_status = index; nf->m_08 = 0; nf->m_parent = parent; /* ... */   // ~65%

// YES - a real ctor on the class (BODY assignments, not a member-init list: the
// 1997-era MSVC5 devs used body assignments; init-lists score ~0.1% differently and
// are not what they wrote).  Declaration-order = retail store order.
class CImage : public CWapObj {
    CImage(i32 index, CImageParent* parent) {
        m_status = index; m_08 = 0; m_parent = parent;
        m_width = 0; m_height = 0; m_surface = 0; m_owned = 0;
    }
};
CImage* nf = new CImage(index, parent);                                 // 99.4%
```
```asm
push 0x34                     ; sizeof(T)
call ??2@YAPAXI@Z             ; operator new
add  esp,4
test eax,eax
je   <null>
mov  [eax+0x4],edi            ; m_status = index
mov  [eax+0x8],ebp            ; m_08 = 0    (ebp pinned as the zero reg)
mov  [eax+0xc],ecx            ; m_parent = parent
mov  [eax],offset ??_7CImage  ; <-- retail schedules the vptr store 4th
mov  [eax+0x10],ebp           ; m_width = 0 ...
```
STEERABLE (the big win: regalloc). The real ctor recovers retail's register roles and lifts
the family ~65→99% (CImageSet::CreateFrame24/28/30 0x151fb0/152060/152110 65→99.4,
CSprite::InsertFrame 0x151f00 84→99.4, CDDrawWorkerMapSmall CreateWorker28/2C 0x165990/165a10
62→96.7, Factory_1658c0/165a90 →92–96).

**Vptr position — where the last ~1% lives, and how to get it (→ 100 EXACT):** the residual is
the vptr store POSITION. A ctor stamps the DERIVED vptr *between the base-ctor body and the
derived-ctor body*. So look at which fields retail sets BEFORE the single vptr store:

- **pre-vptr fields are in the BASE class** → give the BASE a real ctor and DELEGATE
  (`Derived(args) : Base(args) { m_derivedField = …; }`). cl emits: base seed, derived vptr,
  derived field — retail's exact order. **This reaches 100 EXACT** (CDDrawWorkerList
  CreateWorkerA/B28/B2C/B30 0x156fd0/1573e0/157330/157150, 55–61→**100**: the 9 pre-vptr fields
  are all CDDrawWorkerBase's, only m_78 is derived).
- **pre-vptr fields are the DERIVED class's own** → a single derived ctor sets them AFTER the
  derived vptr (cl won't sink the vptr past them), so vptr lands 1st not Nth → ~99, not 100
  (CImage: m_status/m_08/m_parent are CImage's own). Moving them to a base is a layout change
  (Fable) — take the 99%.

Base-ctor delegation via `: Base(args)` is REQUIRED syntax (not the member-init-list to avoid).
Safe to add a ctor only when you own every `new Class` site (grep first) and keep any existing
default ctor.
