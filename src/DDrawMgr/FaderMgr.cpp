#include <Gruntz/FaderMgr.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/FxModeT1.h>

#include <rva.h>
#include <Mfc.h>
#include <string.h>

VTBL(CFaderArray, 0x001f0790);

RVA(0x0017d8f0, 0x1e)
CFaderMgr::CFaderMgr() {
    m_active = 0;
    m_0c = 0;
}

RVA(0x0017d910, 0x65)
CFaderMgr::~CFaderMgr() {
    FreeAll();
}

RVA(0x0017d980, 0x1f)
i32 CFaderMgr::SetConfig(CDDSurface* a, CDDSurface* b, CDDrawPtrCollections* pool) {
    m_timerArgA = a;
    m_timerArgB = b;
    m_sharedPtrColl = pool;
    m_active = 1;
    return 1;
}

RVA(0x0017d9a0, 0x11)
void CFaderMgr::FreeAll() {
    DeleteAll();
    m_active = 0;
}

RVA(0x0017d9c0, 0x7a0)
CFader* CFaderMgr::Add(i32 nFaderType, CFxModeDesc* pInit) {
    CFader* fader = 0;

    switch (nFaderType) {
        case 0: {
            if (pInit != 0 && pInit->m_type != 1) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderShape* f = new CFaderShape;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT1 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 1: {
            if (pInit != 0 && pInit->m_type != 2) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderLight* f = new CFaderLight;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT2 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 2: {
            if (pInit != 0 && pInit->m_type != 3) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderSine* f = new CFaderSine;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT3 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 3: {
            if (pInit != 0 && pInit->m_type != 4) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderRadial* f = new CFaderRadial;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT4 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 4: {
            if (pInit != 0 && pInit->m_type != 5) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderFlat* f = new CFaderFlat;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT5 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        case 5: {
            if (pInit != 0 && pInit->m_type != 6) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return 0;
            }
            CFaderMesh* f = new CFaderMesh;
            fader = f;
            f->SetTimers(m_timerArgA, m_timerArgB);
            f->Set2c(m_sharedPtrColl);
            if (pInit == 0) {
                CFxModeT6 init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return 0;
                }
            }
            break;
        }
        default:
            Trace("CFaderMgr::Add (...) - nFaderType is invalid");
            break;
    }

    if (fader != 0) {
        m_arr.Add(fader);
    }
    return fader;
}

RVA_COMPGEN(0x0017e160, 0x8, ??1CFxModeT1@@QAE@XZ)

// @early-stop
RVA(0x0017e170, 0x5b)
void CFaderMgr::Remove(CFader* pFader) {
    i32 i = 0;
    i32 last = m_arr.m_nSize - 1;
    if (last >= 0) {
        CFader** w = m_arr.m_pData;
        while (*w != pFader) {
            i++;
            w++;
            if (i > last) {
                return;
            }
        }
        i32 cnt = m_arr.m_nSize - i - 1;
        CFader** dst = &m_arr.m_pData[i];
        if (cnt) {
            memcpy(dst, dst + 1, cnt * sizeof(CFader*));
        }
        m_arr.m_nSize--;
        delete pFader;
    }
}

RVA(0x0017e1d0, 0x4d)
void CFaderMgr::DeleteAll() {
    i32 i = 0;
    i32 last = m_arr.m_nSize - 1;
    if (last >= 0) {
        do {
            CFader* p = m_arr.m_pData[i];
            delete p;
            i++;
            last = m_arr.m_nSize - 1;
        } while (i <= last);
    }
    if (m_arr.m_pData) {
        operator delete(m_arr.m_pData);
        m_arr.m_pData = 0;
    }
    m_arr.m_nMaxSize = 0;
    m_arr.m_nSize = 0;
}

RVA(0x0017e230, 0xc)
void CFaderMgr::Trace(CString s) {
    static_cast<void>(s);
}

RVA_COMPGEN(0x0017e240, 0x51, ??1CFaderArray@@UAE@XZ)

// @early-stop
RVA(0x0017e2a0, 0x188)
RVA_COMPGEN(0x0017e430, 0x1e, ??_GCFaderArray@@UAEPAXI@Z)
void CFaderArray::Serialize(CArchive& ar) {
    if (ar.IsStoring()) {
        ar.WriteCount(m_nSize);
    } else {
        i32 n = ar.ReadCount();
        if (n == 0) {
            if (m_pData != 0) {
                ::operator delete(m_pData);
                m_pData = 0;
            }
            m_nMaxSize = 0;
            m_nSize = 0;
        } else if (m_pData == 0) {
            m_pData = static_cast<CFader**>(::operator new(n * 4));
            memset(m_pData, 0, n * 4);
            m_nMaxSize = n;
            m_nSize = n;
        } else if (n <= m_nMaxSize) {
            if (n > m_nSize) {
                memset(m_pData + m_nSize, 0, (n - m_nSize) * 4);
            }
            m_nSize = n;
        } else {
            i32 grow = m_nGrowBy;
            if (grow == 0) {
                grow = m_nSize / 8;
                if (grow < 4) {
                    grow = 4;
                } else if (grow > 0x400) {
                    grow = 0x400;
                }
            }
            i32 newMax = m_nMaxSize + grow;
            if (n >= newMax) {
                newMax = n;
            }
            CFader** nd = static_cast<CFader**>(::operator new(newMax * 4));
            memcpy(nd, m_pData, m_nSize * 4);
            memset(nd + m_nSize, 0, (n - m_nSize) * 4);
            ::operator delete(m_pData);
            m_pData = nd;
            m_nSize = n;
            m_nMaxSize = newMax;
        }
    }
    if (ar.IsStoring()) {
        ar.Write(m_pData, m_nSize * 4);
    } else {
        ar.Read(m_pData, m_nSize * 4);
    }
}
