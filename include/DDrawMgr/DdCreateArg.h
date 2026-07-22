// DdCreateArg.h - the CDirectDrawMgr::CreatePoolItem factory-arg views.
//
// @identity-TODO (honest by-offset + abstract-COM models, no fabricated identity):
// CreatePoolItem's arg0 is a deeply-nested descriptor whose +0x08 is a COM-style
// interface; only its slot 12 (+0x30) Make(outB, outA) is invoked. No RTTI COL is on
// the descriptor or its interface, so the concrete classes are unrecovered until the
// CDDrawSurfacePair->+0xc surface-descriptor subsystem is RTTI-pinned. Used in
// src/DDrawMgr/DirectDrawMgr.cpp.
#ifndef DDRAWMGR_DDCREATEARG_H
#define DDRAWMGR_DDCREATEARG_H

#include <rva.h>
#include <Win32.h> // STDMETHOD / PURE / HRESULT (COM-style interface)

struct CDdDescSrc {
    STDMETHOD(v00)() PURE;
    STDMETHOD(v01)() PURE;
    STDMETHOD(v02)() PURE;
    STDMETHOD(v03)() PURE;
    STDMETHOD(v04)() PURE;
    STDMETHOD(v05)() PURE;
    STDMETHOD(v06)() PURE;
    STDMETHOD(v07)() PURE;
    STDMETHOD(v08)() PURE;
    STDMETHOD(v09)() PURE;
    STDMETHOD(v0a)() PURE;
    STDMETHOD(v0b)() PURE;
    STDMETHOD(Make)(void* outB, void* outA) PURE; // slot 12 (+0x30)
};
SIZE_UNKNOWN();

struct CDdCreateArg {
    char m_pad00[8];
    CDdDescSrc* m_8; // +0x08 descriptor source
};
SIZE_UNKNOWN();

#endif // DDRAWMGR_DDCREATEARG_H
