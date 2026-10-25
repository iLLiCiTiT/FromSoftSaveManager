#include "HeaderTabs.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Painting.h"

namespace fssm::ui::ds3 {
namespace {
    constexpr int BAR_HEIGHT = 32;
    constexpr int ICON_SIZE = 26;
    constexpr int TAB_SPACING = 20;
    constexpr int FONT_SIZE = 18;
}

HeaderTabs::HeaderTabs(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    m_stack = new QStackedWidget(this);
    m_stack->setAttribute(Qt::WA_TranslucentBackground, true);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, BAR_HEIGHT + 4, 0, 0);
    layout->addWidget(m_stack);
}

void HeaderTabs::addTab(const QString& title, const QString& icon, QWidget* page) {
    addTab(title, QPixmap(":/ds3_images/" + icon), page);
}

void HeaderTabs::addTab(const QString& title, const QPixmap& icon, QWidget* page) {
    QPixmap pix = icon;
    if (!pix.isNull() && pix.height() != ICON_SIZE) {
        pix = pix.scaled(ICON_SIZE, ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_tabs.push_back({title, pix});
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

int HeaderTabs::tabWidth(int index, bool iconOnly) const {
    if (iconOnly) return ICON_SIZE;
    return ICON_SIZE + 7 + QFontMetrics(serifFont(FONT_SIZE)).horizontalAdvance(m_tabs[index].title) + 6;
}

bool HeaderTabs::compact() const {
    int x = 4;
    for (int i = 0; i < int(m_tabs.size()); ++i) x += tabWidth(i, false) + TAB_SPACING;
    return x - TAB_SPACING > width();
}

QRect HeaderTabs::tabRect(int index) const {
    const bool iconsOnly = compact();
    int x = 4;
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        const int w = tabWidth(i, iconsOnly && i != m_current);
        if (i == index) return {x, 0, w, BAR_HEIGHT};
        x += w + TAB_SPACING;
    }
    return {};
}

int HeaderTabs::tabAt(const QPoint& pos) const {
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        if (tabRect(i).contains(pos)) return i;
    }
    return -1;
}

void HeaderTabs::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setFont(serifFont(FONT_SIZE));
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        const QRect r = tabRect(i);
        const bool current = i == m_current;
        painter.setOpacity(current ? 1.0 : (i == m_hover ? 0.8 : 0.5));
        painter.drawPixmap(r.left(), r.top() + (BAR_HEIGHT - m_tabs[i].icon.height()) / 2, m_tabs[i].icon);
        if (r.width() > ICON_SIZE) {
            drawShadowText(painter, r.adjusted(ICON_SIZE + 7, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                           m_tabs[i].title, current ? TEXT_COLOR : DIM_TEXT_COLOR);
        }
        painter.setOpacity(1.0);
        if (current) drawGoldRule(painter, r.left(), r.right() + 8, r.bottom() + 2, 220);
    }
}

void HeaderTabs::mouseMoveEvent(QMouseEvent* event) {
    const int hover = tabAt(event->pos());
    if (hover != m_hover) {
        m_hover = hover;
        setCursor(hover >= 0 && hover != m_current ? Qt::PointingHandCursor : Qt::ArrowCursor);
        setToolTip(hover >= 0 && hover != m_current && compact() ? m_tabs[hover].title : QString());
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
    update();
    QWidget::leaveEvent(event);
}
}
