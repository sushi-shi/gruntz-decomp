#ifndef GRUNTZ_LOGICTYPEID_H
#define GRUNTZ_LOGICTYPEID_H

enum LogicTypeId {
    LOGIC_ANICYCLE = 0x3ea,  // CAniCycle::GetTypeTag               @0x0f450
    LOGIC_DONOTHING = 0x3ec, // CDoNothing::GetTypeTag              @0x0f6b0
    LOGIC_FRONTCANDY =
        0x3ef, // CFrontCandy::GetTypeTag             @0x0fa40 (vtable 0x1e84ec, slot 2)
    LOGIC_BEHINDCANDY = 0x3f0,           // CBehindCandy::GetTypeTag            @0x0fb70
    LOGIC_EYECANDY = 0x3f1,              // CEyeCandy::GetTypeTag               @0x0fca0
    LOGIC_BEHINDCANDYANI = 0x3f3,        // CBehindCandyAni::GetTypeTag         @0x10030
    LOGIC_EYECANDYANI = 0x3f4,           // CEyeCandyAni::GetTypeTag            @0x0ff00
    LOGIC_EXITTRIGGER = 0x3f7,           // CExitTrigger::GetTypeTag            @0x10870
    LOGIC_TELEPORTER = 0x3fc,            // CTeleporter::GetTypeTag             @0x10d80
    LOGIC_TILETRIGGERTRANSITION = 0x405, // CTileTriggerTransition::GetTypeTag  @0x11730
    LOGIC_BRICKZ = 0x409,                // CBrickz::GetTypeTag                 @0x11300
    LOGIC_OBJECTDROPPER = 0x40f,         // CObjectDropper::GetTypeTag          @0x124a0
    LOGIC_GRUNTSTAMINASPRITE = 0x410,    // CGruntStaminaSprite::GetTypeTag     @0x12020
    LOGIC_GRUNTTOYTIMESPRITE = 0x411,    // CGruntToyTimeSprite::GetTypeTag     @0x120e0
    LOGIC_STATICHAZARD = 0x416,          // CStaticHazard::GetTypeTag           @0x12ae0
    LOGIC_GRUNTWINGZTIMESPRITE = 0x417,  // CGruntWingzTimeSprite::GetTypeTag   @0x121a0
    LOGIC_TOOBSPIKEZ = 0x418,            // CToobSpikez::GetTypeTag             @0x12ba0
    LOGIC_PARTICLEZ = 0x41c,             // CParticlez::GetTypeTag              @0x12cd0
    LOGIC_SPOTLIGHT =
        0x41d, // CSpotLight::GetTypeTag              @0x12ff0 (vtable 0x1e75bc, slot 2)
    LOGIC_WAYPOINT = 0x420,     // CWayPoint::GetTypeTag               @0x10220
    LOGIC_ACTIONAREA = 0x423,   // CActionArea::GetTypeTag             @0x07f80
    LOGIC_PATHHAZARD = 0x425,   // CPathHazard::GetTypeTag             @0x132f0
    LOGIC_VOICETRIGGER = 0x426, // CVoiceTrigger::GetTypeTag           @0x133b0
    LOGIC_FORTRESSFLAG = 0x427, // CFortressFlag::GetTypeTag           @0x10e40
    LOGIC_TOYPEEK = 0x428, // CToyPeek::GetTypeTag                @0x11bf0 (vtable 0x1e7204, slot 2)
    LOGIC_WARPSTONEPAD =
        0x429, // CWarpStonePad::GetTypeTag           @0x10f00 (vtable 0x1e71ac, slot 2)
    LOGIC_GUARDPOINT = 0x42a, // CGuardPoint::GetTypeTag             @0x10350

    // ILT-census recovered leaves (slot-2 per-class type-id accessors, attributed by
    // vtable_scan.find_holding: each is a UNIQUE 1-holder override -> its RTTI class).
    LOGIC_GRUNT = 0x3e8,       // CGrunt::GetTypeTag                  @0x0f2a0 (vtable 0x1e8754)
    LOGIC_ROLLINGBALL = 0x3e9, // CRollingBall::GetTypeTag            @0x12f30 (vtable 0x1e86fc)
    LOGIC_SINGLEFRAMEMESSAGE =
        0x3eb,                     // CSingleFrameMessage::GetTypeTag     @0x0f580 (vtable 0x1e864c)
    LOGIC_DONOTHINGNORMAL = 0x3ed, // CDoNothingNormal::GetTypeTag        @0x0f7e0 (vtable 0x1e859c)
    LOGIC_SIMPLEANIMATION = 0x3ee, // CSimpleAnimation::GetTypeTag        @0x0f910 (vtable 0x1e8544)
    LOGIC_FRONTCANDYANI = 0x3f2,   // CFrontCandyAni::GetTypeTag          @0x0fdd0 (vtable 0x1e83e4)
    LOGIC_MENUSPARKLE = 0x3f5,     // CMenuSparkle::GetTypeTag            @0x10160 (vtable 0x1e82dc)
    LOGIC_GRUNTSTARTINGPOINT =
        0x3f6, // CGruntStartingPoint::GetTypeTag     @0x105b0 (vtable 0x1e8284)
    LOGIC_GRUNTCREATIONPOINT =
        0x3f8,                 // CGruntCreationPoint::GetTypeTag     @0x106e0 (vtable 0x1e81d4)
    LOGIC_WORMHOLE = 0x3fa,    // CWormhole::GetTypeTag               @0x10930 (vtable 0x1e817c)
    LOGIC_GRUNTPUDDLE = 0x3fb, // CGruntPuddle::GetTypeTag            @0x10cc0 (vtable 0x1e8124)
    LOGIC_CURSORSNAPSPRITE =
        0x3fd,                // CCursorSnapSprite::GetTypeTag       @0x11860 (vtable 0x1e8074)
    LOGIC_LEVELTIME = 0x3fe,  // CLevelTime::GetTypeTag              @0x11990 (vtable 0x1e801c)
    LOGIC_INGAMEICON = 0x407, // CInGameIcon::GetTypeTag             @0x11cb0 (vtable 0x1e7d04)
    LOGIC_INGAMETEXT = 0x408, // CInGameText::GetTypeTag             @0x11d70 (vtable 0x1e7cac)
    LOGIC_GRUNTSELECTEDSPRITE =
        0x40a, // CGruntSelectedSprite::GetTypeTag    @0x11e30 (vtable 0x1e7bfc)
    LOGIC_GRUNTHEALTHSPRITE =
        0x40b,                    // CGruntHealthSprite::GetTypeTag      @0x11f60 (vtable 0x1e7ba4)
    LOGIC_GRUNTTOYSPRITE = 0x40c, // CGruntToySprite::GetTypeTag         @0x12260 (vtable 0x1e7b4c)
    LOGIC_LIGHTFX = 0x40d,        // CLightFx::GetTypeTag                @0x123e0 (vtable 0x1e7af4)
    LOGIC_PROJECTILE = 0x412,     // CProjectile::GetTypeTag             @0x12960 (vtable 0x1e798c)
    LOGIC_DROPPEDOBJECT = 0x414,  // CDroppedObject::GetTypeTag          @0x12560 (vtable 0x1e78d4)
    LOGIC_DROPPEDOBJECTSHADOW =
        0x415,              // CDroppedObjectShadow::GetTypeTag    @0x12620 (vtable 0x1e787c)
    LOGIC_TIMEBOMB = 0x419, // CTimeBomb::GetTypeTag               @0x12a20 (vtable 0x1e771c)
    LOGIC_GRUNTPOWERUPSPRITE =
        0x41a,               // CGruntPowerupSprite::GetTypeTag     @0x12320 (vtable 0x1e76c4)
    LOGIC_EXPLOSION = 0x41b, // CExplosion::GetTypeTag              @0x12e00 (vtable 0x1e766c)
    LOGIC_SECRETTELEPORTERTRIGGER =
        0x41e,                  // CSecretTeleporterTrigger::GetTypeTag @0x109f0 (vtable 0x1e7564)
    LOGIC_KITCHENSLIME = 0x41f, // CKitchenSlime::GetTypeTag           @0x130b0 (vtable 0x1e750c)
    LOGIC_SINGLEANIMATION = 0x421, // CSingleAnimation::GetTypeTag        @0x10480 (vtable 0x1e745c)
    LOGIC_WARLORD = 0x422,         // CWarlord::GetTypeTag                @0x107a0 (vtable 0x1e7404)
    LOGIC_SECRETLEVELTRIGGER =
        0x42c, // CSecretLevelTrigger::GetTypeTag     @0x10b90 (vtable 0x1e8804)
};

#endif // GRUNTZ_LOGICTYPEID_H
