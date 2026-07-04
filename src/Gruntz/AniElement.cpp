#include <rva.h>
// CAniElement.cpp - the 0x28-byte 'ANI' animation element (primary vftable
// @0x5efba8) cataloged by CDDrawSubMgrAni (src/DDrawMgr/DDrawSubMgrAni.cpp). It
// owns a CObArray of 0x34-byte frame records (tomalla-39, vtable @0x5f02c0),
// a name buffer and a scale.
//
//   0x06b270  AtChecked  - bounds-checked record fetch from the CObArray.
//   0x165460  Build      - (re)parse the element from a CAniSource descriptor:
//               set flags/scale, copy the name, then for each record allocate +
//               parse a 0x34-byte frame; on any parse failure, scalar-delete every
//               record, free the name and empty the array. Returns 1 / 0.
//   0x1655c0  Configure  - gate on the source's 'ANI' (0x414e49) tag, then
//               BeginParse / Build / EndParse the entry.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------
#include <Gruntz/AniElement.h>
#include <Globals.h>

// Global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body so its
// rel32 call is reloc-masked.
void* operator new(u32 n);
// The buffer is freed via _RezFree (@0x1b9b82, __cdecl).
extern "C" void RezFree(void* p);

// Set by the record parser (0x168c60) to the parsed name length; the builder uses
// it to advance the record-stream cursor. 0x6bf3c4 -> file RVA 0x2bf3c4.

// The 'ANI' source entry's tag reader / parse session (CParseSource family,
// __thiscall on the entry node). Modeled as a layout-compatible view so the
// `mov ecx,entry; call` forms fall out; external/no-body.
class CAniEntry {
public:
    i32 ParseTag_139800();     // 0x139800  -> 0x414e49 for 'ANI'
    void* BeginParse_139960(); // 0x139960  returns the source descriptor or 0
    void EndParse_1399d0();    // 0x1399d0
};

// The 0x34-byte frame record Build `new`s + catalogs is the shared CAniRecordView
// (include/Gruntz/AniRecordView.h): its ctor stamps the primary vptr (@0x5f02c0)
// and seeds m_owner/m_count/m_indices. (Formerly a CAniElement.cpp-local
// CAniRecordInit duplicate - folded into the one shared view.)

// ---------------------------------------------------------------------------
// 0x06b270: bounds-checked CObArray fetch. 1 stack arg (ret 4).
RVA(0x0006b270, 0x1b)
::CObject* CAniElement::AtChecked_06b270(i32 i) const {
    if (i >= 0 && i < m_records.m_nSize) {
        return m_records.m_pData[i];
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x165460: (re)build the element from a parsed-source descriptor. __thiscall,
// 3 stack args (ret 0xc). Returns 1 on success, 0 on any record-parse failure.
// @early-stop
// 89.34% - whole body byte-correct in shape (offsets, calls, control flow, the
// for-loop success-first/`jl` exit order per docs/patterns/loop-preheader-vs-exit-
// block-order.md). Residual is three regalloc/scheduling walls: (1) retail rebases
// the descriptor's m_08 read onto the cursor (`mov ecx,[ebp-0x18]` where ebp=src+0x20)
// vs our `mov ecx,[ebx+8]`; (2) retail re-zeros the record reg at the loop top
// (`xor edi,edi` + a `jmp` skipping it the first iteration - zero-register-pinning.md);
// (3) the SetAtGrow arg lands in edx (retail) vs ecx (ours) with the array-`this`
// `lea` hoisted before the pushes (pin-local-for-callee-saved-reg.md). None steerable.
RVA(0x00165460, 0x156)
i32 CAniElement::Build_165460(void* ctx, CAniSource* src, i32 flags) {
    m_flags = flags;
    m_scale = 1.0f;
    m_total = 0;
    m_flags = src->m_flags | flags;

    const char* cursor = src->m_data;
    if (src->m_namelen != 0) {
        m_name = (char*)operator new(src->m_namelen + 2);
        i32 n;
        for (n = 0; n < src->m_namelen; n++) {
            m_name[n] = *cursor++;
        }
        m_name[n] = 0;
    } else {
        m_name = 0;
    }

    CAniRecordView* rec = 0;
    i32 i;
    for (i = 0; i < src->m_count; i++) {
        rec = new CAniRecordView;
        if (rec->Parse_168c60(ctx, cursor) == 0) {
            goto fail;
        }
        m_records.SetAtGrow(m_records.m_nSize, (::CObject*)rec);
        cursor += g_aniParsedNameLen + 0x14;
        m_total += rec->GetSize_168e50();
    }
    return 1;

fail:
    if (rec != 0) {
        rec->ScalarDtor(1);
    }
    for (i = 0; i < m_records.m_nSize; i++) {
        ::CObject* p = m_records.m_pData[i];
        if (p != 0) {
            ((CAniRecordView*)p)->ScalarDtor(1);
        }
    }
    if (m_name != 0) {
        RezFree(m_name);
        m_name = 0;
    }
    m_records.SetSize(0, -1);
    return 0;
}

// ---------------------------------------------------------------------------
// 0x1655c0: gate on the 'ANI' tag, then BeginParse / Build / EndParse the entry.
// __thiscall, 3 stack args (ret 0xc). Returns Build's result (0 if not 'ANI').
RVA(0x001655c0, 0x53)
i32 CAniElement::Configure_1655c0(void* ctx, void* entry, i32 flags) {
    if (((CAniEntry*)entry)->ParseTag_139800() != 0x414e49) {
        return 0;
    }
    m_flags = flags;
    void* src = ((CAniEntry*)entry)->BeginParse_139960();
    if (src == 0) {
        return 0;
    }
    i32 r = Build_165460(ctx, (CAniSource*)src, 0);
    ((CAniEntry*)entry)->EndParse_1399d0();
    return r;
}

SIZE_UNKNOWN(CAniElement);
SIZE_UNKNOWN(CAniEntry);
SIZE_UNKNOWN(CAniRecordArray);
SIZE_UNKNOWN(CAniRecordView);
SIZE_UNKNOWN(CAniSource);
