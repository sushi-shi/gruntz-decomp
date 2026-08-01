#include <rva.h>
#include <string.h>
#include <Gruntz/GameInfo.h>

RVA(0x00118040, 0xb6)
i32 CGameInfo::SetNames(char* name, char* name2, i32 unused) {
    if (name == 0) {
        return 0;
    }
    i32 len = static_cast<i32>(strlen(name));
    if (len < 0) {
        goto fail;
    }
    if (len > 16) {
        return 0;
    }
    if (name2 != 0) {
        i32 len2 = static_cast<i32>(strlen(name));
        if (len2 < 0) {
            goto fail;
        }
        if (len2 > 64) {
            goto fail;
        }
    }
    memset(&m_04, 0, 212);
    strcpy(m_14, name);
    if (name2 != 0) {
        strcpy(m_36, name2);
    }
    m_8 = 1;
    return 1;
fail:
    return 0;
}

RVA(0x00118130, 0x44)
i32 CGameInfo::CopyBody(char* body) {
    if (body != 0) {
        i32 len = static_cast<i32>(strlen(body + 0x10));
        if (len > 0 && len < 16) {
            memcpy(&m_04, body, 212);
            Check1();
            return 1;
        }
    }
    return 0;
}

RVA(0x001181d0, 0x70)
i32 CGameInfo::Update(i32 s, i32 timestamp, i32 type) {
    if (s == 0) {
        return 0;
    }
    if (timestamp == 0) {
        return 0;
    }
    CGameInfoTime* b = &m_b8;
    if (b == 0) {
        return 0;
    }
    if (b->m_4 > s) {
        return 0;
    }
    if (b->m_4 == s && b->m_8 < timestamp) {
        return 0;
    }
    b->m_4 = s;
    b->m_8 = timestamp;
    BuildGameDate(b);
    m_d4 = type;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00118260, 0x63)
i32 CGameInfo::CopyIfLarger(CGameInfoTime* src, i32 type) {
    if (src == 0) {
        return 0;
    }
    CGameInfoTime* dst = &m_b8;
    if (dst == 0) {
        return 0;
    }
    if (dst->m_4 > src->m_4) {
        return 0;
    }
    if (dst->m_4 == src->m_4 && dst->m_8 < src->m_8) {
        return 0;
    }
    *dst = *src;
    m_d4 = type;
    return 1;
}
