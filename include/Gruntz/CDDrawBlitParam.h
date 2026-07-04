#pragma once
// CDDrawBlitParam - the 0x3c-byte blit/command parameter sub-object held at
// CDDrawBlitWorker/CWwdGameObject +0x1a0. Its dispatch/serialize methods live in
// CDDrawSubMgr.cpp; CWwdGameObject embeds it (the former per-TU `CmdMap` view).
// Single-source shared declaration - only offsets + method symbols are load-bearing.
#include <Ints.h>

class CDDrawBlitParamSrc;
struct CSerialArchive; // the shared serialize stream (Read @+0x2c / Write @+0x30)
class CDDrawBlitWorker;

class CDDrawBlitParam {
public:
    // Construct's argument is a generic owner/source object with a +0x0c
    // "elements" pointer; it arrives as a CDDrawBlitParamSrc from the worker path
    // and as the owning CWwdGameObject from the game-object path, so it is modeled
    // as void* (reinterpreted internally) to keep one mangled symbol across both.
    void Construct(void* src);
    void Reset_15c2c0();
    void Setup_15c2d0(CDDrawBlitParamSrc* src);
    void Recompute_15c320(i32 a1);
    i32 SelectCue_157a80(void* force);
    i32 Serialize_15c970(CSerialArchive* ar);
    i32 Deserialize_15ca70(CSerialArchive* ar);
    i32 Find(CSerialArchive* ar, i32 type, i32 a3, i32 a4);

    char m_pad00[0x0c];           // +0x00 .. +0x0b
    CDDrawBlitWorker* m_worker;   // +0x0c
    i32 m_10;                     // +0x10
    CDDrawBlitParamSrc* m_srcRef; // +0x14  the resolved source (map value)
    char* m_element;              // +0x18  current element (transient; not serialized)
    i32 m_index;                  // +0x1c
    i32 m_20;                     // +0x20
    i32 m_24;                     // +0x24
    i32 m_28;                     // +0x28
    i32 m_2c;                     // +0x2c
    i32 m_30;                     // +0x30
    i32 m_34;                     // +0x34
    float m_scale;                // +0x38
};
