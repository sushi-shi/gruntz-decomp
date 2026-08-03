#ifndef GRUNTZ_CUFO_H
#define GRUNTZ_CUFO_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/SerialArchive.h>

class CUFO : public CPathHazard {
public:
    RVA(0x000b4c40, 0x4b)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode mode, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CPathHazard::SerializeMove(ar, mode, c, d)) {
            return 0;
        }
        if (mode == SERIAL_POSTLOAD) {
            CWwdGameObjectA* o = m_object;
            o->m_drawActive = 1;
            // Two domains, one slot, and the SHAPE is byte-evidenced: retail stores
            // the register holding `mode` (`mov [eax+0x50],edi`), not an immediate.
            // SERIAL_POSTLOAD and SHADE_ALPHA_16 are both 8, and CUFO's ctor sets
            // that same SHADE_ALPHA_16 / 0x80 pair on this object.
            o->m_drawFillCmd = static_cast<ShadeMode>(mode);
            o->m_fillFraction = 0x80;
        }
        return 1;
    }

    static inline void SerQuadPair(CFileMemBase* s, i32 tag, CHazardTimer* p) {
        if (tag != 4) {
            if (tag == 7) {
                s->Read(&p->m_deadline, 8);
                s->Read(&p->m_window, 8);
            }
        } else {
            s->Write(&p->m_deadline, 8);
            s->Write(&p->m_window, 8);
        }
    }

    RVA(0x000133b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_UFO;
    }
    CUFO() {}
    CUFO(CGameObject* obj);

    virtual i32 Tick() OVERRIDE;
};
SIZE(0x130);

#endif // GRUNTZ_CUFO_H
