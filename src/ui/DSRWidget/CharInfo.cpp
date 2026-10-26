#include "CharInfo.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
#include <vector>

#include "Painting.h"
#include "../Utils.h"

namespace fssm::ui::dsr {
namespace {
    // Sizes are measured on a 1440p capture of the game's Status screen; FIXED_SCALE
    // maps them to the app. Frames, fonts and rows keep this one size; resizing only
    // changes how much fits.
    constexpr double FIXED_SCALE = 0.5;
    // Rank thresholds of the offering covenants (+1, +2, +3).
    constexpr int RANK_THRESHOLDS[3] = {10, 30, 80};

    struct StatRow {
        QString icon;           // :/dsr_images/<icon>, empty for none
        QString label;
        QString value;
        bool ruleBefore = false;    // the game separates stat groups with a rule
    };

    QString fromView(std::string_view s) {
        return QString::fromUtf8(s.data(), int(s.size()));
    }

    template <size_t N>
    QString label(const std::array<std::string_view, N>& labels, size_t index) {
        return index < N ? fromView(labels[index]) : QString();
    }

    // Attunement slots granted by the attunement stat.
    int attunementSlotCount(uint32_t attunement) {
        constexpr std::pair<uint32_t, int> STEPS[] = {
            {50, 10}, {41, 9}, {34, 8}, {28, 7}, {23, 6}, {19, 5}, {16, 4}, {14, 3}, {12, 2}, {10, 1}};
        for (const auto& [minimum, count] : STEPS) {
            if (attunement >= minimum) return count;
        }
        return 0;
    }

    const fssm::parse::dsr::BaseItem* findSpell(uint32_t spellId) {
        for (const auto& item : fssm::parse::dsr::ALL_ITEMS) {
            if (item.category == fssm::parse::dsr::ItemCategory::Spells && item.id == spellId) return &item;
        }
        return nullptr;
    }
}

QString covenantTitle(const fssm::parse::dsr::DSRCharacterInfo& c) {
    QString title = label(fssm::parse::dsr::DSR_COVENANT_LABELS, c.covenantId);
    const uint8_t level = c.covenantId < c.covenantLevels.size() ? c.covenantLevels[c.covenantId] : 0;
    int rank = 0;
    for (const int threshold : RANK_THRESHOLDS) rank += level >= threshold;
    if (rank > 0) title += QString("+%1").arg(rank);
    return title;
}

CharacterInfoWidget::CharacterInfoWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_scroll.setContentDrag(true);
}

void CharacterInfoWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    if (charInfo != nullptr) {
        m_character = *charInfo;
    } else {
        m_character.reset();
    }
    update();
}

QPixmap CharacterInfoWidget::icon(const QString& resource, int size) {
    const QString key = resource + QString("@%1").arg(size);
    auto it = m_iconCache.find(key);
    if (it != m_iconCache.end()) return *it;
    QPixmap pix(resource);
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
    if (!m_character.has_value()) {
        m_scroll.clear();
        painter.setFont(serifFont(px(36)));
        drawShadowText(painter, rect(), Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    const auto& c = *m_character;
    using namespace fssm::parse::dsr;

    const int lineH = px(46);
    const int ruleGap = px(12);
    const int pad = px(36);
    const int iconSize = px(40);
    const int nameBoxH = px(50);
    const int panelGap = px(16);
    const int minPanelW = px(520);
    const int maxPanelW = px(640);
    const int scrollBarW = px(12);
    const QFont textFont = serifFont(px(28));

    const std::vector<StatRow> statusRows = {
        {"", "Covenant", covenantTitle(c)},
        {"attr_level", "Level", QString::number(c.level), true},
        {"attr_souls", "Souls", QString::number(c.souls)},
        {"attr_vitality", "Vitality", QString::number(c.vitality), true},
        {"attr_attunement", "Attunement", QString::number(c.attunement)},
        {"attr_endurance", "Endurance", QString::number(c.endurance)},
        {"attr_strength", "Strength", QString::number(c.strength)},
        {"attr_dexterity", "Dexterity", QString::number(c.dexterity)},
        {"attr_resistance", "Resistance", QString::number(c.resistance)},
        {"attr_intelligence", "Intelligence", QString::number(c.intelligence)},
        {"attr_faith", "Faith", QString::number(c.faith)},
        {"attr_humanity", "Humanity", QString::number(c.humanity), true},
    };
    const std::vector<StatRow> vitalRows = {
        {"hp", "HP", QString("%1/%2").arg(c.hpCurrent).arg(c.hpMax)},
        {"stamina", "Stamina", QString::number(c.staminaMax)},
        {"res_bleed", "Bleed Resist", QString::number(c.bleedRes), true},
        {"res_poison", "Poison Resist", QString::number(c.poisonRes)},
        {"res_curse", "Curse Resist", QString::number(c.curseRes)},
    };
    const std::vector<StatRow> originRows = {
        {"", "State", c.hollowState == 8 ? "Hollow" : "Human"},
        {"", "Sex", c.gender == 1 ? "Male" : "Female"},
        {"", "Class", label(DSR_CLASSES, c.classId)},
        {"", "Physique", label(DSR_PHYSIQUE, c.physiqueId)},
        {"", "Gift", label(DSR_GIFTS, c.giftId)},
    };
    const int slotCount = attunementSlotCount(c.attunement);

    const auto rowsHeight = [&](const std::vector<StatRow>& rows) {
        int h = 0;
        for (const auto& row : rows) h += lineH + (row.ruleBefore ? ruleGap : 0);
        return h;
    };
    // Attunement slots: one cell per slot with the attuned spell's icon. The save's use
    // counts don't match what the game shows (three times as many), so they're left out.
    const int cellW = px(92);
    const int cellH = px(76);
    const auto slotColumns = [&](int panelW) { return std::max(1, (panelW - 2 * pad) / cellW); };
    const auto slotsHeight = [&](int panelW) {
        const int rows = slotCount > 0 ? (slotCount + slotColumns(panelW) - 1) / slotColumns(panelW) : 0;
        return lineH + px(6) + rows * cellH;
    };

    // Panels: status | vitals with the attunement slots | origin. Three columns when they
    // fit, else the last two stack in the second column.
    const QRect area = rect();
    int columns = 3;
    if (area.width() < 3 * minPanelW + 2 * panelGap) columns = 2;
    if (area.width() < 2 * minPanelW + panelGap) columns = 1;
    // Leave room for the scroll bar when the stack doesn't fit.
    int availableW = area.width();
    const int statusH = 2 * pad + nameBoxH + px(8) + rowsHeight(statusRows);
    const auto panelWidth = [&](int w) {
        return std::min(maxPanelW, (w - (columns - 1) * panelGap) / columns);
    };
    int panelW = panelWidth(availableW);
    const auto vitalsH = [&](int w) { return 2 * pad + rowsHeight(vitalRows) + ruleGap + slotsHeight(w); };
    const int originH = 2 * pad + rowsHeight(originRows);
    const auto contentHeight = [&](int w) {
        switch (columns) {
            case 3: return std::max({statusH, vitalsH(w), originH});
            case 2: return std::max(statusH, vitalsH(w) + panelGap + originH);
            default: return statusH + vitalsH(w) + originH + 2 * panelGap;
        }
    };
    if (contentHeight(panelW) > area.height()) {
        availableW -= scrollBarW;
        panelW = panelWidth(availableW);
    }
    const int contentH = std::max(contentHeight(panelW), area.height());
    m_scroll.setGeometry(area, QRect(area.right() - px(6) + 1, area.top(), px(6), area.height()), contentH);
    m_scroll.setStep(2 * lineH);
    const int top = area.top() - m_scroll.value();
    const int left = area.left();

    QRect statusPanel, vitalsPanel, originPanel;
    switch (columns) {
        case 3:
            statusPanel = QRect(left, top, panelW, contentH);
            vitalsPanel = QRect(left + panelW + panelGap, top, panelW, contentH);
            originPanel = QRect(left + 2 * (panelW + panelGap), top, panelW, contentH);
            break;
        case 2:
            statusPanel = QRect(left, top, panelW, contentH);
            vitalsPanel = QRect(left + panelW + panelGap, top, panelW, vitalsH(panelW));
            originPanel = QRect(vitalsPanel.left(), vitalsPanel.bottom() + 1 + panelGap, panelW,
                                contentH - vitalsPanel.height() - panelGap);
            break;
        default:
            statusPanel = QRect(left, top, panelW, statusH);
            vitalsPanel = QRect(left, statusPanel.bottom() + 1 + panelGap, panelW, vitalsH(panelW));
            originPanel = QRect(left, vitalsPanel.bottom() + 1 + panelGap, panelW, originH);
            break;
    }

    painter.setFont(textFont);
    const auto drawRows = [&](const std::vector<StatRow>& rows, const QRect& panel, int y) {
        const int x = panel.left() + pad;
        const int w = panel.width() - 2 * pad;
        for (const auto& row : rows) {
            if (row.ruleBefore) {
                drawRule(painter, x - px(6), x + w + px(6), y + ruleGap / 2 - 1);
                y += ruleGap;
            }
            const QRect line(x, y, w, lineH);
            int textLeft = line.left();
            if (!row.icon.isEmpty()) {
                painter.drawPixmap(line.left(), line.center().y() - iconSize / 2,
                                   icon(":/dsr_images/" + row.icon, iconSize));
                textLeft += iconSize + px(14);
            }
            const QFontMetrics fm(painter.font());
            const int valueW = std::min(fm.horizontalAdvance(row.value), line.right() - textLeft - px(60));
            drawShadowText(painter, QRect(line.right() - valueW + 1, line.top(), valueW, line.height()),
                           Qt::AlignRight | Qt::AlignVCenter, row.value);
            drawShadowText(painter, QRect(textLeft, line.top(), line.right() - valueW - px(12) - textLeft, line.height()),
                           Qt::AlignLeft | Qt::AlignVCenter, row.label);
            y += lineH;
        }
        return y;
    };

    // Status: name box, covenant, level and attributes.
    drawPanelFrame(painter, statusPanel, s * 1.4);
    const QRect nameBox(statusPanel.left() + pad, statusPanel.top() + pad, statusPanel.width() - 2 * pad, nameBoxH);
    drawNameBox(painter, nameBox);
    drawShadowText(painter, nameBox, Qt::AlignCenter, QString::fromStdU16String(c.name));
    drawRows(statusRows, statusPanel, nameBox.bottom() + 1 + px(8));

    // Vitals and attunement slots.
    drawPanelFrame(painter, vitalsPanel, s * 1.4);
    int y = drawRows(vitalRows, vitalsPanel, vitalsPanel.top() + pad);
    {
        const int x = vitalsPanel.left() + pad;
        const int w = vitalsPanel.width() - 2 * pad;
        drawRule(painter, x - px(6), x + w + px(6), y + ruleGap / 2 - 1);
        y += ruleGap;
        const QRect line(x, y, w, lineH);
        drawShadowText(painter, line, Qt::AlignLeft | Qt::AlignVCenter, "Attunement Slots");
        drawShadowText(painter, line, Qt::AlignRight | Qt::AlignVCenter, QString::number(slotCount));
        y += lineH + px(6);
        const int cols = slotColumns(vitalsPanel.width());
        const int gridLeft = x + (w - cols * cellW) / 2;
        const int spellIcon = px(64);
        int slot = 0;
        for (int i = 0; i < slotCount; ++i) {
            const QRect cell(gridLeft + (i % cols) * cellW, y + (i / cols) * cellH, cellW, cellH);
            const QRect iconRect(cell.center().x() - spellIcon / 2, cell.top(), spellIcon, spellIcon);
            // Spells fill the slots in order; a spell may take more than one slot.
            const AttunementSlot* attuned = nullptr;
            while (slot < int(c.attunementSlots.size())) {
                const AttunementSlot& candidate = c.attunementSlots[slot++];
                if (candidate.spellId != 0xFFFFFFFFu && candidate.spellId != 0) {
                    attuned = &candidate;
                    break;
                }
            }
            const BaseItem* spell = attuned != nullptr ? findSpell(attuned->spellId) : nullptr;
            QPixmap spellPix;
            if (spell != nullptr && inventoryResourcesAvailable()) {
                spellPix = icon(":/dsr_inv_images/" + fromView(spell->image), spellIcon);
            }
            if (!spellPix.isNull()) {
                painter.drawPixmap(iconRect, spellPix);
            } else {
                // Empty slot, or the spell's name when item images aren't bundled.
                painter.setPen(QPen(QColor(FRAME_COLOR.red(), FRAME_COLOR.green(), FRAME_COLOR.blue(), 120), 1));
                painter.setBrush(QColor(0, 0, 0, 50));
                painter.drawRect(QRectF(iconRect).adjusted(px(8) + 0.5, px(4) + 0.5, -px(8) - 0.5, -px(4) - 0.5));
                if (spell != nullptr) {
                    painter.setFont(serifFont(px(18)));
                    drawShadowText(painter, iconRect.adjusted(px(10), px(6), -px(10), -px(6)),
                                   Qt::AlignCenter | Qt::TextWordWrap, fromView(spell->label));
                    painter.setFont(textFont);
                }
            }
        }
    }

    // Character creation details.
    drawPanelFrame(painter, originPanel, s * 1.4);
    drawRows(originRows, originPanel, originPanel.top() + pad);

    m_scroll.paint(painter, QColor(0, 0, 0, 120), GOLD_COLOR);
}

void CharacterInfoWidget::wheelEvent(QWheelEvent* event) {
    if (m_character.has_value()) m_scroll.wheel(event, true);
}

void CharacterInfoWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_scroll.mousePress(event)) QWidget::mousePressEvent(event);
}

void CharacterInfoWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_scroll.mouseMove(event)) QWidget::mouseMoveEvent(event);
}

void CharacterInfoWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_scroll.mouseRelease(event)) QWidget::mouseReleaseEvent(event);
}

void CharacterInfoWidget::leaveEvent(QEvent* event) {
    m_scroll.leave();
    QWidget::leaveEvent(event);
}
}
