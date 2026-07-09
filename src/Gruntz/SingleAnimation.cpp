// SingleAnimation.cpp - a single-shot eyecandy animation game-object
// (C:\Proj\Gruntz).
//
// One trace-discovered CSingleAnimation method:
//   ~CSingleAnimation @0x010540 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CSingleAnimation : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// CSingleAnimation::Serialize @0x104a0 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, then (only on success) the +0x34 sub-object's
// chain. Returns the second chain's success normalized to a bool.
RVA(0x000104a0, 0x47)
i32 CSingleAnimation::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CSingleAnimation::~CSingleAnimation @0x010540 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x00010540, 0x44)
CSingleAnimation::~CSingleAnimation() {}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
