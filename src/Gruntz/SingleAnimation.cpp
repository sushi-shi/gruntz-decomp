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
