#include "CharInfo.h"

#include <QPaintEvent>
#include <QPainter>
#include <cmath>

#include "Painting.h"

namespace fssm::ui::ds3 {
namespace {
    // Frame, fonts and rows keep one size (the look at the default window size);
    // resizing only changes how much fits.
    constexpr double FIXED_SCALE = 0.82;

    struct StatRow {
        QString icon;           // :/ds3_images/<icon>, empty for none
        QString label;
        QString value;
        bool gapBefore = false; // the game separates stat groups with a blank line
    };

    QString pair(uint32_t current, uint32_t max) {
        return QString("%1/ %2").arg(current).arg(max);
    }
}

CharacterInfoWidget::CharacterInfoWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void CharacterInfoWidget::setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo) {
    if (charInfo != nullptr) {
        m_character = *charInfo;
    } else {
        m_character.reset();
    }
    update();
}

QPixmap CharacterInfoWidget::icon(const QString& name, int size) {
    const QString key = name + QString("@%1").arg(size);
    auto it = m_iconCache.find(key);
    if (it != m_iconCache.end()) return *it;
    QPixmap pix(":/ds3_images/" + name);
    if (!pix.isNull()) pix = pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_iconCache.insert(key, pix);
    return pix;
}

void CharacterInfoWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };

    const QRect panel = rect();
    drawStonePanel(painter, panel);
    const QRect inner = panel.adjusted(px(18), px(8), -px(18), -px(12));
    if (!m_character.has_value()) {
        painter.setFont(serifFont(px(20)));
        drawShadowText(painter, panel, Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    const auto& c = *m_character;

    // Title row: "Player Status" on the left, the character's name on the right.
    const QRect title(inner.left(), inner.top(), inner.width(), px(32));
    painter.setFont(serifFont(px(21)));
    drawShadowText(painter, title.adjusted(px(4), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "Player Status");
    drawShadowText(painter, title.adjusted(0, 0, -px(4), 0), Qt::AlignRight | Qt::AlignVCenter,
                   QString::fromStdU16String(c.name));
    drawGoldRule(painter, title.left(), title.right(), title.bottom() + 2, 220);

    const std::vector<StatRow> left = {
        {"attr_level", "Level", QString::number(c.level)},
        {"", "Hollowing", QString::number(c.hollowing)},
        {"attr_vigor", "Vigor", QString::number(c.vigor), true},
        {"attr_attunement", "Attunement", QString::number(c.attunement)},
        {"attr_endurance", "Endurance", QString::number(c.endurance)},
        {"attr_vitality", "Vitality", QString::number(c.vitality)},
        {"attr_strength", "Strength", QString::number(c.strength)},
        {"attr_dexterity", "Dexterity", QString::number(c.dexterity)},
        {"attr_intelligence", "Intelligence", QString::number(c.intelligence)},
        {"attr_faith", "Faith", QString::number(c.faith)},
        {"attr_luck", "Luck", QString::number(c.luck)},
    };
    const std::vector<StatRow> right = {
        {"attr_souls", "Souls", QString::number(c.souls)},
        {"attr_hp", "HP", pair(c.hpCurrent, c.hpMax), true},
        {"attr_fp", "FP", pair(c.fpCurrent, c.fpMax)},
        {"attr_stamina", "Stamina", QString::number(c.staminaMax)},
        {"attr_bleed_res", "Bleed", QString::number(c.bleedRes), true},
        {"attr_poison_res", "Poison", QString::number(c.poisonRes)},
        {"attr_frost_res", "Frost", QString::number(c.frostRes)},
        {"attr_curse_res", "Curse", QString::number(c.curseRes)},
    };

    const int top = title.bottom() + px(14);
    const int gap = px(26);
    const bool twoColumns = inner.width() >= px(520);
    const int columnWidth = twoColumns ? (inner.width() - gap) / 2 : inner.width();
    const int lineH = px(29);
    const int iconSize = px(22);
    painter.setFont(serifFont(px(19)));
    const auto drawColumn = [&](const std::vector<StatRow>& rows, int x, int y) {
        for (const auto& row : rows) {
            if (row.gapBefore) y += px(14);
            const QRect line(x, y, columnWidth, lineH);
            int textLeft = line.left() + iconSize + px(4);
            if (!row.icon.isEmpty()) {
                painter.drawPixmap(line.left(), line.center().y() - iconSize / 2, icon(row.icon, iconSize));
            }
            drawShadowText(painter, QRect(textLeft, line.top(), line.right() - textLeft, line.height()),
                           Qt::AlignLeft | Qt::AlignVCenter, row.label);
            drawShadowText(painter, line.adjusted(0, 0, -px(4), 0), Qt::AlignRight | Qt::AlignVCenter, row.value);
            drawGoldRule(painter, line.left(), line.right(), line.bottom(), 120);
            y += lineH + px(1);
        }
        return y;
    };
    const int leftBottom = drawColumn(left, inner.left(), top);
    if (twoColumns) {
        drawColumn(right, inner.left() + columnWidth + gap, top);
    } else {
        drawColumn(right, inner.left(), leftBottom + px(10));
    }
}
}
