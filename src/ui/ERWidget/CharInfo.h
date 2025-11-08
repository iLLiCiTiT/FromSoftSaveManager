#pragma once
#include <QPainter>
#include <QLabel>
#include <QPaintEvent>

#include "../../parse/Parse.h"
#include "../Utils.h"

namespace fssm::ui::er {
class AttrSepWidget: public QWidget {
    Q_OBJECT
public:
    explicit AttrSepWidget(QWidget* parent): QWidget(parent) {
        setAttribute(Qt::WA_TranslucentBackground, true);
        m_pix = QPixmap(":/er_images/sep_1");
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    }
protected:
    QSize sizeHint() const override {return QSize(6, 0);};
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setClipRect(event->rect());
        painter.setPen(Qt::NoPen);
        painter.drawPixmap(rect(), m_pix);
    }
private:
    QPixmap m_pix;
};


class NameSepWidget: public QWidget {
    Q_OBJECT
public:
    explicit NameSepWidget(QWidget* parent): QWidget(parent) {
        setAttribute(Qt::WA_TranslucentBackground, true);
        m_pix = QPixmap(":/er_images/sep_2");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    }
protected:
    QSize sizeHint() const override {return QSize(0, 10);};
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setClipRect(event->rect());
        painter.setPen(Qt::NoPen);
        painter.drawPixmap(rect(), m_pix);
    }
private:
    QPixmap m_pix;
};

class NameWidget: public QWidget {
    Q_OBJECT
public:
    explicit NameWidget(QWidget* parent);
    void setText(QString text) { m_nameLabel->setText(text); }
private:
    QLabel* m_nameLabel = nullptr;
    NameSepWidget* m_sepWidget = nullptr;
};

class ERAttributesLabel: public QWidget {
    Q_OBJECT
public:
    explicit ERAttributesLabel(QWidget* parent);
private:
    PixmapLabel* m_iconWidget = nullptr;
    QLabel* m_labelWidget = nullptr;
};

class CharacterStatusWidget: public QWidget {
  Q_OBJECT
public:
    explicit CharacterStatusWidget(QWidget* parent);
    void setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo) const;
private:
    NameWidget* m_nameLabel = nullptr;
    QLabel* m_levelValueW = nullptr;
    QLabel* m_runesValueW = nullptr;
    QLabel* m_vigorValueW = nullptr;
    QLabel* m_mindValueW = nullptr;
    QLabel* m_enduranceValueW = nullptr;
    QLabel* m_strengthValueW = nullptr;
    QLabel* m_dexterityValueW = nullptr;
    QLabel* m_intelligenceValueW = nullptr;
    QLabel* m_faithValueW = nullptr;
    QLabel* m_arcaneValueW = nullptr;
    QLabel* m_hpValueW = nullptr;
    QLabel* m_fpValueW = nullptr;
    QLabel* m_staminaValueW = nullptr;
    void setEmpty() const;
};

class CharacterInfoWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterInfoWidget(QWidget* parent): QWidget(parent) {
        setAttribute(Qt::WA_TranslucentBackground, true);

        m_statusWidget = new CharacterStatusWidget(this);

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_statusWidget, 1);
    }
    void setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo) const { m_statusWidget->setCharacter(charInfo); }
private:
    CharacterStatusWidget* m_statusWidget = nullptr;
};
}