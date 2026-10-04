#ifndef GRUNTZ_GRUNTZ_GAMEPLAYPRESENTATION_H
#define GRUNTZ_GRUNTZ_GAMEPLAYPRESENTATION_H
#include <Gruntz/GameStateId.h>

enum GameplayPresentationKind { RestoreGameplay, EnterGameplayState, StartGameplayInput };

struct GameplayPresentationAction {
    GameplayPresentationAction(GameplayPresentationKind kind = RestoreGameplay,
        GameStateId previous = GAMESTATE_NONE) : kind(kind), previous(previous), recovery(false) {}
    GameplayPresentationKind kind;
    GameStateId previous;
    bool recovery;
};

// A requested scene is composed on the next frame, outside an interrupted draw.
// Recovery preserves its completion action; cancellation discards it.
class GameplayPresentation {
public:
    GameplayPresentation() : m_phase(Idle) {}
    bool request(GameplayPresentationKind kind, GameStateId previous) {
        if (active()) return false;
        m_action = GameplayPresentationAction(kind, previous);
        m_phase = Prepare;
        return true;
    }
    bool recover() {
        if (!active() || m_action.recovery) return false;
        m_action.recovery = true;
        m_phase = Prepare;
        return true;
    }
    void prepared() { if (m_phase == Prepare) m_phase = Playing; }
    bool take(GameplayPresentationAction& action) {
        if (m_phase != Playing) return false;
        action = m_action;
        cancel();
        return true;
    }
    void cancel() { m_phase = Idle; }
    bool active() const { return m_phase != Idle; }
    bool needsPrepare() const { return m_phase == Prepare; }
    GameplayPresentationAction action() const { return m_action; }
private:
    enum Phase { Idle, Prepare, Playing };
    Phase m_phase;
    GameplayPresentationAction m_action;
};
#endif
