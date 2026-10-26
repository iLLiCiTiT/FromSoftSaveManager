#include "Covenants.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>

#include "CharInfo.h"
#include "Painting.h"
#include "../Utils.h"

namespace fssm::ui::dsr {
namespace {
    // Same scale as the Status screen.
    constexpr double FIXED_SCALE = 0.5;
    // Rank thresholds of the offering covenants (+1, +2, +3).
    constexpr int RANK_THRESHOLDS[3] = {10, 30, 80};

    struct Covenant {
        int id;             // index into DSR_COVENANT_LABELS and covenantLevels
        const char* image;  // :/dsr_inv_images/<image>: the offering, or the covenant's ring
        bool offerings;     // ranks up with offerings
    };

    constexpr Covenant COVENANTS[] = {
        {1, "white_seance_ring", false},
        {2, "ring_of_the_sun_princess", false},
        {3, "sunlight_medal", true},
        {4, "humanity", true},
        {5, "dragon_scale", true},
        {6, "eye_of_death", true},
        {7, "cat_covenant_ring", true},
        {8, "souvenir_of_reprisal", true},
        {9, "humanity", true},
    };
}

CovenantsWidget::CovenantsWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_scroll.setContentDrag(true);
}

void CovenantsWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    if (charInfo != nullptr) {
        m_character = *charInfo;
    } else {
        m_character.reset();
    }
    update();
}

QPixmap CovenantsWidget::icon(const QString& resource, int size) {
    const QString key = resource + QString("@%1").arg(size);
    auto it = m_iconCache.find(key);
    if (it != m_iconCache.end()) return *it;
    QPixmap pix(resource);
    if (!pix.isNull()) pix = pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_iconCache.insert(key, pix);
    return pix;
}

void CovenantsWidget::paintEvent(QPaintEvent* event) {
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
    using fssm::parse::dsr::DSR_COVENANT_LABELS;

    const int pad = px(36);
    const int titleH = px(52);
    const int rowH = px(70);
    const int iconSize = px(60);
    const int scrollBarW = px(12);
    const int count = int(std::size(COVENANTS));
    const int contentH = 2 * pad + titleH + px(14) + count * rowH;
    const QRect area = rect();
    const bool scrolls = contentH > area.height();
    const int panelW = std::min(px(1100), area.width() - (scrolls ? scrollBarW : 0));
    m_scroll.setGeometry(area, QRect(area.right() - px(6) + 1, area.top(), px(6), area.height()), contentH);
    m_scroll.setStep(rowH);
    const QRect panel(area.left(), area.top() - m_scroll.value(), panelW, std::max(contentH, area.height()));

    drawPanelFrame(painter, panel, s * 1.4);
    const int x = panel.left() + pad;
    const int w = panel.width() - 2 * pad;

    // Title row: the joined covenant, as on the Status screen.
    const QRect title(x, panel.top() + pad, w, titleH);
    painter.setFont(serifFont(px(30)));
    drawShadowText(painter, title, Qt::AlignLeft | Qt::AlignVCenter, "Covenant");
    drawShadowText(painter, title, Qt::AlignRight | Qt::AlignVCenter, covenantTitle(c));
    drawRule(painter, x - px(6), x + w + px(6), title.bottom() + px(6));

    int y = title.bottom() + 1 + px(14);
    for (const Covenant& covenant : COVENANTS) {
        const QRect row(x, y, w, rowH);
        y += rowH;
        const bool joined = covenant.id == c.covenantId;
        if (joined) fillLeather(painter, row.adjusted(-px(10), px(3), px(10), -px(3)), SELECTED_COLOR);

        int textLeft = row.left() + px(6);
        if (inventoryResourcesAvailable()) {
            const QPixmap pix = icon(QString(":/dsr_inv_images/") + covenant.image, iconSize);
            if (!pix.isNull()) painter.drawPixmap(textLeft, row.center().y() - iconSize / 2, pix);
            textLeft += iconSize + px(14);
        }

        const int offerings = covenant.id < int(c.covenantLevels.size()) ? c.covenantLevels[covenant.id] : 0;
        int rank = 0;
        for (const int threshold : RANK_THRESHOLDS) rank += offerings >= threshold;

        painter.setFont(serifFont(px(28)));
        const QFontMetrics fm(painter.font());
        // Offerings and rank on the right; the progress line runs under them.
        const int valuesW = px(230);
        const QRect values(row.right() - valuesW + 1, row.top(), valuesW, rowH);
        if (covenant.offerings) {
            const int next = rank < 3 ? RANK_THRESHOLDS[rank] : RANK_THRESHOLDS[2];
            const QString countText = rank < 3 ? QString("%1/%2").arg(offerings).arg(next) : QString::number(offerings);
            const QRect upper(values.left(), values.top(), values.width(), rowH * 62 / 100);
            drawShadowText(painter, upper, Qt::AlignRight | Qt::AlignVCenter, countText);
            if (rank > 0) {
                drawShadowText(painter, upper, Qt::AlignLeft | Qt::AlignVCenter, QString("+%1").arg(rank), AREA_COLOR);
            }
            // Thin progress bar towards the next rank.
            const int previous = rank > 0 ? RANK_THRESHOLDS[rank - 1] : 0;
            const double progress = rank >= 3 ? 1.0 : double(offerings - previous) / (next - previous);
            const QRect bar(values.left(), upper.bottom() + px(4), values.width(), std::max(2, px(6)));
            painter.fillRect(bar, QColor(0, 0, 0, 140));
            painter.fillRect(QRect(bar.left(), bar.top(), int(bar.width() * std::clamp(progress, 0.0, 1.0)), bar.height()),
                             joined ? GOLD_LIGHT_COLOR : GOLD_COLOR);
        } else {
            drawShadowText(painter, values, Qt::AlignRight | Qt::AlignVCenter, "-", DIM_TEXT_COLOR);
        }

        const QString name = covenant.id < int(DSR_COVENANT_LABELS.size())
            ? QString::fromUtf8(DSR_COVENANT_LABELS[covenant.id].data(), int(DSR_COVENANT_LABELS[covenant.id].size()))
            : QString();
        const bool active = joined || offerings > 0;
        drawShadowText(painter, QRect(textLeft, row.top(), values.left() - px(16) - textLeft, rowH),
                       Qt::AlignLeft | Qt::AlignVCenter, name, active ? TEXT_COLOR : DIM_TEXT_COLOR);
        if (!joined) drawRule(painter, x, x + w, row.bottom(), 90);
    }

    m_scroll.paint(painter, QColor(0, 0, 0, 120), GOLD_COLOR);
}

void CovenantsWidget::wheelEvent(QWheelEvent* event) {
    if (m_character.has_value()) m_scroll.wheel(event, true);
}

void CovenantsWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_scroll.mousePress(event)) QWidget::mousePressEvent(event);
}

void CovenantsWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_scroll.mouseMove(event)) QWidget::mouseMoveEvent(event);
}

void CovenantsWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_scroll.mouseRelease(event)) QWidget::mouseReleaseEvent(event);
}

void CovenantsWidget::leaveEvent(QEvent* event) {
    m_scroll.leave();
    QWidget::leaveEvent(event);
}
}
