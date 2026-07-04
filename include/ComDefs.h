// ComDefs.h - the dev-authentic COM/DirectX SDK interface macros (STDMETHOD /
// STDMETHOD_ / PURE / HRESULT / REFIID), a minimal mirror of <basetyps.h> +
// <wtypes.h>. The DirectX-manager modules declared their COM interfaces the real
// SDK way (`STDMETHOD_(ULONG, AddRef)() PURE;` etc.); this surfaces those macros
// for our hand-modeled DirectDraw/DirectSound/DirectInput interface views.
//
// Why a local mirror rather than the real header: the STDMETHOD family lives in the
// standalone <basetyps.h>, but HRESULT/REFIID/IID/GUID live in <wtypes.h>, reachable
// only through <objbase.h> - which drags <rpc.h>, <rpcndr.h>, <unknwn.h>, <objidl.h>
// and the full <windows.h> into every includer (verified: even <windows.h> under
// WIN32_LEAN_AND_MEAN does NOT expose HRESULT/REFIID). Most COM headers here are
// pulled into TUs kept deliberately lean - no <windows.h> at all, e.g.
// SoundDevice.cpp - mirroring the engine's own self-contained interface views, so
// pulling that whole OLE/RPC/windows chain in would be a broad, non-neutral
// footprint. As the codebase already does to bound scope (see <Win32.h>, <Ints.h>),
// this mirrors just the handful of definitions those declarations need and stands
// alone. Include it ONLY from the COM-interface headers that need it, and keep those
// headers lean: folding it into a widely-included header (e.g. <Win32.h>) adds these
// declarations to already-matched TUs and perturbs MSVC5 regalloc off a cliff
// (measured: -1.2% on CLightFxRender) - so it is deliberately NOT pulled in broadly.
//
// Every definition is a faithful, ABI-identical copy of its SDK original and carries
// the SDK's OWN guard macro (verified against VC5 <wtypes.h> / <basetyps.h>), so it
// coexists with a real <windows.h>/<objbase.h> later in the same TU: whoever comes
// first wins, the other's guard skips. All matching-neutral: STDMETHODCALLTYPE ==
// __stdcall, HRESULT == long (4B), REFIID == const IID& == a 4B const-pointer (the
// refcount return uses <Ints.h>'s u32 rather than ULONG - see the note below).
#ifndef GRUNTZ_COMDEFS_H
#define GRUNTZ_COMDEFS_H

#include <Ints.h>

#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE __stdcall
#endif

// HRESULT - the COM status code (signed long, 4 bytes). Guard mirrors <wtypes.h>.
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef long HRESULT;
#endif

// NB: the IUnknown AddRef/Release refcount return is spelled `u32` (from <Ints.h>)
// rather than the SDK's ULONG. Both are unsigned long / 4 bytes (ABI-identical), but
// <winnt.h> typedefs ULONG *unguarded*, so a second ULONG typedef here would collide
// with a real <windows.h> in the same TU and - being an extra declaration - perturbs
// MSVC5 regalloc in unrelated matched functions (measured: -1.2% on CLightFxRender).
// u32 is the codebase's own alias (the Rust-style ints convention), always present,
// dup-free, matching-neutral - so it is what these COM slots use here.

// GUID / IID / REFIID - the COM interface-id types (<wtypes.h> / <guiddef.h>). All
// guarded (or windows.h-LEAN-absent), so no dup vs a real <windows.h> in the same TU.
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif
#ifndef __IID_DEFINED__
#define __IID_DEFINED__
typedef GUID IID;
#endif
#ifndef REFIID
#define REFIID const IID&
#endif

// STDMETHOD family (<basetyps.h>, C++ mode): a __stdcall virtual COM slot.
#ifndef STDMETHOD
#define STDMETHOD(method) virtual HRESULT STDMETHODCALLTYPE method
#endif
#ifndef STDMETHOD_
#define STDMETHOD_(type, method) virtual type STDMETHODCALLTYPE method
#endif
#ifndef PURE
#define PURE = 0
#endif

#endif // GRUNTZ_COMDEFS_H
