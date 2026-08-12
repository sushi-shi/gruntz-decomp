#ifndef GRUNTZ_GRUNTZ_CFADER_H
#define GRUNTZ_GRUNTZ_CFADER_H

#include <Ints.h>

class CDDSurface;
#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h>

class CFader {
public:
    CFader();
    virtual ~CFader();

    virtual void RenderFrame(i32 f) = 0;
    virtual i32 GetFrameCount() = 0;
    virtual void BeginFade();
    virtual void EndFade();

    void Wait(i32 delay);
    void SetTimers(CDDSurface* src, CDDSurface* dst);
    void Set2c(class CDDrawPtrCollections* pool);

    void RunFadeStepped(i32 step, i32 lead, i32 vsync);

    void RunFade(u32 dur, i32 lead, i32 vsync);

    CShadeTableCache m_cache;
    CShadeTable* m_table;
    i32 m_previousFrame;
    CDDSurface* m_timerA;
    CDDSurface* m_timerB;

    class CDDrawPtrCollections* m_ptrColl;
    i32 m_flag;
    i32 m_measuredFps;
};

extern const float g_faderScale_5f085c;
extern const double g_faderPowK;
extern const float g_faderHalf;
extern const double g_faderScale;
extern const double g_faderBiasR;
extern const float g_faderBiasFade;
extern const float g_faderOne;
extern const float g_faderHalfPi;
extern const float g_sineHalfPi;
extern const float g_sineOne;
#include <io.h>

extern const float g_fxBias;
extern const float g_fxEps;

void __cdecl operator delete(void* p);
void ScatterSamples(i32* arr, i32, i32, i32);

#endif // GRUNTZ_GRUNTZ_CFADER_H
