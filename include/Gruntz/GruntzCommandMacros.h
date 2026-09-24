#ifndef GRUNTZ_GRUNTZ_GRUNTZCOMMANDMACROS_H
#define GRUNTZ_GRUNTZ_GRUNTZCOMMANDMACROS_H

#define PLAYCUE(TAG)                                                                               \
    if (m_world->m_soundRegistry->m_silentMode == false) {                                         \
        SoundCue* _c = static_cast<SoundCue*>(m_world->m_soundRegistry->Lookup(TAG));              \
        if (_c)                                                                                    \
            _c->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);                                      \
    }

#define PLAYCUE_MAP(TAG, VAR)                                                                      \
    {                                                                                              \
        SoundCueRegistry* _reg = m_world->m_soundRegistry;                                         \
        if (_reg->m_silentMode == false) {                                                         \
            VAR = 0;                                                                               \
            MapLookup(_reg->m_cues, TAG, VAR);                                                     \
            if (VAR)                                                                               \
                VAR->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);                                 \
        }                                                                                          \
    }

#define ITEMCHEAT(N, MSG)                                                                          \
    {                                                                                              \
        CPlay* _g = PickPlayOrPausedState();                                                       \
        if (!_g)                                                                                   \
            return 0;                                                                              \
        _g->SetCursorFrame(N);                                                                     \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChatMessage(MSG);                                                                    \
        return 1;                                                                                  \
    }

#define WARP(N, ERR)                                                                               \
    {                                                                                              \
        m_gameMode = GAMEMODE_QUESTZ;                                                              \
        m_strWorldFile.Empty();                                                                    \
        if (!PassClickToPlayState((N), 0, 1))                                                      \
            ReportError(IDX(IDS_SET_GAME_STATE), (ERR));                                           \
        return 1;                                                                                  \
    }

#define BRICKPICKUP(ID, MSG)                                                                       \
    {                                                                                              \
        if (!PickPlayOrPausedState())                                                              \
            return 0;                                                                              \
        CGrunt* _cell = m_triggerMgr->m_recList.GetCount() != 1                                    \
                            ? 0                                                                    \
                            : m_triggerMgr->m_units                                                \
                                  [m_triggerMgr->HeadRec()->m_y                                    \
                                   + m_triggerMgr->HeadRec()->m_x * TM_UNITS_PER_PLAYER];          \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_playerIndex != g_curPlayer)                                                   \
            return 0;                                                                              \
        CGrunt* _c2 =                                                                              \
            m_triggerMgr                                                                           \
                ->m_units[_cell->m_unitIndex + _cell->m_playerIndex * TM_UNITS_PER_PLAYER];        \
        i32 _r = (_c2 && _c2->m_entranceCommitted) ? _c2->LoadPickupSprites(ID, 0, 0, 0, 1) : 0;   \
        if (!_r)                                                                                   \
            return 0;                                                                              \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChatMessage(MSG);                                                                    \
        return 1;                                                                                  \
    }

#define BRICKABILITY(N, MSG)                                                                       \
    {                                                                                              \
        if (!PickPlayOrPausedState())                                                              \
            return 0;                                                                              \
        CGrunt* _cell = m_triggerMgr->m_recList.GetCount() != 1                                    \
                            ? 0                                                                    \
                            : m_triggerMgr->m_units                                                \
                                  [m_triggerMgr->HeadRec()->m_y                                    \
                                   + m_triggerMgr->HeadRec()->m_x * TM_UNITS_PER_PLAYER];          \
        if (!_cell)                                                                                \
            return 0;                                                                              \
        if (_cell->m_playerIndex != g_curPlayer)                                                   \
            return 0;                                                                              \
        if (!_cell->LoadGruntAbilityTuning(N))                                                     \
            return 0;                                                                              \
        PLAYCUE("GAME_MAJORCHEAT");                                                                \
        AppendChatMessage(MSG);                                                                    \
        return 1;                                                                                  \
    }

#define RESTART(N)                                                                                 \
    {                                                                                              \
        CMenuState* mus = 0;                                                                       \
        GameStateId st = m_curState->Update();                                                     \
        if (st == GAMESTATE_MENU) {                                                                \
            mus = static_cast<CMenuState*>(m_curState);                                            \
            (static_cast<CMenuState*>(m_curState))->StopMusicChain();                              \
            while (ShowCursor(0) >= 0) {                                                           \
            }                                                                                      \
        }                                                                                          \
        PlayMovieEntry(IDX(N));                                                                    \
        if (mus) {                                                                                 \
            mus->StartMusic();                                                                     \
            while (ShowCursor(1) < 0) {                                                            \
            }                                                                                      \
        }                                                                                          \
        return 1;                                                                                  \
    }

#define RESTART2(N)                                                                                \
    {                                                                                              \
        CMenuState* mus = 0;                                                                       \
        GameStateId st = m_curState->Update();                                                     \
        if (st == GAMESTATE_MENU) {                                                                \
            mus = static_cast<CMenuState*>(m_curState);                                            \
            (static_cast<CMenuState*>(m_curState))->StopMusicChain();                              \
        }                                                                                          \
        PlayMovieEntry(IDX(N));                                                                    \
        if (mus)                                                                                   \
            mus->StartMusic();                                                                     \
        return 1;                                                                                  \
    }

#endif // GRUNTZ_GRUNTZ_GRUNTZCOMMANDMACROS_H
