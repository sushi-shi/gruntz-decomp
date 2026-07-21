#include <Gruntz/State.h>
#include <rva.h>

void operator delete(void*);

RVA(0x0008c750, 0xa9)
CState::CState() {
    m_mgr = 0;
    m_symParser = 0;
    m_c = 0;
    m_levelBank = 0;
    m_2c = 0;
    m_blitSurface0 = 0;
    m_blitSurface1 = 0;
    m_38 = 0;
    m_ready = 0;
    m_versionString[0] = 0; // the +0x4c byte store (the buffer's lead NUL)
    m_24 = 0;
    m_scratchSurface0 = 0;
    m_scratchSurface1 = 0;
    m_168 = 0;
    m_170 = 0x40;
    m_16c = 0;
    m_174 = 0x40;
    m_178 = 0;
    m_180 = 0x40;
    m_17c = 0;
    m_184 = 0x40;
    m_188 = 0;
    m_190 = 0;
    m_18c = 0;
    m_194 = 0;
    m_198 = 0;
    m_1a0 = 0;
    m_19c = 0;
    m_1a4 = 0;
    m_cursorX = 0;
    m_cursorY = 0;
}

// CState::~CState() - the slot-0 scalar-deleting dtor `??_G` (0x8c710). Its body is
// defined INLINE in <Gruntz/State.h> so MSVC folds it into the synth `??_G` thunk; the
// thunk has no source body, so pin its symbol by mangled name here.
// @rva-symbol: ??_GCState@@UAEPAXI@Z 0x0008c710 0x24
