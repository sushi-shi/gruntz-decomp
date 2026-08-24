# A cached `const char*` hoists a CString conversion the retail loop repeats
tags: cpp:local cpp:loop cpp:string | asm:mov | topic:codegen-idiom
symptoms: `walls loopscan` says our loop body is fatter/thinner by exactly the number of uses; retail re-reads ONE stack slot before each call while we push the same register; `mov ecx,DWORD PTR [esp+0x1c]` repeated
confidence: 8/10

MFC's `operator LPCTSTR` is a header inline that returns `m_pchData`, so passing
a `CString` to a `const char*` parameter emits a FRESH load at every call site.
Introducing a `const char* name = str;` local collapses all of them into one -
which is what a reconstruction naturally writes, and what retail did not.

```cpp
// ours: one load, kept in a callee-saved register for the whole body
const char* groupName = static_cast<const char*>(group);
if (!bute.Exists(groupName, NULL)) { ... }
code = *bute.GetStringDef(groupName, "Text", &code);

// retail: the CString goes to each call, so each call re-reads m_pchData
if (!bute.Exists(group, NULL)) { ... }
code = *bute.GetStringDef(group, "Text", &code);
```
```asm
; ours - hoisted
mov  esi,DWORD PTR [esp+0x38]
push esi
...
push esi
; retail - one load per use, all from the same slot
mov  eax,DWORD PTR [esp+0x28]
push eax
...
mov  ecx,DWORD PTR [esp+0x1c]
push ecx
```
Steerable: delete the cache and pass the `CString`. The delta is exactly one
`mov` per use site, so `walls loopscan`'s `retail+movxN` names N = the number of
uses before you read any source. `CChatBoxOwner::HandleTextInputKey` 0x205c0
85.18 -> 85.32 with five uses (`retail+movx6`: five conversions plus the
receiver hoist described in the row). The reverse reading is the useful one:
`ours+movxN` inside a loop means WE re-read something retail cached in a local.
