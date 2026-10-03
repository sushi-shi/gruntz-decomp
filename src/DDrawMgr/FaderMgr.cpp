#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/FaderConfigKind.h>
#include <Gruntz/FaderKind.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/FaderSubtypes.h>
#include <Gruntz/ShapeFaderConfig.h>

#include <string.h>

CFaderMgr::CFaderMgr() {
    m_active = false;
    m_traceEnabled = false;
}

CFaderMgr::~CFaderMgr() {
    FreeAll();
}

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

void CFaderMgr::FreeAll() {
    DeleteAll();
    m_active = false;
}

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
        m_arr.push_back(fader);
    }
    return fader;
}

void CFaderMgr::Remove(CFader* pFader) {
    i32 i = 0;
    i32 count = static_cast<i32>(m_arr.size());
    while (i <= count - 1) {
        if (m_arr[i] == pFader) {
            m_arr.erase(m_arr.begin() + i);
            delete pFader;
            return;
        }
        i++;
    }
}

void CFaderMgr::DeleteAll() {
    i32 i = 0;
    i32 last = (static_cast<i32>(m_arr.size()) - 1);
    if (last >= 0) {
        do {
            CFader* p = m_arr[i];
            delete p;
            i++;
            last = (static_cast<i32>(m_arr.size()) - 1);
        } while (i <= last);
    }
    m_arr.clear();
}

void CFaderMgr::SetTraceEnabled(b32 enabled) {
    m_traceEnabled = enabled;
}

void CFaderMgr::Trace(std::string s) {
    static_cast<void>(s);
}
