#ifndef SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
#define SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H

#include <rva.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class CTileTriggerLogic;
class CTileTriggerContainer;

class CTileTriggerSwitchLogic {
public:
    virtual i32 Setup(
        CTileTriggerContainer* owner,
        i32 typeId,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        i32 linkGate,
        i32 damageParam,
        i32 checkpointType
    );

    virtual i32 BuildSmall(
        CTileTriggerContainer* owner,
        i32 typeId,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        const RECT* rect,
        i32 linkGate,
        i32 damageParam,
        i32 checkpointType
    );
    virtual i32 SwitchDown();
    virtual i32 SwitchUp();

    CTileTriggerSwitchLogic();

    ~CTileTriggerSwitchLogic() {
        m_initGate = 0;
    }

    i32 VerifyBlockLinksB();
    i32 VerifyBlockLinks();

    i32 ValidateByType(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj);
    i32 SaveState(CFileMemBase* s);

    i32 LoadState(CFileMemBase* s);

    i32 m_typeId;

    i32 m_tileX;
    i32 m_tileY;
    i32 m_cellKey;
    i32 m_linkGate;
    i32 m_damageParam;
    i32 m_reserved1c;
    i32 m_initGate;

    CTileTriggerContainer* m_owner;
    i32 m_checkpointType;
    i32 m_block[24];
};
SIZE(0x8c);
VTBL(CTileTriggerSwitchLogic, 0x001eae8c);

class CTileMultiTriggerSwitchLogic : public CTileTriggerSwitchLogic {
public:
    CTileMultiTriggerSwitchLogic();
};
SIZE(0x8c);
VTBL(CTileMultiTriggerSwitchLogic, 0x001eaeb4);

class CTileExclusiveTriggerSwitchLogic : public CTileTriggerSwitchLogic {

    virtual i32 SwitchDown() OVERRIDE;

public:
    CTileExclusiveTriggerSwitchLogic();
};
SIZE(0x8c);
VTBL(CTileExclusiveTriggerSwitchLogic, 0x001eaecc);

class CTileSecretTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 SwitchDown() OVERRIDE;

public:
    CTileSecretTriggerSwitchLogic();
};
SIZE(0x8c);
VTBL(CTileSecretTriggerSwitchLogic, 0x001eaf24);

class CTileTimeTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 SwitchDown() OVERRIDE;
    virtual i32 SwitchUp() OVERRIDE;

public:
    CTileTimeTriggerSwitchLogic();
};
SIZE(0x8c);
VTBL(CTileTimeTriggerSwitchLogic, 0x001eaf3c);

class CCheckpointTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 SwitchDown() OVERRIDE;
    virtual i32 SwitchUp() OVERRIDE;

public:
    CCheckpointTriggerSwitchLogic();

    virtual i32 BuildSmall(
        CTileTriggerContainer* owner,
        i32 typeId,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        const RECT* rect,
        i32 linkGate,
        i32 damageParam,
        i32 checkpointType
    ) OVERRIDE;
};
SIZE(0x8c);
VTBL(CCheckpointTriggerSwitchLogic, 0x001eaf54);

#endif // SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
