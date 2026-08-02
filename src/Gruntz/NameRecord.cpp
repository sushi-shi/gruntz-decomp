#include <rva.h>

#include <Gruntz/GameInfo.h>

#include <string.h>

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
    memset(&m_body, 0, sizeof(m_body));
    strcpy(m_body.m_name, name);
    if (name2 != 0) {
        strcpy(m_body.m_location, name2);
    }
    m_body.m_version = 1;
    return 1;
fail:
    return 0;
}

RVA(0x00118130, 0x44)
i32 CGameInfo::CopyBody(char* body) {
    if (body != 0) {
        i32 len = static_cast<i32>(strlen(body + 0x10));
        if (len > 0 && len < 16) {
            memcpy(&m_body, body, sizeof(m_body));
            HasSupportedVersion();
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
    CGameInfoTime* b = &m_body.m_time;
    if (b == 0) {
        return 0;
    }
    if (b->m_score > s) {
        return 0;
    }
    if (b->m_score == s && b->m_timeMs < timestamp) {
        return 0;
    }
    b->m_score = s;
    b->m_timeMs = timestamp;
    BuildGameDate(b);
    m_body.m_type = type;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00118260, 0x63)
i32 CGameInfo::CopyIfLarger(CGameInfoTime* src, i32 type) {
    if (src == 0) {
        return 0;
    }
    CGameInfoTime* dst = &m_body.m_time;
    if (dst == 0) {
        return 0;
    }
    if (dst->m_score > src->m_score) {
        return 0;
    }
    if (dst->m_score == src->m_score && dst->m_timeMs < src->m_timeMs) {
        return 0;
    }
    *dst = *src;
    m_body.m_type = type;
    return 1;
}
