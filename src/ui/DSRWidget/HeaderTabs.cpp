#include "HeaderTabs.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Painting.h"

namespace fssm::ui::dsr {
namespace {
    // Same height as the "Characters" heading of the list beside it.
    constexpr int BAR_HEIGHT = 48;
    constexpr int ICON_SIZE = 30;
    constexpr int ICON_SPACING = 4;
    constexpr int LABEL_FONT_SIZE = 14;
}

HeaderTabs::HeaderTabs(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    m_stack = new QStackedWidget(this);
    m_stack->setAttribute(Qt::WA_TranslucentBackground, true);
    m_stack->installEventFilter(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, BAR_HEIGHT + 8, 0, 0);
    layout->addWidget(m_stack);
}

void HeaderTabs::addTab(const QString& title, const QString& subtitle, const QPixmap& icon, QWidget* page) {
    m_tabs.push_back({title, subtitle, icon, greyscale(icon)});
    m_stack->addWidget(page);
    if (m_current < 0) setCurrentIndex(0);
    update();
}

void HeaderTabs::setCurrentIndex(int index) {
    if (index < 0 || index >= int(m_tabs.size()) || index == m_current) return;
    m_current = index;
    m_stack->setCurrentIndex(index);
    update();
}

QRect HeaderTabs::iconRect(int index) const {
    const int count = int(m_tabs.size());
    const int left = width() - count * ICON_SIZE - (count - 1) * ICON_SPACING - 2;
    return {left + index * (ICON_SIZE + ICON_SPACING), 2, ICON_SIZE, ICON_SIZE};
}

int HeaderTabs::tabAt(const QPoint& pos) const {
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        if (iconRect(i).adjusted(-2, -2, 2, 2).contains(pos)) return i;
    }
    return -1;
}

void HeaderTabs::paintEvent(QPaintEvent* event) {
    if (m_current < 0) return;
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const int iconsLeft = iconRect(0).left();
    const Tab& current = m_tabs[m_current];
    drawMenuHeading(painter, QRect(0, 0, iconsLeft - 12, BAR_HEIGHT), current.icon,
                    current.title, current.subtitle, iconsLeft - 10);

    // Screen icons, as in the game's header; the current one framed in bronze.
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        const QRect r = iconRect(i);
        const bool isCurrent = i == m_current;
        painter.setOpacity(isCurrent ? 1.0 : (i == m_hover ? 1.0 : 0.7));
        painter.drawPixmap(r, (isCurrent || i == m_hover) ? m_tabs[i].icon : m_tabs[i].greyIcon);
        painter.setOpacity(1.0);
        if (isCurrent) {
            painter.setPen(QPen(GOLD_LIGHT_COLOR, 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRectF(r).adjusted(-1.5, -1.5, 1.5, 1.5));
        }
    }

    // The hovered icon's title, under the row of icons.
    if (m_hover >= 0) {
        painter.setFont(serifFont(LABEL_FONT_SIZE));
        const QRect labelRect(iconsLeft - 40, ICON_SIZE + 4, width() - iconsLeft + 40, BAR_HEIGHT - ICON_SIZE - 4);
        drawShadowText(painter, labelRect, Qt::AlignRight | Qt::AlignVCenter, m_tabs[m_hover].title,
                       m_hover == m_current ? TEXT_COLOR : AREA_COLOR);
    }
}

void HeaderTabs::mouseMoveEvent(QMouseEvent* event) {
    const int hover = tabAt(event->pos());
    if (hover != m_hover) {
        m_hover = hover;
        setCursor(hover >= 0 && hover != m_current ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void HeaderTabs::mousePressEvent(QMouseEvent* event) {
    const int tab = event->button() == Qt::LeftButton ? tabAt(event->pos()) : -1;
    if (tab >= 0) {
        setCurrentIndex(tab);
        setCursor(Qt::ArrowCursor);
        return;
    }
    QWidget::mousePressEvent(event);
}

void HeaderTabs::leaveEvent(QEvent* event) {
    m_hover = -1;
    update();
    QWidget::leaveEvent(event);
}

bool HeaderTabs::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_stack && event->type() == QEvent::Enter && m_hover >= 0) {
        m_hover = -1;
        unsetCursor();
        update();
    }
    return QWidget::eventFilter(watched, event);
}
}
