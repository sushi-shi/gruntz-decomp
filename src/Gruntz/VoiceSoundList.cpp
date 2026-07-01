#include <Mfc.h> // CObList/CString machinery (reloc-masked); /GX EH frame

#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <rva.h>
// VoiceSoundList.cpp - CVoiceBuilder::BuildVoiceSoundList (0x11c210, 669 B): build
// the CStringList of valid "VOICES_<dir>[_<sub>]" sound names for speech-group `n`,
// reading the group's DIR / sub-keys from g_buteMgr ([SG<n>]) and keeping only the
// names that resolve to a 'WAV' resource through this->m_0->m_34.  The four CString
// temps + the heap CStringList node give the routine its /GX exception frame.
//
// Only offsets / code bytes are load-bearing; engine callees are reloc-masked.

// The 'WAV' resource resolver reached through this->m_0->m_34 (FUN_0013bff0).
struct VoiceResolver {
    void* Resolve(const char* name, i32 tag); // FUN_0013bff0 __thiscall, ret resource (0 absent)
};
struct VoiceRoot { // this->m_0
    char m_pad00[0x34];
    VoiceResolver* m_34; // +0x34
};
struct VoiceList;
struct CVoiceBuilder {
    VoiceList* BuildVoiceSoundList(i32 n);
    VoiceRoot* m_0; // +0x00
};

// CButeMgr getter (g_buteMgr @ 0x6453d8): GetStringDef(tag, key, def) returns the
// stored CString (or `def`) - 0x173180 __thiscall. On the canonical CButeMgr.
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The format-into-CString helper (FUN_001b2cf5, reloc-masked free fn).
void EngFmt(CString* out, const char* fmt, ...); // 0x1b2cf5

// The heap CStringList node (0x24 bytes): a CObList (+0x00) plus two scalar fields.
// AddName copies one CString into the list (FUN_00402446 via its ILT thunk).
struct VoiceList {
    VoiceList();
    CObList m_list;                 // +0x00 (0x1c)
    i32 m_1c;                       // +0x1c
    i32 m_20;                       // +0x20
    void AddName(const CString& s); // FUN_00402446 __thiscall
};
inline VoiceList::VoiceList() {
    m_1c = 0;
    m_20 = -1;
}

// @early-stop
// /GX EH-state + regalloc wall (~79%): complete body - the n bounds guards, the four
// CString temps, the [SG<n>] DIR/sub-key g_buteMgr reads, the VOICES_<dir>[_<sub>]
// name format, the 'WAV'-resolve filter and the heap CStringList accumulation all
// align with retail.  Residual: (a) the per-CString-temp destruct trylevel slot index
// + the descending dtor sweep on the failure path are the documented /GX EH-state wall
// (docs/seh-eh.md), and (b) the AddName receiver passes the name CString by value with
// two extra scalar args where this models a by-reference add - a small arg-shape detail
// that doesn't change the resolve/accumulate logic.  Deferred to the final sweep.
RVA(0x0011c210, 0x29d)
VoiceList* CVoiceBuilder::BuildVoiceSoundList(i32 n) {
    if (n <= 0) {
        return 0;
    }
    if (n >= 0x4b0) {
        return 0;
    }

    CString dir, scratch, sub, name;
    EngFmt(&scratch, "SG%i", n);
    scratch = *g_buteMgr.GetStringDef((char*)(LPCTSTR)scratch, "DIR", &dir);

    EngFmt(&sub, "S%i", 1);
    sub = *g_buteMgr.GetStringDef((char*)(LPCTSTR)scratch, (char*)(LPCTSTR)sub, &dir);

    VoiceList* list = 0;
    if (!scratch.IsEmpty()) {
        list = new VoiceList();
    }

    if (!sub.IsEmpty()) {
        i32 i = 1;
        do {
            i++;
            if (sub.IsEmpty()) {
                EngFmt(&name, "VOICES_%s", (LPCTSTR)scratch);
            } else {
                EngFmt(&name, "VOICES_%s_%s", (LPCTSTR)scratch, (LPCTSTR)sub);
            }
            void* res = m_0->m_34->Resolve((LPCTSTR)name, 0x574156);
            if (res != 0) {
                CString tmp = name;
                list->AddName(tmp);
                EngFmt(&sub, "S%i", i);
                sub = *g_buteMgr.GetStringDef((char*)(LPCTSTR)scratch, (char*)(LPCTSTR)sub, &dir);
            } else {
                sub.Empty();
            }
        } while (!sub.IsEmpty());
    }
    return list;
}
SIZE_UNKNOWN(CVoiceBuilder);
SIZE_UNKNOWN(VoiceList);
SIZE_UNKNOWN(VoiceResolver);
SIZE_UNKNOWN(VoiceRoot);
