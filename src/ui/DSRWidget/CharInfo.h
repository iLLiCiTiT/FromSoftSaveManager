#pragma once

#include <QLabel>

class CharacterStatusWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterStatusWidget(QWidget* parent);
    void setCharacter();
private:
    void setEmpty();
    QLabel* m_nameValueWidget = nullptr;
    QLabel* m_covenantValueWidget = nullptr;
    QLabel* m_levelValueWidget = nullptr;
    QLabel* m_soulsValueWidget = nullptr;
    QLabel* m_vitalityValueWidget = nullptr;
    QLabel* m_attunementValueWidget = nullptr;
    QLabel* m_enduranceValueWidget = nullptr;
    QLabel* m_strengthValueWidget = nullptr;
    QLabel* m_dexterityValueWidget = nullptr;
    QLabel* m_resistanceValueWidget = nullptr;
    QLabel* m_intelligenceValueWidget = nullptr;
    QLabel* m_faithValueWidget = nullptr;
    QLabel* m_humanityValueWidget = nullptr;
};

class CharacterInfoWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterInfoWidget(QWidget* parent);
    void setCharacter();
private:
    CharacterStatusWidget* m_statusWidget = nullptr;
};
