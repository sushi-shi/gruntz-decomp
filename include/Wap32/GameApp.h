#ifndef GRUNTZ_WAP32_GAMEAPP_H
#define GRUNTZ_WAP32_GAMEAPP_H

#include <Ints.h>

#include <Enums.h>
#include <Utils/AsyncKeyState.h>
#include <Utils/MillisPer.h>

#define FREE_GAME_MANAGER                                                                              if (m_gameMgr) {                                                                                       delete m_gameMgr;                                                                                  m_gameMgr = NULL;                                                                              }

#define CLEAR_GAME_MANAGER_WINDOW                                                                      m_gameWnd = NULL;                                                                                  m_owner = NULL

#endif
