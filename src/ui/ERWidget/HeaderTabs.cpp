#include "HeaderTabs.h"

#include <QLinearGradient>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace fssm::ui::er {
namespace {
    // Fixed pixel sizes: the header keeps one size when the window is resized. Height,
    // font and rule position match the "Characters" title next to it (CharacterList.cpp).
    constexpr int BAR_HEIGHT = 30;
    constexpr int RULE_Y = BAR_HEIGHT - 5;
    constexpr int ICON_SIZE = 18;
    constexpr int ICON_GAP = 6;
    constexpr int TAB_SPACING = 24;
    constexpr int FONT_SIZE = 17;
    constexpr int TITLE_LINE_HEIGHT = 10;

    const QColor TEXT_COLOR(228, 224, 210);
    const QColor DIM_TEXT_COLOR(150, 144, 128);
    const QColor GOLD_COLOR(190, 168, 112);

    QFont serifFont(int pixelSize) {
        QFont font("Georgia");
        font.setStyleHint(QFont::Serif);
        font.setPixelSize(std::max(8, pixelSize));
        return font;
    }

    void drawText(QPainter& painter, const QRect& rect, int flags, const QString& text, const QColor& color) {
        painter.save();
        painter.setPen(QColor(0, 0, 0, 210));
        painter.drawText(rect.translated(1, 1), flags, text);
        painter.setPen(color);
        painter.drawText(rect, flags, text);
        painter.restore();
    }
}

HeaderTabs::HeaderTabs(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    m_animation.setInterval(16);
    connect(&m_animation, &QTimer::timeout, this, &HeaderTabs::stepAnimation);
    m_titleLine = QPixmap(":/er_images/title_line");
    m_stack = new QStackedWidget(this);
    m_stack->setAttribute(Qt::WA_TranslucentBackground, true);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, BAR_HEIGHT, 0, 0);
    layout->addWidget(m_stack);
}

void HeaderTabs::addTab(const QString& title, const QString& icon, QWidget* page) {
    addTab(title, QPixmap(":/er_images/" + icon), page);
}

void HeaderTabs::addTab(const QString& title, const QPixmap& icon, QWidget* page) {
    QPixmap pix = icon;
    if (!pix.isNull() && (pix.width() > ICON_SIZE || pix.height() > ICON_SIZE)) {
        pix = pix.scaled(ICON_SIZE, ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_tabs.push_back({title, pix});
    m_expand.push_back(m_tabs.size() == 1 ? 1.0 : 0.0);
    m_stack->addWidget(page);
    if (m_current < 0) setCurrentIndex(0);
    update();
}

void HeaderTabs::setCurrentIndex(int index) {
    if (index < 0 || index >= int(m_tabs.size()) || index == m_current) return;
    m_current = index;
    m_stack->setCurrentIndex(index);
    m_animation.start();
    update();
}

void HeaderTabs::stepAnimation() {
    // About 160 ms from the icon alone to the full title.
    constexpr double STEP = 16.0 / 160.0;
    bool done = true;
    for (int i = 0; i < int(m_expand.size()); ++i) {
        const double target = i == m_current || i == m_hover ? 1.0 : 0.0;
        m_expand[i] = target > m_expand[i] ? std::min(target, m_expand[i] + STEP) : std::max(target, m_expand[i] - STEP);
        done = done && m_expand[i] == target;
    }
    if (done) m_animation.stop();
    update();
}

int HeaderTabs::tabWidth(int index, bool iconOnly) const {
    if (iconOnly) return ICON_SIZE;
    return ICON_SIZE + ICON_GAP + QFontMetrics(serifFont(FONT_SIZE)).horizontalAdvance(m_tabs[index].title) + 4;
}

bool HeaderTabs::compact() const {
    int x = 12;
    for (int i = 0; i < int(m_tabs.size()); ++i) x += tabWidth(i, false) + TAB_SPACING;
    return x - TAB_SPACING > width();
}

QRect HeaderTabs::tabRect(int index) const {
    const bool iconsOnly = compact();
    int x = 12;
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        int w = tabWidth(i, false);
        if (iconsOnly) {
            const int icon = tabWidth(i, true);
            w = icon + int(std::round(m_expand[i] * (w - icon)));
        }
        if (i == index) return {x, 0, w, BAR_HEIGHT};
        x += w + TAB_SPACING;
    }
    return {};
}

int HeaderTabs::tabAt(const QPoint& pos) const {
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        if (tabRect(i).adjusted(-TAB_SPACING / 2, 0, TAB_SPACING / 2, 0).contains(pos)) return i;
    }
    return -1;
}

void HeaderTabs::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Dark band with soft, blurred edges so it blends into the background (fades out to
    // the right most), with a faint gold rule along its bottom.
    const QRect bar(0, 0, width(), RULE_Y + 1);
    if (m_band.size() != bar.size()) {
        m_band = QImage(bar.size(), QImage::Format_ARGB32_Premultiplied);
        m_band.fill(Qt::transparent);
        QPainter bandPainter(&m_band);
        QLinearGradient band(0, 0, bar.width(), 0);
        band.setColorAt(0.0, QColor(10, 10, 8, 0));
        band.setColorAt(std::min(0.5, 10.0 / std::max(1, bar.width())), QColor(10, 10, 8, 170));
        band.setColorAt(0.55, QColor(10, 10, 8, 120));
        band.setColorAt(1.0, QColor(10, 10, 8, 0));
        bandPainter.fillRect(m_band.rect(), band);
        QLinearGradient feather(0, 0, 0, bar.height());
        feather.setColorAt(0.0, QColor(0, 0, 0, 0));
        feather.setColorAt(10.0 / bar.height(), QColor(0, 0, 0, 255));
        feather.setColorAt(0.75, QColor(0, 0, 0, 255));
        feather.setColorAt(1.0, QColor(0, 0, 0, 90));
        bandPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        bandPainter.fillRect(m_band.rect(), feather);
    }
    painter.drawImage(bar.topLeft(), m_band);
    QLinearGradient rule(bar.topLeft(), bar.topRight());
    QColor ruleColor = GOLD_COLOR;
    ruleColor.setAlpha(80);
    rule.setColorAt(0.04, ruleColor);
    rule.setColorAt(0.6, ruleColor);
    ruleColor.setAlpha(0);
    rule.setColorAt(1.0, ruleColor);
    painter.fillRect(QRect(0, RULE_Y, width(), 1), rule);

    painter.setFont(serifFont(FONT_SIZE));
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        const QRect r = tabRect(i);
        const bool current = i == m_current;
        const bool hovered = !current && i == m_hover;
        // The current tab sits on the game's glowing title line, with a soft light behind it.
        if (current || hovered) {
            painter.setOpacity(current ? 1.0 : 0.4);
            const QRect glowRect(r.left() - 24, bar.top(), r.width() + 48, bar.height());
            QRadialGradient glow(QPointF(glowRect.center().x(), RULE_Y), glowRect.width() / 2.0);
            glow.setColorAt(0.0, QColor(GOLD_COLOR.red(), GOLD_COLOR.green(), GOLD_COLOR.blue(), 60));
            glow.setColorAt(1.0, QColor(GOLD_COLOR.red(), GOLD_COLOR.green(), GOLD_COLOR.blue(), 0));
            painter.save();
            painter.setClipRect(glowRect, Qt::IntersectClip);
            painter.fillRect(glowRect, glow);
            painter.restore();

            const int x1 = glowRect.left();
            const int x2 = glowRect.right();
            QLinearGradient line(x1, 0, x2, 0);
            line.setColorAt(0.0, QColor(214, 190, 130, 0));
            line.setColorAt(0.5, QColor(232, 212, 160, 235));
            line.setColorAt(1.0, QColor(214, 190, 130, 0));
            painter.fillRect(QRect(x1, RULE_Y - 1, x2 - x1, 2), line);
            painter.drawPixmap(QRect(x1, RULE_Y - TITLE_LINE_HEIGHT / 2, x2 - x1, TITLE_LINE_HEIGHT), m_titleLine);
        }
        painter.setOpacity(current ? 1.0 : (hovered ? 0.85 : 0.5));
        // Icon and title are centered on the band (above the rule), not the whole row.
        const QRect content(r.left(), bar.top(), r.width(), RULE_Y - bar.top());
        const QPixmap& icon = m_tabs[i].icon;
        painter.drawPixmap(content.left() + (ICON_SIZE - icon.width()) / 2,
                           content.top() + (content.height() - icon.height()) / 2, icon);
        if (content.width() > ICON_SIZE) {
            // Laid out at full width and clipped, so a tab sliding open reveals its title.
            painter.save();
            painter.setClipRect(content, Qt::IntersectClip);
            const QRect full(content.left(), content.top(), tabWidth(i, false), content.height());
            drawText(painter, full.adjusted(ICON_SIZE + ICON_GAP, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                     m_tabs[i].title, current ? TEXT_COLOR : DIM_TEXT_COLOR);
            painter.restore();
        }
        painter.setOpacity(1.0);
    }
}

void HeaderTabs::mouseMoveEvent(QMouseEvent* event) {
    const int hover = tabAt(event->pos());
    if (hover != m_hover) {
        m_hover = hover;
        m_animation.start();
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void HeaderTabs::mousePressEvent(QMouseEvent* event) {
    const int tab = event->button() == Qt::LeftButton ? tabAt(event->pos()) : -1;
    if (tab >= 0) {
        setCurrentIndex(tab);
        return;
    }
    QWidget::mousePressEvent(event);
}

void HeaderTabs::leaveEvent(QEvent* event) {
    m_hover = -1;
    m_animation.start();
    update();
    QWidget::leaveEvent(event);
}
}
