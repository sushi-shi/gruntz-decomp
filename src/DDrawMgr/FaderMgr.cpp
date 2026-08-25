#include <rva.h>

#include <Mfc.h>

#include <Gruntz/FaderConfigKind.h>
#include <Gruntz/FaderKind.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/ShapeFaderConfig.h>

#include <string.h>

RVA(0x0017d8f0, 0x1e)
CFaderMgr::CFaderMgr() {
    m_active = false;
    m_traceEnabled = 0;
}

RVA(0x0017d910, 0x65)
CFaderMgr::~CFaderMgr() {
    FreeAll();
}

RVA(0x0017d980, 0x1f)
i32 CFaderMgr::SetDefaults(
    CDDSurface* primary,
    CDDSurface* secondary,
    CDDrawDeviceManager* manager
) {
    m_primarySurface = primary;
    m_secondarySurface = secondary;
    m_deviceManager = manager;
    m_active = true;
    return 1;
}

RVA(0x0017d9a0, 0x11)
void CFaderMgr::FreeAll() {
    DeleteAll();
    m_active = false;
}

RVA(0x0017d9c0, 0x7a0)
CFader* CFaderMgr::Add(FaderKind nFaderType, CFaderConfig* pInit) {
    CFader* fader = NULL;

    switch (nFaderType) {
        case FADERKIND_SHAPE: {
            if (pInit != NULL && pInit->m_kind != FADER_CONFIG_SHAPE) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return NULL;
            }
            CFaderShape* f = new CFaderShape;
            fader = f;
            f->SetDefaultSurfaces(m_primarySurface, m_secondarySurface);
            f->SetDeviceManager(m_deviceManager);
            if (pInit == NULL) {
                CShapeFaderConfig init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            }
            break;
        }
        case FADERKIND_LIGHT: {
            if (pInit != NULL && pInit->m_kind != FADER_CONFIG_LIGHT) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return NULL;
            }
            CFaderLight* f = new CFaderLight;
            fader = f;
            f->SetDefaultSurfaces(m_primarySurface, m_secondarySurface);
            f->SetDeviceManager(m_deviceManager);
            if (pInit == NULL) {
                CLightFaderConfig init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            }
            break;
        }
        case FADERKIND_SINE: {
            if (pInit != NULL && pInit->m_kind != FADER_CONFIG_SINE) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return NULL;
            }
            CFaderSine* f = new CFaderSine;
            fader = f;
            f->SetDefaultSurfaces(m_primarySurface, m_secondarySurface);
            f->SetDeviceManager(m_deviceManager);
            if (pInit == NULL) {
                CSineFaderConfig init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            }
            break;
        }
        case FADERKIND_RADIAL: {
            if (pInit != NULL && pInit->m_kind != FADER_CONFIG_RADIAL) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return NULL;
            }
            CFaderRadial* f = new CFaderRadial;
            fader = f;
            f->SetDefaultSurfaces(m_primarySurface, m_secondarySurface);
            f->SetDeviceManager(m_deviceManager);
            if (pInit == NULL) {
                CRadialFaderConfig init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            }
            break;
        }
        case FADERKIND_FLAT: {
            if (pInit != NULL && pInit->m_kind != FADER_CONFIG_FLAT) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return NULL;
            }
            CFaderFlat* f = new CFaderFlat;
            fader = f;
            f->SetDefaultSurfaces(m_primarySurface, m_secondarySurface);
            f->SetDeviceManager(m_deviceManager);
            if (pInit == NULL) {
                CFlatFaderConfig init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            }
            break;
        }
        case FADERKIND_MESH: {
            if (pInit != NULL && pInit->m_kind != FADER_CONFIG_MESH) {
                Trace(
                    "CFaderMgr::Add (..., pInit ) - pInit does not point to the correct derived "
                    "class"
                );
                return NULL;
            }
            CFaderMesh* f = new CFaderMesh;
            fader = f;
            f->SetDefaultSurfaces(m_primarySurface, m_secondarySurface);
            f->SetDeviceManager(m_deviceManager);
            if (pInit == NULL) {
                CMeshFaderConfig init;
                if (f->ApplyInit(&init) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            } else {
                if (f->ApplyInit(pInit) == 0) {
                    Trace("CFaderMgr::Add (...) - Invalid init class");
                    delete fader;
                    return NULL;
                }
            }
            break;
        }
        default:
            Trace("CFaderMgr::Add (...) - nFaderType is invalid");
            break;
    }

    if (fader != NULL) {
        m_arr.Add(fader);
    }
    return fader;
}

RVA_COMPGEN(0x0017e160, 0x8, ??1CShapeFaderConfig@@QAE@XZ)

RVA(0x0017e170, 0x5b)
void CFaderMgr::Remove(CFader* pFader) {
    i32 i = 0;
    i32 count = m_arr.GetSize();
    while (i <= count - 1) {
        if (m_arr.m_pData[i] == pFader) {
            m_arr.RemoveAt(i);
            delete pFader;
            return;
        }
        i++;
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
        m_arr.m_pData = NULL;
    }
    m_arr.m_nMaxSize = 0;
    m_arr.m_nSize = 0;
}

// @identity-TODO: placement beside Trace is the only evidence for the setting's name.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017e220, 0xa)
void CFaderMgr::SetTraceEnabled(i32 enabled) {
    m_traceEnabled = enabled;
}

RVA(0x0017e230, 0xc)
void CFaderMgr::Trace(CString s) {
    static_cast<void>(s);
}

RVA_COMPGEN(0x0017e240, 0x51, ??1CFaderArray@@UAE@XZ)

RVA(0x0017e2a0, 0x188)
RVA_COMPGEN(0x0017e430, 0x1e, ??_GCFaderArray@@UAEPAXI@Z)
void CFaderArray::Serialize(CArchive& ar) {
    if (ar.IsStoring()) {
        ar.WriteCount(m_nSize);
    } else {
        i32 n = ar.ReadCount();
        if (n == 0) {
            if (m_pData != NULL) {
                delete[] m_pData;
                m_pData = NULL;
            }
            m_nMaxSize = 0;
            m_nSize = 0;
        } else if (m_pData == NULL) {
            m_pData = new CFader*[n];
            ConstructElements<CFader*>(m_pData, n);
            m_nMaxSize = n;
            m_nSize = n;
        } else if (n <= m_nMaxSize) {
            if (n > m_nSize) {
                ConstructElements<CFader*>(&m_pData[m_nSize], n - m_nSize);
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
            i32 newMax;
            if (n < m_nMaxSize + grow) {
                newMax = m_nMaxSize + grow;
            } else {
                newMax = n;
            }
            CFader** nd = new CFader*[newMax];
            memcpy(nd, m_pData, m_nSize * 4);
            ConstructElements<CFader*>(&nd[m_nSize], n - m_nSize);
            delete[] m_pData;
            m_pData = nd;
            m_nSize = n;
            m_nMaxSize = newMax;
        }
    }
    SerializeElements<CFader*>(ar, m_pData, m_nSize);
}
