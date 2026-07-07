// Object.h - the WAP grand-base is MFC's ::CObject (RTTI name "CObject", vtable
// RVA 0x1e8cb4 / VA 0x5e8cb4, COL type-descriptor `CObject`).
//
// The game statically links MFC, so the retail "CObject" @0x1e8cb4 IS MFC's
// CObject. The reconstruction used to model that one class TWICE - MFC's ::CObject
// in MFC-including TUs, and a lean 5-slot `Wap::CObject` stand-in in the pure-Win32
// TUs that avoided the heavy <Mfc.h> umbrella. They were the same class, so the
// duality is now UNIFIED: there is a single global `CObject` (MFC's), every engine
// class derives it directly (`: public CObject`), and every TU that needs the
// grand-base brings <Mfc.h> - which is all this header now does.
//
// COST (accepted - clean-room mandate targets MAX %, not current %): the ~51
// formerly-lean classes had the inline-empty Wap::~CObject folded into their
// derived dtors (an inlined grand-base vptr re-stamp); they now inherit MFC's
// out-of-line ~CObject (NAFXCW), so the folded teardown emits `call ??1CObject`
// where it used to inline the stamp - a measured % drop, recoverable later (a PCH
// that brings MFC uniformly may help). This also dissolves every `(::CObject*)`
// cast that existed only because the two were distinct source types.
#ifndef WAP32_COBJECT_H
#define WAP32_COBJECT_H

#include <Mfc.h> // the single ::CObject (MFC, statically linked; RTTI @0x1e8cb4)

#endif // WAP32_COBJECT_H
