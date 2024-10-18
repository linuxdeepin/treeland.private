#pragma once

#include <QAbstractTransition>
#include <QEvent>
#include <QState>

class QStateMachine;

class StateSwitcher : public QState
{
    explicit StateSwitcher(QStateMachine *machine);

    void onEntry(QEvent *) override;
    void onExit(QEvent *) override;
};

class StateSwitchTransition : public QAbstractTransition
{
public:
    explicit StateSwitchTransition(int rand);

protected:
    bool eventTest(QEvent *event) override;
    void onTransition(QEvent *) override;

private:
    int m_rand;
};

class StateSwitchEvent : public QEvent
{
public:
    explicit StateSwitchEvent(int rand);

    static constexpr QEvent::Type StateSwitchType = QEvent::Type(QEvent::User + 256);

    int rand() const { return m_rand; }

private:
    int m_rand;
};
