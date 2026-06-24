# /GX dtor: an embedded sub-object's trailing vptr re-stamp is its MEMBER destructor
tags: cpp:dtor | cpp:eh | cpp:member | asm:mov | topic:codegen-idiom topic:eh
symptoms: ??1 ends with `mov [this+N],<base-vtbl>` AFTER the last member RemoveAll/teardown; frameless recompile or one trylevel short; manual-vtable (non-polymorphic) class with an embedded sub-object at +N
confidence: 8/10
variants: eh-dtor-model-members-as-destructible.md, eh-dtor-needs-base-subobject.md

A manual-vtable class (modeled with `m_vtbl = &Retail_vftable` stamps, not `virtual`)
that embeds a polymorphic sub-object at +N: the retail `~Class` ends with
`mov [this+N],&base_subobject_vtbl` (the sub-object's vptr restored to its abstract
base) emitted AFTER the last real member teardown. That trailing store is the
**sub-object's member destructor running in reverse declaration order** — not body
code. Writing it as the last statement of `~Class` puts it in the wrong EH state
(before the member-teardown trylevel) and leaves the dtor one trylevel short. Give
the sub-object type a real destructor whose ONLY job is the vptr restore; declare it
as a value member; let the compiler schedule it.

```cpp
struct CObjList {                 // the embedded sub-object at +0x10
    void* m_vtbl;                 // its own vtable ptr
    /* ... */
    ~CObjList() { m_vtbl = &CObjList_purecall_vftbl; }  // restore to abstract base
};
class CSymParser {
    CObjList m_list;              // +0x10
    CParserHash m_hash;           // +0x80 (also a destructible member: ~ => RemoveAll)
    ~CSymParser();                // body does the manual teardown; members auto-destruct
};
// ~CSymParser body ends WITHOUT the RemoveAll / vptr re-stamp — those are the
// m_hash and m_list member destructors, run in reverse decl order under the frame.
```
```asm
; ... +0x88 manual loop ...
lea  ecx,[esi+0x80]
mov  [esp+0x1c],0          ; trylevel 0 (m_hash teardown)
call RemoveAll             ; m_hash member dtor
mov  [esi+0x10],<0x5ef760> ; m_list member dtor: vptr restore  <-- the trailing store
; epilogue (fs:0 restore)
```
Steerable: recovered ~CSymParser 79.7%→98.0% (the residual is the EH state-index /
vptr-restamp-position wall + reloc naming). The companion destructible member
(CParserHash : CHashBase with `~ => RemoveAll`) is a thin local subclass so the
shared CHashBase value member in sibling TUs stays trivially-destructible (a bare
`~CHashBase` there cost CHashBase::Lookup 100%→99% via a SIB-byte regalloc shift).
