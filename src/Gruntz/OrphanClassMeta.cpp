// OrphanClassMeta.cpp - class-metadata (SIZE) host for orphan include/Gruntz
// headers that no in-scope .cpp includes (the SIZE sweep hosts a header class's
// annotation at a .cpp that #includes it; these headers have no such owner, so
// this functionless TU is their host). labels.py text-scans SIZE_UNKNOWN tree-wide
// and the macro emits NO code, so this unit contributes nothing to the diff.
#include <rva.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/CGameRegistry.h>
#include <Gruntz/CState.h>
#include <Gruntz/Enums.h>
#include <Gruntz/GameModeBase.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeTableInline.h>

// ActNameRegistry.h
SIZE_UNKNOWN(CActColl);
SIZE_UNKNOWN(CActColl2);
SIZE_UNKNOWN(CActName);
// CGameRegistry.h
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CGameViewport);
SIZE_UNKNOWN(CSpriteFactoryHolder);
SIZE_UNKNOWN(CTileGrid);
// CState.h
SIZE_UNKNOWN(CState);
// Enums.h
SIZE_UNKNOWN(GruntzVolumeAttenuation);
// GameModeBase.h
SIZE_UNKNOWN(CGameModeBase);
// GruntIndicatorSprite.h
SIZE_UNKNOWN(CGruntEntry);
SIZE_UNKNOWN(CGruntLayerHolder);
SIZE_UNKNOWN(CGruntRenderable);
SIZE_UNKNOWN(CIndicatorActReg);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CIndicatorSyncHelper);
// LogicTypeTableInline.h
SIZE_UNKNOWN(CLogicTypeBuilder);
SIZE_UNKNOWN(CLogicTypeCtx);
SIZE_UNKNOWN(CLogicTypeReg);
