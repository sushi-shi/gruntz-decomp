#ifndef GRUNTZ_IO_MOVIEPLAYERINLINE_H
#define GRUNTZ_IO_MOVIEPLAYERINLINE_H

#include <Io/MoviePlayer.h>

#include <smack.h>

#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64

inline void CMoviePlayer::DecodeFrame() {
    i32 hr = m_srcSurf->Lock(NULL, &m_srcDesc, 1, NULL);
    while (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (m_srcSurf->Restore() != 0) {
            return;
        }
        hr = m_srcSurf->Lock(NULL, &m_srcDesc, 1, NULL);
    }
    if (hr == 0) {
        SmackToBuffer(
            m_smackHandle,
            0,
            0,
            m_srcDesc.lPitch,
            m_smackHandle->Height,
            m_srcDesc.lpSurface,
            m_smackBufMode
        );
        SmackDoFrame(m_smackHandle);
        m_frameDecoded = true;
        m_srcSurf->Unlock(m_srcDesc.lpSurface);
    }
}

#endif // GRUNTZ_IO_MOVIEPLAYERINLINE_H
