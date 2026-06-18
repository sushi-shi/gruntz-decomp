// UnknownPettigrew.cpp - VirtualMethodUnknown14 of the tomalla-named class
// UnknownPettigrew (a CDirectDrawMgr surface/page sub-manager in the "Harry
// Potter" family). A standard readiness predicate shared by several Lucius-
// derived managers: reports ready (1) when the parent/root handle at +0x0c
// is present and the base status word at +0x04 is no longer the inactive -1
// sentinel. Plain /O2 /MT leaf: NO SEH frame, NO relocations - touches only
// those two member offsets.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted
// code bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

class UnknownMinerva {
public:
    void ClearUnknownMap();
};

class UnknownPettigrew {
public:
    int VirtualMethodUnknown14();
    void VirtualMethodUnknown18();

    void *m_vptr;               // +0x00
    int   m_04;                 // +0x04  -1 when inactive
    char  m_pad08[0x0c-0x08];   // +0x08..0x0b
    int   m_0c;                 // +0x0c  parent/root handle
};

// ---------------------------------------------------------------------------
// UnknownPettigrew::VirtualMethodUnknown14  @0x1577a0  (__thiscall, ret 0)
// Ready when the parent handle is present and the status word is not -1.
// ---------------------------------------------------------------------------
// @address: 0x1577a0
// @size:    0x16
int UnknownPettigrew::VirtualMethodUnknown14()
{
    if (m_0c == 0)
        goto fail;
    if (m_04 != -1)
        return 1;

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownPettigrew::VirtualMethodUnknown18  @0x157ae0  (__thiscall, ret 0)
// Clears the parent map then zeroes a member field.
// ---------------------------------------------------------------------------
// @address: 0x157ae0
// @size:    0x11
void UnknownPettigrew::VirtualMethodUnknown18()
{
    ((class UnknownMinerva *)this)->ClearUnknownMap();
    m_0c = 0;
}
