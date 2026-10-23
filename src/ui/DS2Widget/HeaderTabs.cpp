#include "HeaderTabs.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <array>

#include "Painting.h"

namespace fssm::ui::ds2 {
namespace {
    // Medallion cells in ui_menu_tabs.png (287x60), left to right.
    constexpr std::array<QRect, 5> MEDALLION_RECTS = {{
        {9, 2, 53, 56},
        {61, 2, 53, 56},
        {114, 2, 54, 56},
        {168, 2, 54, 56},
        {222, 2, 54, 56},
    }};
    constexpr int ICON_SIZE = 28;
    constexpr int TEXT_GAP = 2;
    constexpr int TEXT_PADDING = 9;
    constexpr int TAB_SPACING = 8;
    constexpr int FONT_SIZE = 14;
    constexpr int BAND_HEIGHT = 22;
}

HeaderTabs::HeaderTabs(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    m_medallions = QPixmap(":/ds2_images/ui_menu_tabs");
    m_selectedBand = QPixmap(":/ds2_images/ui_row_selected");
    m_stack = new QStackedWidget(this);
    m_stack->setAttribute(Qt::WA_TranslucentBackground, true);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, HEADER_HEIGHT, 0, 0);
    layout->addWidget(m_stack);
}

void HeaderTabs::addTab(const QString& title, Medallion icon, QWidget* page) {
    QPixmap pix;
    if (!m_medallions.isNull()) {
        pix = m_medallions.copy(MEDALLION_RECTS[int(icon)])
            .scaled(ICON_SIZE, ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

QRect HeaderTabs::tabRect(int index) const {
    const QFontMetrics fm(serifFont(FONT_SIZE));
    int x = 2;
    for (int i = 0; i < int(m_tabs.size()); ++i) {
        const int w = ICON_SIZE + TEXT_GAP + fm.horizontalAdvance(m_tabs[i].title) + TEXT_PADDING * 2;
        if (i == index) return {x, 0, w, HEADER_HEIGHT - 4};
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
        const bool hovered = i == m_hover;
        const QRect iconRect(r.left(), r.top() + (r.height() - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE);
        const QRect textRect = r.adjusted(ICON_SIZE + TEXT_GAP, 0, 0, 0);

        // The amber band runs from under the medallion to the end of the title,
        // like the selected row in the game's lists.
        if ((current || hovered) && !m_selectedBand.isNull()) {
            const QRect band(iconRect.center().x(), r.center().y() - BAND_HEIGHT / 2 + 1,
                             r.right() - iconRect.center().x(), BAND_HEIGHT);
            painter.setOpacity(current ? 1.0 : 0.35);
            painter.drawPixmap(band, m_selectedBand);
        }

        painter.setOpacity(current ? 1.0 : (hovered ? 0.85 : 0.6));
        painter.drawPixmap(iconRect, m_tabs[i].icon);
        painter.setOpacity(1.0);
        drawOutlinedText(painter, textRect.adjusted(TEXT_PADDING, 0, -TEXT_PADDING, 0),
                         Qt::AlignLeft | Qt::AlignVCenter, m_tabs[i].title,
                         current ? TEXT_COLOR : DIM_TEXT_COLOR);
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
